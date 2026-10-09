#include "inditrans.h"
#include "script_constants.h"
#include "static_scripts.h"
#include "type_defs.h"
#include "utf.h"
#include <array>
#include <bit>
#include <cassert>
#include <limits>
#include <optional>
#include <string>
#include <variant>

using InditransLogger = void(const std::string&);
InditransLogger* inditransLogger = nullptr;

/// Replaces modern Grantha-derived consonants (ஸ, ஜ, ஜ²) with traditional Tamil orthography (ச, ச³, ச⁴)
/// when the TamilTraditional option is enabled.
constexpr std::string_view tamilTraditionalReplacement(std::string_view text) noexcept {
  if (text == "ஸ")
    return "ச";
  if (text == "ஜ")
    return "ச³";
  if (text == "ஜ²")
    return "ச⁴";
  return text;
}

/// Tests whether the transliteration option flag @p val is set within @p mask.
inline constexpr bool operator*(const TranslitOptions& mask, const TranslitOptions& val) noexcept {
  return (mask & val) == val;
}

/// Tests whether the transliteration option flag @p val is NOT set within @p mask.
inline constexpr bool operator/(const TranslitOptions& mask, const TranslitOptions& val) noexcept {
  return (mask & val) != val;
}

/// Discriminated union without std::variant footprint overhead.
///
/// Stores either a 4-byte ScriptToken payload (indicated by text == nullptr, with the token
/// bit-cast into value) or a borrowed UTF-8 string slice (text pointing to the buffer and
/// value holding the length). Occupies 16 bytes on x86-64 and 8 bytes on Wasm32.
struct TokenOrString {
  const char* text; ///< Pointer to borrowed text, or nullptr if holding a ScriptToken
  size_t value; ///< String length if text != nullptr; bit-cast ScriptToken payload if text == nullptr
  TokenOrString() = default;
  TokenOrString(ScriptToken token) noexcept
      : text(nullptr)
      , value(std::bit_cast<uint32_t>(token)) { }
  TokenOrString(std::string_view view) noexcept
      : text(view.data() ? view.data() : "")
      , value(view.size()) { }
};
static_assert(sizeof(ScriptToken) == sizeof(uint32_t));

/// Returns true if the item holds a ScriptToken (indicated by text == nullptr).
inline bool HoldsScriptToken(const TokenOrString& item) noexcept { return item.text == nullptr; }

/// Extracts the ScriptToken payload from a TokenOrString item.
inline ScriptToken GetScriptToken(const TokenOrString& item) noexcept {
  return std::bit_cast<ScriptToken>(static_cast<uint32_t>(item.value));
}

template <typename T> inline bool HoldsScriptToken(const T& var) { return std::holds_alternative<ScriptToken>(var); }
template <typename T> inline ScriptToken GetScriptToken(const T& var) { return std::get<ScriptToken>(var); }

/// Represents either a fully grouped syllabic TokenUnit or an untransliterated string view.
using TokenUnitOrString = std::variant<TokenUnit, std::string_view>;
template <typename T> inline bool HoldsTokenUnit(const T& var) { return std::holds_alternative<TokenUnit>(var); }
template <typename T> inline TokenUnit GetTokenUnit(const T& var) { return std::get<TokenUnit>(var); }

// Common for TokenOrString & TokenUnitOrString
template <typename T> inline bool HoldsString(const T& var) { return std::holds_alternative<std::string_view>(var); }
template <typename T> inline std::string_view GetString(const T& var) { return std::get<std::string_view>(var); }

/// Returns true if the item holds a borrowed string view.
inline bool HoldsString(const TokenOrString& item) noexcept { return item.text != nullptr; }

/// Extracts the borrowed string view from a TokenOrString item.
inline std::string_view GetString(const TokenOrString& item) noexcept { return { item.text, item.value }; }

const TokenUnitOrString endOfText("");
constexpr TokenUnit invalidTokenUnit(invalidScriptToken);

inline bool operator==(const TokenUnitOrString& a, const TokenUnitOrString& b) noexcept {
  if (HoldsString(a) && HoldsString(b)) {
    return GetString(a) == GetString(b);
  }
  if (HoldsTokenUnit(a) && HoldsTokenUnit(b)) {
    return GetTokenUnit(a) == GetTokenUnit(b);
  }
  return false;
}
inline bool operator!=(const TokenUnitOrString& a, const TokenUnitOrString& b) noexcept { return !(a == b); }

