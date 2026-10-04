#include "inditrans.h"
#include "script_constants.h"
#include "static_scripts.h"
#include "type_defs.h"
#include "utf.h"
#include <optional>
#include <string>
#include <variant>
#include <vector>

using InditransLogger = void(const std::string&);
InditransLogger* inditransLogger = nullptr;

constexpr std::string_view tamilTraditionalReplacement(std::string_view text) noexcept {
  if (text == "ஸ")
    return "ச";
  if (text == "ஜ")
    return "ச³";
  if (text == "ஜ²")
    return "ச⁴";
  return text;
}

inline constexpr bool operator*(const TranslitOptions& mask, const TranslitOptions& val) noexcept {
  return (mask & val) == val;
}
inline constexpr bool operator/(const TranslitOptions& mask, const TranslitOptions& val) noexcept {
  return (mask & val) != val;
}

using TokenOrString = std::variant<ScriptToken, std::string_view>;
template <typename T> inline bool HoldsScriptToken(const T& var) { return std::holds_alternative<ScriptToken>(var); }
template <typename T> inline ScriptToken GetScriptToken(const T& var) { return std::get<ScriptToken>(var); }

using TokenUnitOrString = std::variant<TokenUnit, std::string_view>;
template <typename T> inline bool HoldsTokenUnit(const T& var) { return std::holds_alternative<TokenUnit>(var); }
template <typename T> inline TokenUnit GetTokenUnit(const T& var) { return std::get<TokenUnit>(var); }

// Common for TokenOrString & TokenUnitOrString
template <typename T> inline bool HoldsString(const T& var) { return std::holds_alternative<std::string_view>(var); }
template <typename T> inline std::string_view GetString(const T& var) { return std::get<std::string_view>(var); }

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

class InputReader {
public:
  InputReader(const std::string_view& input, const ScriptReaderMap& map, const TranslitOptions& options,
      const std::string_view& skipStart = "##", const std::string_view& skipEnd = "##") noexcept
      : options(options) {
    tokenUnits.reserve(input.size());
    if (map.nonRoman) {
      if (map.source)
        scan<ReaderPolicy::Explicit>(input, map, skipStart, skipEnd);
      else
        scan<ReaderPolicy::Indic>(input, map, skipStart, skipEnd);
    } else {
      if (map.folded)
        scan<ReaderPolicy::FoldedRoman>(input, map, skipStart, skipEnd);
      else
        scan<ReaderPolicy::Roman>(input, map, skipStart, skipEnd);
    }
    iter = tokenUnits.begin();
  }

private:
  template <ReaderPolicy Policy>
  void scan(const std::string_view& input, const ScriptReaderMap& map, const std::string_view& skipStart,
      const std::string_view& skipEnd) noexcept {
    auto ptr = input.data();
    auto end = ptr + input.length();

    while (ptr < end) {
      auto match = map.lookupToken<Policy>(ptr, end);
      if (match.sequence) {
        const auto tokens = match.tokens();
        if (tokens.front().tokenType != TokenType::Accent || options / TranslitOptions::IgnoreVedicAccents) {
          tokenUnits.emplace_back(tokens.front());
          for (size_t i = 1; i < tokens.size(); ++i)
            tokenUnits.emplace_back(tokens[i]);
        }
        ptr += match.matchLen;
      } else {
        ptr = scanUnrecognized<Policy>(ptr, end, map, skipStart, skipEnd);
      }
    }
  }

  template <ReaderPolicy Policy>
  [[gnu::noinline]] const char* scanUnrecognized(const char* ptr, const char* end, const ScriptReaderMap& map,
      const std::string_view& skipStart, const std::string_view& skipEnd) noexcept {
    const bool skipXml = !(options * TranslitOptions::NoXMLTagHandling);
    while (ptr < end) {
      const auto* start = ptr;
      if (skipXml && *ptr == '<') {
        const auto close = std::string_view(ptr, end - ptr).find('>');
        ptr = close == std::string_view::npos ? end : ptr + close + 1;
        tokenUnits.emplace_back(std::string_view(start, ptr - start));
      } else if (*ptr == skipStart[0] && ptr + skipStart.length() - 1 < end
          && std::string_view(ptr, skipStart.length()) == skipStart) {
        ptr += skipStart.length();
        start = ptr;
        const auto close = skipEnd.empty() ? std::string_view::npos : std::string_view(ptr, end - ptr).find(skipEnd);
        if (close == std::string_view::npos) {
          ptr = end;
        } else {
          tokenUnits.emplace_back(std::string_view(start, close));
          ptr += close + skipEnd.length();
        }
      } else if (map.lookupToken<Policy>(ptr, end).sequence != 0) {
        break;
      } else {
        ptr++;
        while (ptr < end && *ptr != skipStart[0] && *ptr != '<' && map.lookupToken<Policy>(ptr, end).sequence == 0) {
          ptr++;
        }
        tokenUnits.emplace_back(std::string_view(start, ptr - start));
      }
    }
    return ptr;
  }

public:
  inline bool hasMore() noexcept { return iter < tokenUnits.end(); }