/// Streaming input tokenizer that scans UTF-8 text on demand and groups tokens into TokenUnits.
///
/// Operates with a bounded 16-element stack-allocated buffer (MaxSequence + Lookahead) without
/// dynamic allocation, ring-buffer arithmetic, or array shifting.
class InputReader {
public:
  InputReader(const std::string_view& input, const ScriptReaderMap& map, const TranslitOptions& options,
      const std::string_view& skipStart, const std::string_view& skipEnd) noexcept
      : ptr(input.data())
      , end(input.data() + input.length())
      , map(map)
      , skipStart(skipStart.empty() ? "##" : skipStart)
      , skipEnd(skipEnd.empty() ? "##" : skipEnd)
      , options(options) {
    if (map.nonRoman) {
      if (map.source)
        policyFn = [](InputReader& reader) { reader.pull<ReaderPolicy::Explicit>(); };
      else
        policyFn = [](InputReader& reader) { reader.pull<ReaderPolicy::Indic>(); };
    } else {
      if (map.folded)
        policyFn = [](InputReader& reader) { reader.pull<ReaderPolicy::FoldedRoman>(); };
      else
        policyFn = [](InputReader& reader) { reader.pull<ReaderPolicy::Roman>(); };
    }
  }

private:
  const char* ptr;
  const char* end;
  const ScriptReaderMap& map;
  const std::string_view skipStart;
  const std::string_view skipEnd;
  const TranslitOptions options;
  // Encoded sequences contain at most 15 tokens; the reader needs one
  // lookahead token. Leave room for a full sequence before each refill.
  static constexpr size_t MaxSequence = (1u << (16 - inditrans::static_data::sequenceOffsetBits)) - 1;
  static constexpr size_t Lookahead = 1;
  static constexpr size_t BufferCapacity = MaxSequence + Lookahead;
  static_assert(BufferCapacity >= MaxSequence + Lookahead);
  std::array<TokenOrString, BufferCapacity> buffer;
  size_t bufferSize = 0;
  size_t head = 0;
  void (*policyFn)(InputReader&);

  /// Ensures that at least @p requiredSize tokens/strings are available in the lookahead buffer.
  /// Refills only when the current expansion has been consumed, avoiding circular shifting.
  void ensure(size_t requiredSize) {
    if (bufferSize - head >= requiredSize)
      return;
    assert(requiredSize <= Lookahead);
    // All live calls request one token. Refill only after consuming the
    // current expansion, so no shifting or ring-buffer arithmetic is needed.
    bufferSize = 0;
    head = 0;
    while (bufferSize - head < requiredSize && ptr < end) {
      policyFn(*this);
    }
  }

  /// Pulls the next longest token match using the configured Policy, appending extracted
  /// ScriptTokens into the buffer, or delegates to pullUnrecognized on a miss.
  template <ReaderPolicy Policy> void pull() noexcept {
    auto match = map.lookupToken<Policy>(ptr, end);
    if (match.sequence) {
      const auto tokens = match.tokens();
      if (tokens.front().tokenType != TokenType::Accent || options / TranslitOptions::IgnoreVedicAccents) {
        assert(bufferSize + tokens.size() <= BufferCapacity);
        buffer[bufferSize++] = TokenOrString(tokens.front());
        for (size_t i = 1; i < tokens.size(); ++i)
          buffer[bufferSize++] = TokenOrString(tokens[i]);
      }
      ptr += match.matchLen;
    } else {
      ptr = pullUnrecognized<Policy>(ptr, end, map, skipStart, skipEnd);
    }
  }

  /// Extracts contiguous spans of non-transliterated text:
  /// 1. XML/HTML tags (e.g. `<tag>`) unless NoXMLTagHandling is set
  /// 2. Protected skip spans delimited by skipStart and skipEnd (e.g. `##protected##`)
  /// 3. Unrecognized character sequences that do not match any trie prefix
  template <ReaderPolicy Policy>
  [[gnu::noinline]] const char* pullUnrecognized(const char* ptr, const char* end, const ScriptReaderMap& map,
      const std::string_view& skipStart, const std::string_view& skipEnd) noexcept {
    const bool skipXml = !(options * TranslitOptions::NoXMLTagHandling);
    const auto* start = ptr;
    if (skipXml && *ptr == '<') {
      const auto close = std::string_view(ptr, end - ptr).find('>');
      ptr = close == std::string_view::npos ? end : ptr + close + 1;
      buffer[bufferSize++] = TokenOrString(std::string_view(start, ptr - start));
    } else if (!skipStart.empty() && *ptr == skipStart.front()
        && skipStart.size() <= static_cast<size_t>(end - ptr)
        && std::string_view(ptr, skipStart.size()) == skipStart) {
      ptr += skipStart.length();
      start = ptr;
      const auto close = skipEnd.empty() ? std::string_view::npos : std::string_view(ptr, end - ptr).find(skipEnd);
      if (close == std::string_view::npos) {
        // An unmatched opening delimiter discards the remaining input.
        ptr = end;
      } else {
        buffer[bufferSize++] = TokenOrString(std::string_view(start, close));
        ptr += close + skipEnd.length();
      }
    } else {
      // The trie consumes bytes, so raw runs can be scanned without decoding;
      // keep their original bytes intact until a token or delimiter starts.
      ptr++;
      while (ptr < end && (skipStart.empty() || *ptr != skipStart.front())
          && *ptr != '<' && map.lookupToken<Policy>(ptr, end).sequence == 0) {
        ptr++;
      }
      buffer[bufferSize++] = TokenOrString(std::string_view(start, ptr - start));
    }
    return ptr;
  }

public:
  /// Returns true if more tokens or raw string spans are available in the input stream.
  inline bool hasMore() noexcept {
    ensure(1);
    return head < bufferSize;
  }

  TokenUnit lastToken = invalidTokenUnit;

  /// Consumes the next token or string run and groups it into a complete TokenUnit cluster.
  /// Handles special cases like Gurmukhi adhak and Tamil prefixes.
  TokenUnitOrString getNext() noexcept {
    ensure(1);
    const auto& next = buffer[head++];
    if (HoldsString(next)) {
      wordStart = true;
      return GetString(next);
    }

    const auto& token = GetScriptToken(next);
    // Some special handlings
    if (token == GurmukhiAdhak) {
      return inferGurmukhiAdhak();
    }
    if (token == Skip) {
      return invalidTokenUnit;
    }
    TokenUnit tokenUnit = { token };
    switch (token.scriptType) {
      case ScriptType::Indic:
        tokenUnit = readIndicTokenUnit(token);
        break;
      case ScriptType::Tamil:
        tokenUnit = readTamilTokenUnit(token);
        break;
      case ScriptType::Latin:
        tokenUnit = readLatinTokenUnit(token);
        break;
      default:
        break;
    }
    wordStart = (token.tokenType == TokenType::Symbol);
    if (wordStart) {
      lastToken = invalidTokenUnit;
    } else {
      lastToken = tokenUnit;
    }
    return tokenUnit;
  }

private:
  /// Groups tokens for Indic scripts (Devanagari, Bengali, Telugu, etc.):
  /// attaches dependent vowel marks (matras), virama, diacritics (anuswara/visarga),
  /// and Vedic accents to the initiating consonant or independent vowel.
  TokenUnit readIndicTokenUnit(const ScriptToken& start) noexcept {
    TokenUnit tokenUnit = { start };
    if (start.tokenType == TokenType::Consonant) {
      while (true) {
        ensure(1);
        if (head >= bufferSize)
          break;
        if (!HoldsScriptToken(buffer[head]))
          break;
        const auto nextToken = GetScriptToken(buffer[head]);
        switch (nextToken.tokenType) {
          case TokenType::OtherDiacritic:
            tokenUnit.otherDiacritic = nextToken;
            break;
          case TokenType::VowelMark:
            tokenUnit.vowelMark = nextToken;
            break;
          case TokenType::Accent:
            tokenUnit.accent = nextToken;
            break;
          default:
            return tokenUnit;
        }
        head++;
      }
    } else if (start.tokenType == TokenType::Vowel) {
      while (true) {
        ensure(1);
        if (head >= bufferSize)
          break;
        if (!HoldsScriptToken(buffer[head]))
          break;
        const auto nextToken = GetScriptToken(buffer[head]);
        switch (nextToken.tokenType) {
          case TokenType::OtherDiacritic:
            tokenUnit.otherDiacritic = nextToken;
            break;
          case TokenType::Accent:
            tokenUnit.accent = nextToken;
            break;
          default:
            return tokenUnit;
        }
        head++;
      }
    }
    return tokenUnit;
  }