  TokenUnit lastToken = invalidTokenUnit;
  TokenUnitOrString getNext() noexcept {
    const auto& next = *iter++;
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
  TokenUnit readIndicTokenUnit(const ScriptToken& start) noexcept {
    TokenUnit tokenUnit = { start };
    if (start.tokenType == TokenType::Consonant) {
      while (iter < tokenUnits.end() && HoldsScriptToken(*iter)) {
        const auto nextToken = GetScriptToken(*iter);
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
        iter++;
      }
    } else if (start.tokenType == TokenType::Vowel) {
      while (iter < tokenUnits.end() && HoldsScriptToken(*iter)) {
        const auto nextToken = GetScriptToken(*iter);
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
        iter++;
      }
    }
    return tokenUnit;
  }

  TokenUnit readTamilTokenUnit(const ScriptToken& start) noexcept {
    bool endOfPrefix = false;
    if (wordStart) {
      prefixLookupState.reset();
    } else {
      endOfPrefix = (prefixLookupState.value != std::nullopt);
    }

    TokenUnit tokenUnit = { start };
    if (start.tokenType == TokenType::Consonant) {
      bool isPrimary = start.idx <= 20 /* ப */ && start.idx % 5 == 0;
      bool hasVirama = false;

      while (iter < tokenUnits.end()) {
        if (HoldsScriptToken(*iter)) {
          auto nextToken = GetScriptToken(*iter);
          switch (nextToken.tokenType) {
            case TokenType::OtherDiacritic:
              iter++;
              tokenUnit.otherDiacritic = nextToken;
              break;
            case TokenType::VowelMark:
              iter++;
              tokenUnit.vowelMark = nextToken;
              hasVirama = tokenUnit.vowelMark == Virama;
              break;
            case TokenType::Accent:
              iter++;
              tokenUnit.accent = nextToken;
              break;
            default:
              goto done;
          }
        } else if ((isPrimary || start.idx == 7 /* ஜ */) && iter < tokenUnits.end()) {
          auto offset = TamilSuperscripts.find(GetString(*iter));
          if (offset != TamilSuperscripts.npos) {
            tokenUnit.leadToken = start.clone(static_cast<uint8_t>(start.idx + offset / "²"_len));
            iter++;
          } else if ((offset = TamilSubscripts.find(GetString(*iter))) != TamilSubscripts.npos) {
            tokenUnit.leadToken = start.clone(static_cast<uint8_t>(start.idx + offset / "²"_len));
            iter++;
          } else {
            break;
          }
        } else {
          break;
        }
      }
    done:
      if (options * TranslitOptions::TamilSuperscripted) {
        return tokenUnit;
      }

      // lookup before it is modified
      prefixLookup.lookup(tokenUnit, prefixLookupState);

      // Thus க is pronounced ka when it is the initial letter of a word, k when it
      // is muted (க்), kka when it is geminated (க்க), ka when it follows any other
      // muted hard consonant (such as ட் or ற்), ga when it follows a muted soft
      // consonant (as in the frequently occurring cluster ங்க, which is pronounced
      // ṅga) or a muted medial consonant (such as ய் or ர்), and ha when it follows
      // a verb. Likewise ச is pronounced ca (or arbitrarily sa, as in fact it is
      // customarily pronounced in many if not most cases, though strictly speaking
      // this contravenes the ancient rule described here) when it is the initial letter
      // of a word, c when it is muted (ச்), cca when it is geminated (ச்ச), ca when
      // it follows any other muted hard consonant, ja when it follows a muted soft
      // consonant (as in the frequently occurring cluster ஞ்ச, which is
      // pronounced ñja), and sa when it follows a verb. ட is not the initial letter of
      // any word of Tamil origin, but it is pronounced ṭ when it is muted (ட்), ṭṭa
      // when it is geminated (ட்ட), and ḍa when it follows either a muted soft
      // consonant (as in the frequently occurring cluster ண் ட, which is
      // pronounced ṇḍa) or a verb. த is pronounced ta when it is the initial letter
      // of a word, t when it is muted (த்), tta when it is geminated (த்த), and da
      // when it follows either a muted soft consonant (as in the frequently
      // occurring cluster ந்த, which is pronounced nda), a muted medial
      // consonant or a verb. ப is pronounced pa when it is the initial letter of a
      // word, p when it is muted (ப்), ppa when it is geminated (ப்ப), pa when it
      // follows any other muted hard consonant, and ba when it follows either a
      // muted soft consonant (as in the frequently occurring cluster ம்ப, which is
      // pronounced mba, or in the clusters ண் ப and ன்ப, which are pronounced
      // respectively ṇba and ṉba) or a verb. The final hard consonant, ற, also has
      // several allophones or variant forms of pronunciation. Like ட (ṭa), it is
      // never the initial letter of a word. Its default pronunciation is considered to
      // be ṟa (in which ṟ is a trilled ‘r’, described technically as an alveolar trill),
      // but its mute form (ற்) is pronounced ṯ (or sometimes slightly more like ṟ,
      // depending upon which consonant it precedes, and when it is used in the
      // transliteration of a word of Sanskrit origin, it can also be pronounced d or
      // l). Its geminated form (ற்ற) is pronounced ṯṟa, and the cluster ன்ற is
      // pronounced ṉḏṟa, the extra ḏ sound being a natural euphonic increment.
      if (hasVirama) {
        // ignore virama if end of word
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
      while (iter < tokenUnits.end() && HoldsScriptToken(*iter)) {
        const auto nextToken = GetScriptToken(*iter);
        if (nextToken.tokenType == TokenType::OtherDiacritic) {
          tokenUnit.otherDiacritic = nextToken;
        } else if (nextToken.tokenType == TokenType::Accent) {
          tokenUnit.accent = nextToken;
        } else {
          break;
        }
        iter++;
      }
    }
    if (tokenUnit.leadToken.tokenType != TokenType::Consonant) {
      prefixLookup.lookup(tokenUnit, prefixLookupState);
    }
    return tokenUnit;
  }

  TokenUnit readLatinTokenUnit(const ScriptToken& start) noexcept {
    TokenUnit tokenUnit = { start };
    if (start.tokenType == TokenType::Consonant) {
      bool vowelAdded = false;

      while (iter < tokenUnits.end() && HoldsScriptToken(*iter)) {
        auto nextToken = GetScriptToken(*iter);
        bool consume = false;
        switch (nextToken.tokenType) {
          case TokenType::Vowel:
          case TokenType::VowelMark:
            if (!vowelAdded) {
              vowelAdded = true;
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
        iter++;
      }

      if (!vowelAdded) {
        tokenUnit.vowelMark = { TokenType::VowelMark, 0 };
      }
    }
    return tokenUnit;
  }

  TokenUnit inferGurmukhiAdhak() {
    if (!hasMore())
      return invalidTokenUnit;
    const auto peek = *iter;
    if (HoldsScriptToken(peek) && GetScriptToken(peek).tokenType == TokenType::Consonant) {
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

  bool isEndOfWord() const noexcept {
    if (iter >= tokenUnits.end()) {
      return true;
    }
    const auto& next = *iter;
    if (HoldsString(next)) {
      return true;
    }
    const auto token = GetScriptToken(next);
    return token.tokenType == TokenType::Symbol;
  }

  inline bool isDevanagariExtended(int ch) { return ch >= 0xA8E0 && ch <= 0xA8FF; }

  inline bool isVedicExtension(int ch) { return ch >= 0x1CD0 && ch <= 0x1CFA; }

  inline size_t remaining() noexcept { return tokenUnits.end() - iter; }

  std::optional<TokenOrString> peekNext(size_t offset = 0) noexcept {
    if (remaining() <= offset) {
      return std::nullopt;
    }
    return *(iter + offset);
  }

  bool isNextSpace(size_t offset = 0) noexcept {
    auto next = peekNext(offset);
    if (next == std::nullopt) {
      return false;
    }
    if (!HoldsString(*iter)) {
      return false;
    }
    auto str = GetString(*iter);
    for (auto c : str) {
      if (!std::isspace(c)) {
        return false;
      }
    }
    return true;
  }

private:
  const TranslitOptions options;
  std::vector<TokenOrString> tokenUnits;
  std::vector<TokenOrString>::iterator iter;
  bool wordStart { true };
  TamilPrefixLookup prefixLookup;
  TamilPrefixLookup::LookupState prefixLookupState { };
};

class OutputWriter {
public:
  virtual ~OutputWriter() = default;

  void writeTokenUnit(const TokenUnitOrString& tokenUnitOrString, const TokenUnitOrString& next) noexcept {
    if (HoldsString(tokenUnitOrString)) {
      push(GetString(tokenUnitOrString));
      wordStart = true;
    } else {
      auto tokenUnit = GetTokenUnit(tokenUnitOrString);
      if (tokenUnit.leadToken.tokenType == TokenType::Ignore) {
        return;
      }
      switch (map.getType()) {
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

  Utf8StringBuilder& text() noexcept { return buffer; }

  OutputWriter(const ScriptWriterMap& map, const TranslitOptions options, size_t inputSize) noexcept
      : map(map)
      , options(options) {
    buffer.reserve(inputSize);
    setNasalConsonantSize();
  }

protected:
  inline void push(const std::string_view& text) {
    if (options / TranslitOptions::RetainSpecialMarkers) {
      stripChars(text, SpecialMarkers, buffer);
    } else {
      buffer += text;
    }
  }

protected:
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

  void writeTamilTokenUnit(const TokenUnit& tokenUnit, const TokenUnitOrString& next) noexcept {
    bool endOfPrefix = prefixLookupState.value != std::nullopt;
    if (wordStart) {
      prefixLookupState.reset();
    }
    prefixLookup.lookup(tokenUnit, prefixLookupState);

    auto leadIdx = tokenUnit.leadToken.idx;
    auto leadText = map.lookupChar(tokenUnit.leadToken);

    if (tokenUnit.leadToken.tokenType == TokenType::Consonant) {
      // The consonant “ந்” will come in the middle of the words only.
      // The consonant “ண்” and the consonant “ன்” will come at the middle and at the end of words
      // When the consonant “ண்” becomes a uyir meiy it will not come in the beginning of any word
      // When the consonant “ந்” becomes a uyir meiy it will only come at the beginning of the word. It will not come at
      // the end of a any word When the consonant “ன்” becomes a uyir meiy it will not come in the beginning of any word
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
        std::string lastCharStr { lastChar };
        buffer.pop_back();
        push(map.lookupChar(tokenUnit.accent));
        push(lastCharStr);
      } else {
        push(map.lookupChar(tokenUnit.accent));
      }
    }
  }

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

  void inferAnuswara(const TokenUnitOrString& next) noexcept {
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

  template <typename BufType>
  void stripChars(const std::string_view& in, const std::string_view& exclude, BufType& out) noexcept {
    const char* ptr = in.data();
    const char* end = ptr + in.length();
    std::string_view ch;
    while (ptr < end && (ch = UtfUtils::nextUtf8Char(ptr)) != "") {
      if (exclude.find(ch) == exclude.npos) {
        out += ch;
      }
      ptr += ch.length();
    }
  }

  void setNasalConsonantSize() noexcept {
    const auto& anuswara = map.lookupChar(TokenType::OtherDiacritic, Diacritic_Anuswara);
    if (options / TranslitOptions::RetainSpecialMarkers) {
      std::string out { };
      stripChars(anuswara, SpecialMarkers, out);
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
  Utf8StringBuilder buffer { };
  bool wordStart { true };
  TamilPrefixLookup prefixLookup;
  TamilPrefixLookup::LookupState prefixLookupState { };
};

bool transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, TranslitBuffer& output, const std::string_view& skipStart,
    const std::string_view& skipEnd) noexcept {
  if (from == to) {
    return false;
  }
  const auto readerMap = getScriptReaderMap(from);
  if (readerMap == nullptr) {
    return false;
  }
  InputReader reader(input, *readerMap, options, skipStart, skipEnd);

  const auto writerMap = getScriptWriterMap(to);
  if (writerMap == nullptr) {
    return false;
  }
  if (writerMap->getType() == ScriptType::Indic && !writerMap->isVedic()) {
    options = options | TranslitOptions::IgnoreVedicAccents;
  }
  OutputWriter writer(*writerMap, options, input.length() + 1);

  TokenUnitOrString curr = (reader.hasMore() ? reader.getNext() : endOfText);
  while (curr != endOfText) {
    TokenUnitOrString next = (reader.hasMore() ? reader.getNext() : endOfText);
    writer.writeTokenUnit(curr, next);
    curr = next;
  }

  output.reset(writer.text().release());

  return true;
}

std::string transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, const std::string_view& skipStart, const std::string_view& skipEnd) noexcept {
  TranslitBuffer output;
  if (!transliterate(input, from, to, options, output, skipStart, skipEnd)) {
    return std::string();
  }

  return output.get();
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

/// returns a comma-separated list of scripts
int CALL_CONV isScriptSupported(const char* script) { return inditrans::static_data::findScript(script) >= 0; }

/// releaseBuffer
void CALL_CONV releaseBuffer(char* buffer) { std::free(buffer); }

} // extern "C"