  /// Groups tokens for Tamil script, applying Tamil-specific phonological rules:
  /// - Parses attached superscript/subscript numbers (¹²³⁴) to distinguish aspirated/voiced allophones
  /// - Updates prefix recognizer state across prefix-stem boundaries
  /// - Applies contextual phonetic rules (e.g. converting ச to ஸ at word start or after certain vowels,
  ///   or voicing intervocalic / post-nasal stops)
  TokenUnit readTamilTokenUnit(const ScriptToken& start) noexcept {
    bool endOfPrefix = false;
    if (wordStart) {
      prefixLookupState.reset();
    } else {
      // A recognized prefix keeps start-of-stem pronunciation rules active.
      endOfPrefix = (prefixLookupState.value != std::nullopt);
    }

    TokenUnit tokenUnit = { start };
    if (start.tokenType == TokenType::Consonant) {
      bool isPrimary = start.idx <= 20 /* ப */ && start.idx % 5 == 0;
      bool hasVirama = false;

      while (true) {
        ensure(1);
        if (head >= bufferSize)
          break;
        if (HoldsScriptToken(buffer[head])) {
          auto nextToken = GetScriptToken(buffer[head]);
          switch (nextToken.tokenType) {
            case TokenType::OtherDiacritic:
              head++;
              tokenUnit.otherDiacritic = nextToken;
              break;
            case TokenType::VowelMark:
              head++;
              tokenUnit.vowelMark = nextToken;
              hasVirama = tokenUnit.vowelMark == Virama;
              break;
            case TokenType::Accent:
              head++;
              tokenUnit.accent = nextToken;
              break;
            default:
              goto done;
          }
        } else if ((isPrimary || start.idx == 7 /* ஜ */)) {
          auto offset = TamilSuperscripts.find(GetString(buffer[head]));
          if (offset != TamilSuperscripts.npos) {
            tokenUnit.leadToken = start.clone(static_cast<uint8_t>(start.idx + offset / "²"_len));
            head++;
          } else if ((offset = TamilSubscripts.find(GetString(buffer[head]))) != TamilSubscripts.npos) {
            tokenUnit.leadToken = start.clone(static_cast<uint8_t>(start.idx + offset / "²"_len));
            head++;
          } else {
            break;
          }
        } else {
          break;
        }
      }
    done:
      if (options * TranslitOptions::TamilSuperscripted) {
        // Explicit consonant variants bypass contextual pronunciation inference.
        return tokenUnit;
      }

      // Match written Tamil before changing stop indices to their voiced forms.
      prefixLookup.lookup(tokenUnit, prefixLookupState);

      if (hasVirama) {
        // The legacy pronunciation rule drops a word-final primary stop with
        // pulli as a whole unit, including any attached modifiers.
        return (isPrimary && isEndOfWord()) ? invalidTokenUnit : tokenUnit;
      }
      if (isPrimary) {
        if (wordStart || endOfPrefix) {
          const std::array<uint8_t, 6> specialVowelDiactritics = { 2, 4, 10, 13, 16, InvalidToken };
          if (tokenUnit.leadToken.idx == 5 /* ச */ && lastToken.leadToken != tokenUnit.leadToken && !isVirama(lastToken)
              && std::find(specialVowelDiactritics.begin(), specialVowelDiactritics.end(), lastToken.vowelMark.idx)
                  == specialVowelDiactritics.end()) {
            tokenUnit.leadToken = start.clone(31 /* ஸ */);
          }
        } else if (isVirama(lastToken)) {
          // Geminates and stops after hard consonants stay unvoiced. Other
          // clusters voice the stop (+2 in its varga), with a separate ச→ஸ rule.
          if (lastToken.leadToken != tokenUnit.leadToken && !isHardConsonant(lastToken)) {
            if (tokenUnit.leadToken.idx == 5 /* ச */ && !isSoftConsonant(lastToken)) {
              tokenUnit.leadToken = start.clone(31 /* ஸ */);
            } else {
              tokenUnit.leadToken = start.clone(static_cast<uint8_t>(start.idx + 2));
            }
          }
        } else {
          if (tokenUnit.leadToken.idx == 5 /* ச */) {
            tokenUnit.leadToken = start.clone(31 /* ஸ */);
          } else {
            tokenUnit.leadToken = start.clone(static_cast<uint8_t>(start.idx + 2));
          }
        }
      }
    } else if (start.tokenType == TokenType::Vowel) {
      while (true) {
        ensure(1);
        if (head >= bufferSize)
          break;
        if (!HoldsScriptToken(buffer[head]))
          break;
        const auto nextToken = GetScriptToken(buffer[head]);
        if (nextToken.tokenType == TokenType::OtherDiacritic) {
          tokenUnit.otherDiacritic = nextToken;
        } else if (nextToken.tokenType == TokenType::Accent) {
          tokenUnit.accent = nextToken;
        } else {
          break;
        }
        head++;
      }
    }
    if (tokenUnit.leadToken.tokenType != TokenType::Consonant) {
      prefixLookup.lookup(tokenUnit, prefixLookupState);
    }
    return tokenUnit;
  }

  /// Groups tokens for Latin transliteration schemes:
  /// attaches subsequent vowels or vowel marks to a lead consonant.
  /// If a consonant is not followed by a vowel, an explicit virama is synthesized.
  TokenUnit readLatinTokenUnit(const ScriptToken& start) noexcept {
    TokenUnit tokenUnit = { start };
    if (start.tokenType == TokenType::Consonant) {
      bool vowelAdded = false;

      while (true) {
        ensure(1);
        if (head >= bufferSize)
          break;
        if (!HoldsScriptToken(buffer[head]))
          break;
        auto nextToken = GetScriptToken(buffer[head]);
        bool consume = false;
        switch (nextToken.tokenType) {
          case TokenType::Vowel:
          case TokenType::VowelMark:
            if (!vowelAdded) {
              vowelAdded = true;
              // Roman vowel index 0 is 'a'; leaving the mark absent represents
              // the inherent vowel. The same index in VowelMark means virama.
              if (nextToken.idx != Diacritic_Virama) {
                tokenUnit.vowelMark = { TokenType::VowelMark, nextToken.idx };
              }
              consume = true;
            }
            break;
          case TokenType::OtherDiacritic:
            tokenUnit.otherDiacritic = nextToken;
            consume = true;
            break;
          case TokenType::Accent:
            tokenUnit.accent = nextToken;
            consume = true;
            break;
          default:
            break;
        }
        if (!consume) {
          break;
        }
        head++;
      }

      if (!vowelAdded) {
        tokenUnit.vowelMark = { TokenType::VowelMark, 0 };
      }
    }
    return tokenUnit;
  }

  /// Handles the Gurmukhi adhak gemination symbol:
  /// peeks ahead at the following consonant and emits a halved version of it with virama
  /// (e.g. adhak + k -> k + virama + k).
  TokenUnit inferGurmukhiAdhak() {
    ensure(1);
    if (head >= bufferSize)
      return invalidTokenUnit;
    const auto peek = buffer[head];
    if (HoldsScriptToken(peek) && GetScriptToken(peek).tokenType == TokenType::Consonant) {
      // Leave the following consonant buffered: this unit supplies only the
      // first half of the geminate, with aspiration removed within its varga.
      TokenUnit tokenUnit = { GetScriptToken(peek) };
      if (tokenUnit.leadToken.idx < 24) {
        tokenUnit.leadToken.idx -= tokenUnit.leadToken.idx % 5 % 2;
      }
      tokenUnit.vowelMark = Virama;
      return tokenUnit;
    } else {
      return invalidTokenUnit;
    }
  }

  inline bool isHardConsonant(const TokenUnit& token) {
    return token.leadToken.tokenType == TokenType::Consonant
        && ((token.leadToken.idx <= 20 && token.leadToken.idx % 5 == 0 /* க ச ட த ப */)
            || token.leadToken.idx == 35 /* ற */);
  }

  inline bool isSoftConsonant(const TokenUnit& token) {
    auto idx = token.leadToken.idx;
    return token.leadToken.tokenType == TokenType::Consonant
        && ((idx <= 24 && idx % 5 == 4 /* ங ஞ ண ந ம */) || idx == 36 /* ன */ || (idx >= 25 && idx <= 28 /* ய ர ல வ */)
            || idx == 33 /* ள */ || idx == 34 /* ழ */);
  }

  inline bool isVirama(const TokenUnit& token) {
    return token.vowelMark.tokenType == TokenType::VowelMark && token.vowelMark.idx == Diacritic_Virama;
  }

  bool isEndOfWord() noexcept {
    ensure(1);
    if (head >= bufferSize) {
      return true;
    }
    const auto& next = buffer[head];
    if (HoldsString(next)) {
      return true;
    }
    const auto token = GetScriptToken(next);
    return token.tokenType == TokenType::Symbol;
  }

  inline bool isDevanagariExtended(int ch) { return ch >= 0xA8E0 && ch <= 0xA8FF; }

  inline bool isVedicExtension(int ch) { return ch >= 0x1CD0 && ch <= 0x1CFA; }

  std::optional<TokenOrString> peekNext(size_t offset = 0) noexcept {
    ensure(offset + 1);
    if (head + offset >= bufferSize) {
      return std::nullopt;
    }
    return buffer[head + offset];
  }

  bool isNextSpace(size_t offset = 0) noexcept {
    auto next = peekNext(offset);
    if (next == std::nullopt) {
      return false;
    }
    if (!HoldsString(buffer[head + offset])) {
      return false;
    }
    auto str = GetString(buffer[head + offset]);
    for (auto c : str) {
      if (!std::isspace(c)) {
        return false;
      }
    }
    return true;
  }

private:
  bool wordStart { true };
  TamilPrefixLookup prefixLookup;
  TamilPrefixLookup::LookupState prefixLookupState { };
};

/// Output generator specialized by ScriptType (Indic, Tamil, Latin) and output Sink.
///
/// Converts TokenUnit objects into target graphemes using the compiled ScriptWriterMap,
/// applying script-specific orthographic rules, accent stripping, marker filtering, and vowel inferences.
template <typename Sink, ScriptType Type> class OutputWriter {
public:
  virtual ~OutputWriter() = default;

  /// Writes a single TokenUnit cluster or raw string slice, with lookahead to the next token
  /// for contextual orthography rules (such as anuswara assimilation or end-of-word checks).
  void writeTokenUnit(const TokenUnitOrString& tokenUnitOrString, const TokenUnitOrString& next) noexcept {
    if (HoldsString(tokenUnitOrString)) {
      push(GetString(tokenUnitOrString));
      wordStart = true;
    } else {
      auto tokenUnit = GetTokenUnit(tokenUnitOrString);
      if (tokenUnit.leadToken.tokenType == TokenType::Ignore) {
        return;
      }
      switch (Type) {
        case ScriptType::Indic:
          writeIndicTokenUnit(tokenUnit);
          break;
        case ScriptType::Tamil:
          writeTamilTokenUnit(tokenUnit, next);
          break;
        case ScriptType::Latin:
          writeLatinTokenUnit(tokenUnit, next);
          break;
        default:
          return;
      }
      wordStart = (tokenUnit.leadToken.tokenType == TokenType::Symbol);
    }
  }

  OutputWriter(const ScriptWriterMap& map, const TranslitOptions options, Sink& sink) noexcept
      : map(map)
      , options(options)
      , buffer(sink) {
    setNasalConsonantSize();
  }

protected:
  /// Appends text to the destination sink, filtering out special disambiguation markers
  /// unless RetainSpecialMarkers is configured.
  inline void push(const std::string_view& text) {
    if (options / TranslitOptions::RetainSpecialMarkers) {
      stripSpecialMarkers(text, buffer);
    } else {
      buffer += text;
    }
  }

protected:
  /// Writes a TokenUnit in standard Indic Brahmic scripts (Devanagari, Bengali, Telugu, etc.):
  /// emits base consonant or vowel, attached matras, diacritics, and Vedic pitch accents.
  void writeIndicTokenUnit(const TokenUnit& tokenUnit) noexcept {
    if (options * TranslitOptions::ASCIINumerals && tokenUnit.leadToken.tokenType == TokenType::Symbol
        && tokenUnit.leadToken.idx < 10) {
      char str[2] = { static_cast<char>('0' + tokenUnit.leadToken.idx), 0 };
      push(str);
    } else {
      push(map.lookupChar(tokenUnit.leadToken));
    }
    if (tokenUnit.vowelMark.idx != InvalidToken) {
      push(map.lookupChar(tokenUnit.vowelMark));
    }
    if (tokenUnit.otherDiacritic.idx != InvalidToken) {
      push(map.lookupChar(tokenUnit.otherDiacritic));
    }
    if (tokenUnit.accent.idx != InvalidToken && options / TranslitOptions::IgnoreVedicAccents) {
      push(map.lookupChar(tokenUnit.accent));
    }
  }

  /// Writes a TokenUnit in Tamil script, enforcing Tamil orthographic constraints:
  /// - Distributes dental 'ந' at word/prefix start vs alveolar 'ன' word-medially/finally
  /// - Applies optional TamilTraditional consonant replacements (e.g. ஸ -> ச)
  /// - Places superscripts (¹²³⁴) after consonant or after matra as appropriate
  /// - Inserts superscripts between the components of multi-part vowel marks
  /// - Assimilates anuswara into following varga nasal stop
  void writeTamilTokenUnit(const TokenUnit& tokenUnit, const TokenUnitOrString& next) noexcept {
    bool endOfPrefix = prefixLookupState.value != std::nullopt;
    if (wordStart) {
      prefixLookupState.reset();
    }
    prefixLookup.lookup(tokenUnit, prefixLookupState);

    auto leadIdx = tokenUnit.leadToken.idx;
    auto leadText = map.lookupChar(tokenUnit.leadToken);

    if (tokenUnit.leadToken.tokenType == TokenType::Consonant) {
      // Keep dental ந at word/prefix starts and within consonant clusters;
      // use alveolar ன for medial syllables with vowels and final bare consonants.
      if (leadIdx == 19 /* ந */ && !(wordStart || endOfPrefix)
          && (tokenUnit.vowelMark.idx != Diacritic_Virama || isEndOfWord(next))) {
        leadText = "ன";
      } else if (options * TranslitOptions::TamilTraditional) {
        leadText = tamilTraditionalReplacement(leadText);
      }

      auto superscript = Utf8String::trailingChar(leadText).view();
      if (leadText.length() > superscript.length() && TamilSuperscripts.find(superscript) != TamilSuperscripts.npos) {
        leadText = { leadText.data(), leadText.length() - superscript.length() };
      } else {
        superscript = "";
      }
      push(leadText);
      auto consonantPosition = buffer.size();

      std::string_view vowelMarkReminder { };
      if (tokenUnit.vowelMark.idx != InvalidToken) {
        auto vowelMark = map.lookupChar(tokenUnit.vowelMark);
        if (vowelMark.size() > UtfUtils::nextCharLen(vowelMark.data())) {
          push(UtfUtils::nextUtf8Char(vowelMark.data()));
          vowelMarkReminder = vowelMark.substr(UtfUtils::nextCharLen(vowelMark.data()));
        } else {
          push(vowelMark);
        }
      }

      if (options * TranslitOptions::TamilSuperscripted && superscript != "") {
        push(superscript);
      }
      if (vowelMarkReminder.length() > 0) {
        push(vowelMarkReminder);
      }

      if (tokenUnit.otherDiacritic.idx != InvalidToken) {
        if (tokenUnit.otherDiacritic.idx == Diacritic_Anuswara) {
          inferAnuswara(next);
        } else {
          push(map.lookupChar(tokenUnit.otherDiacritic));
        }
      }
    } else {
      if (tokenUnit.leadToken.tokenType == TokenType::Vowel) {
        push(leadText);

        if (tokenUnit.otherDiacritic.idx != InvalidToken) {
          if (tokenUnit.otherDiacritic.idx == Diacritic_Anuswara) {
            inferAnuswara(next);
          } else {
            push(map.lookupChar(tokenUnit.otherDiacritic));
          }
        }
      } else if (options * TranslitOptions::ASCIINumerals && tokenUnit.leadToken.tokenType == TokenType::Symbol
          && tokenUnit.leadToken.idx < 10) {
        char str[2] = { static_cast<char>('0' + tokenUnit.leadToken.idx), 0 };
        push(str);
      } else {
        push(leadText);
      }
    }

    if ((options / TranslitOptions::IgnoreVedicAccents) && (tokenUnit.accent.idx != InvalidToken)
        && (tokenUnit.leadToken.tokenType == TokenType::Consonant
            || tokenUnit.leadToken.tokenType == TokenType::Vowel)) {
      auto lastChar = buffer.back().view();
      if (TamilSpecialChars.find(lastChar) != TamilSpecialChars.npos) {
        // Place the accent before a trailing modifier. Copy the borrowed view
        // before appending, since growing the sink can invalidate its storage.
        std::string lastCharStr { lastChar };
        buffer.pop_back();
        push(map.lookupChar(tokenUnit.accent));
        push(lastCharStr);
      } else {
        push(map.lookupChar(tokenUnit.accent));
      }
    }
  }

  /// Writes a TokenUnit in Latin transliteration:
  /// an absent vowel mark emits the inherent 'a'; an explicit virama emits no vowel.
  void writeLatinTokenUnit(const TokenUnit& tokenUnit, const TokenUnitOrString& next) noexcept {
    auto& leadToken = tokenUnit.leadToken;
    push(map.lookupChar(leadToken.tokenType, leadToken.idx));
    if (leadToken.tokenType == TokenType::Consonant) {
      if (tokenUnit.vowelMark.idx == InvalidToken) {
        push(map.lookupChar(TokenType::Vowel, Diacritic_Virama));
      } else if (tokenUnit.vowelMark.idx != Diacritic_Virama) {
        push(map.lookupChar(tokenUnit.vowelMark));
      }
    }
    if (tokenUnit.accent.idx != InvalidToken && options / TranslitOptions::IgnoreVedicAccents) {
      push(map.lookupChar(tokenUnit.accent));
    }
    if (tokenUnit.otherDiacritic.idx != InvalidToken) {
      auto lookup = map.lookupChar(tokenUnit.otherDiacritic);
      if (tokenUnit.otherDiacritic.idx == Diacritic_Anuswara && lookup.size() == 0) {
        inferAnuswara(next);
      } else {
        push(lookup);
      }
    }
  }

  /// Assimilates anuswara (nasalization) to the class nasal of the following consonant
  /// (e.g. ṅ before gutturals, ñ before palatals, ṇ before retroflex, n before dentals, m before labials).
  void inferAnuswara(const TokenUnitOrString& next) noexcept {
    // Default to म when no varga can be inferred; each five-entry varga ends
    // with its nasal. Tamil needs a pulli to keep that nasal vowel-free.
    size_t idx = 24 /* म */;
    if (next != endOfText && !HoldsString(next)) {
      auto tokenUnit = GetTokenUnit(next);
      if (tokenUnit.leadToken.idx < 24) {
        idx = (((tokenUnit.leadToken.idx / 5) * 5) + 4);
      }
    }
    push(map.lookupChar(TokenType::Consonant, idx));
    if (map.getType() == ScriptType::Tamil) {
      push(map.lookupChar(Virama));
    }
  }

  /// Strips internal non-ASCII disambiguation markers (e.g. 'ʽ', 'ʼ', 'ˮ', 'ˇ')
  /// from the emitted text runs.
  template <typename BufType> void stripSpecialMarkers(const std::string_view& in, BufType& out) noexcept {
    // Every marker byte is non-ASCII; preserve the substring predicate for
    // non-ASCII input, including the existing treatment of stray UTF-8 bytes.
    static_assert([] {
      for (const auto byte : SpecialMarkers)
        if (static_cast<uint8_t>(byte) < 0x80)
          return false;
      return true;
    }());
    const char* ptr = in.data();
    const char* end = ptr + in.length();
    const char* run = ptr;
    while (ptr < end) {
      const auto ch = UtfUtils::nextUtf8Char(ptr);
      if (static_cast<uint8_t>(ch.front()) >= 0x80 && SpecialMarkers.find(ch) != SpecialMarkers.npos) {
        if (ptr != run)
          out += std::string_view(run, ptr - run);
        ptr += ch.length();
        run = ptr;
      } else {
        ptr += ch.length();
      }
    }
    if (ptr != run)
      out += std::string_view(run, ptr - run);
  }

  void setNasalConsonantSize() noexcept {
    const auto& anuswara = map.lookupChar(TokenType::OtherDiacritic, Diacritic_Anuswara);
    if (options / TranslitOptions::RetainSpecialMarkers) {
      std::string out { };
      stripSpecialMarkers(anuswara, out);
    }
  }

  bool isEndOfWord(const TokenUnitOrString& next) const noexcept {
    if (next == endOfText) {
      return true;
    }
    if (HoldsString(next)) {
      return TamilSuperscripts.find(GetString(next)) == TamilSuperscripts.npos;
    }
    const auto token = GetTokenUnit(next);
    return token.leadToken.tokenType == TokenType::Symbol;
  }

private:
  const ScriptWriterMap& map;
  const TranslitOptions options;
  Sink& buffer;
  bool wordStart { true };
  TamilPrefixLookup prefixLookup;
  TamilPrefixLookup::LookupState prefixLookupState { };
};

/// Calculates an initial capacity estimation for the destination output buffer.
/// When expanding Roman / Latin scripts into multi-byte Indic UTF-8 graphemes,
/// reserves a 3x byte multiplier hint to avoid recurring reallocations.
size_t outputCapacity(size_t bytes, const ScriptReaderMap& reader, const ScriptWriterMap& writer) noexcept {
  if (!reader.nonRoman && writer.getType() != ScriptType::Latin
      && bytes <= (std::numeric_limits<size_t>::max() - 1) / 3)
    return bytes * 3 + 1;
  return bytes + 1;
}

/// Typed transliteration driver: instantiates InputReader and OutputWriter specialized on Type,
/// iterating through TokenUnits with 1-unit lookahead and passing them to the writer.
template <typename Sink, ScriptType Type>
bool transliterate_typed(const std::string_view& input, const ScriptReaderMap& readerMap,
    const ScriptWriterMap& writerMap, TranslitOptions options, Sink& sink, const std::string_view& skipStart,
    const std::string_view& skipEnd) noexcept {
  InputReader reader(input, readerMap, options, skipStart, skipEnd);

  if (writerMap.getType() == ScriptType::Indic && !writerMap.isVedic()) {
    // Keep source tokenization unchanged; only the writer suppresses accents
    // that the target script cannot represent.
    options = options | TranslitOptions::IgnoreVedicAccents;
  }
  OutputWriter<Sink, Type> writer(writerMap, options, sink);

  TokenUnitOrString curr = (reader.hasMore() ? reader.getNext() : endOfText);
  while (curr != endOfText) {
    TokenUnitOrString next = (reader.hasMore() ? reader.getNext() : endOfText);
    writer.writeTokenUnit(curr, next);
    curr = next;
  }

  return true;
}

/// Core transliteration dispatcher: routes execution to the appropriate OutputWriter specialization
/// based on the target script's ScriptType (Indic, Tamil, or Latin).
template <typename Sink>
bool transliterate_core(const std::string_view& input, const ScriptReaderMap& readerMap,
    const ScriptWriterMap& writerMap, TranslitOptions options, Sink& sink, const std::string_view& skipStart,
    const std::string_view& skipEnd) noexcept {
  switch (writerMap.getType()) {
    case ScriptType::Indic:
      return transliterate_typed<Sink, ScriptType::Indic>(
          input, readerMap, writerMap, options, sink, skipStart, skipEnd);
    case ScriptType::Tamil:
      return transliterate_typed<Sink, ScriptType::Tamil>(
          input, readerMap, writerMap, options, sink, skipStart, skipEnd);
    case ScriptType::Latin:
      return transliterate_typed<Sink, ScriptType::Latin>(
          input, readerMap, writerMap, options, sink, skipStart, skipEnd);
    default:
      return false;
  }
}

/// Transliterates input into a heap-allocated TranslitBuffer via Utf8StringBuilder.
bool transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, TranslitBuffer& output, const std::string_view& skipStart,
    const std::string_view& skipEnd) noexcept {
  if (from == to)
    return false;
  const auto* readerMap = getScriptReaderMap(from);
  const auto* writerMap = getScriptWriterMap(to);
  if (!readerMap || !writerMap)
    return false;
  Utf8StringBuilder sink;
  sink.reserve(outputCapacity(input.length(), *readerMap, *writerMap));
  if (!transliterate_core(input, *readerMap, *writerMap, options, sink, skipStart, skipEnd)) {
    return false;
  }
  output.reset(sink.release());
  return true;
}

/// Zero-copy sink adaptor wrapping an existing std::string reference.
struct StdStringSink {
  std::string& str;
  StdStringSink(std::string& s)
      : str(s) { }
  void operator+=(char ch) { str += ch; }
  void operator+=(std::string_view view) { str += view; }
  size_t size() const { return str.size(); }
  Utf8Char back() const {
    auto len = UtfUtils::prevCharLen(str.data() + str.size());
    return Utf8Char(str.data() + str.size() - len);
  }
  void pop_back() {
    auto len = UtfUtils::prevCharLen(str.data() + str.size());
    str.resize(str.size() - len);
  }
};

/// Transliterates input directly into an existing std::string buffer.
bool transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, std::string& output, const std::string_view& skipStart,
    const std::string_view& skipEnd) noexcept {
  if (from == to)
    return false;
  const auto* readerMap = getScriptReaderMap(from);
  const auto* writerMap = getScriptWriterMap(to);
  if (!readerMap || !writerMap)
    return false;
  // Append calls use the existing input-sized hint. Reserving extra prefix
  // capacity could invalidate a borrowed input view before the reader starts.
  output.reserve(output.empty() ? outputCapacity(input.length(), *readerMap, *writerMap) : input.length() + 1);
  StdStringSink sink(output);
  return transliterate_core(input, *readerMap, *writerMap, options, sink, skipStart, skipEnd);
}

/// Transliterates input and returns the result as a new std::string by value.
std::string transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, const std::string_view& skipStart, const std::string_view& skipEnd) noexcept {
  std::string output;
  if (!transliterate(input, from, to, options, output, skipStart, skipEnd)) {
    return std::string();
  }

  return output;
}

extern "C" {

/// transliterate
char* CALL_CONV transliterate(const char* input, const char* from, const char* to, unsigned long options,
    const char* skipStart, const char* skipEnd) {
  TranslitBuffer output;
  std::string_view inputView(input);
  std::string_view skipStartView(skipStart);
  std::string_view skipEndView(skipEnd);
  if (!transliterate(inputView, from, to, static_cast<TranslitOptions>(options), output, skipStartView, skipEndView)) {
    return nullptr;
  } else {
    auto retval = output.release();
    return retval;
  }
}

/// Checks concrete script names and aliases against the compiled name table.
int CALL_CONV isScriptSupported(const char* script) { return inditrans::static_data::findScript(script) >= 0; }

/// releaseBuffer
void CALL_CONV releaseBuffer(char* buffer) { std::free(buffer); }

} // extern "C"
