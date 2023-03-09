#include "inditrans.h"
#include "type_defs.h"
#include "utilities.h"
#include "wasi_fix.h"
#include "tamil_prefix.h"
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

using InditransLogger = void(const std::string&);
InditransLogger* inditransLogger = nullptr;

inline constexpr bool operator*(const TranslitOptions& mask, const TranslitOptions& val) noexcept {
  return (mask & val) == val;
}
inline constexpr bool operator/(const TranslitOptions& mask, const TranslitOptions& val) noexcept {
  return (mask & val) != val;
}

TranslitOptions getTranslitOptions(const std::string_view& optStr) noexcept {
  Utf8String str(optStr);
  static constexpr std::array<std::string_view, static_cast<size_t>(MaxTranslitOptions)> optionStrings
      = { "None", "IgnoreVedicAccents", "IgnoreQuotedMarkers", "TamilTraditional", "TamilSuperscripted",
          "RetainZeroWidthChars", "ASCIINumerals" };

  TranslitOptions mask { TranslitOptions::None };
  for (const auto& opt : str.split(" \t,:;/|&")) {
    auto match = std::find(optionStrings.begin(), optionStrings.end(), opt);
    if (match != optionStrings.end()) {
      const auto idx = std::distance(optionStrings.begin(), match);
      if (idx > 0) {
        mask = mask | static_cast<TranslitOptions>(1 << (idx - 1));
      }
    }
  }

  return mask;
}

static bool scriptIsReadable(const std::string_view name) noexcept {
  constexpr std::array<std::string_view, 3> notReadable { "readablelatin", "romanreadable", "romancolloquial" };
  return std::find(notReadable.begin(), notReadable.end(), name) == notReadable.end();
}

static bool scriptIsIndic(const std::string_view name) noexcept {
  constexpr std::array<std::string_view, 11> notReadable { "assamese", "bengali", "devanagari", "gujarati", "gurmukhi",
    "kannada", "malayalam", "oriya", "sinhala", "tamil", "telugu" };
  return std::find(notReadable.begin(), notReadable.end(), name) == notReadable.end();
}

constexpr auto InvalidToken = std::numeric_limits<uint8_t>::max();

struct Token {
  TokenType tokenType;
  uint8_t idx;

  Token(TokenType tokenType = TokenType::Ignore, uint8_t idx = InvalidToken)
      : tokenType(tokenType)
      , idx(idx) { }

  bool operator==(const Token& other) const noexcept { return tokenType == other.tokenType && idx == other.idx; }
  bool operator!=(const Token& other) const noexcept { return tokenType != other.tokenType || idx != other.idx; }
};

struct ScriptToken : public Token {
  ScriptType scriptType;

  ScriptToken(TokenType tokenType, uint8_t idx, ScriptType scriptType)
      : Token(tokenType, idx)
      , scriptType(scriptType) { }

  bool operator==(const ScriptToken& other) const noexcept {
    return tokenType == other.tokenType && scriptType == other.scriptType && idx == other.idx;
  }
  bool operator!=(const ScriptToken& other) const noexcept {
    return tokenType != other.tokenType || scriptType != other.scriptType || idx != other.idx;
  }

  ScriptToken clone(uint8_t newIdx) const noexcept { return { tokenType, newIdx, scriptType }; }
};

using LookupTable = Char32Trie<ScriptToken>;
using LookupResult = LookupTable::LookupResult;

class ScriptReaderMap {
public:
  ScriptReaderMap(const std::string_view name, const ScriptData& scriptData) noexcept
      : name(name)
      , scriptData(scriptData) {
    if (!scriptIsReadable(name)) {
      return;
    }

    addScript(name, scriptData);
    const std::array<std::string_view, 21>& accentMap = scriptData.type == ScriptType::Roman ? RomanAccents : Accents;
    addCharMap(name, TokenType::Accent, scriptData.type, accentMap.data(), accentMap.size());

    tokenMap.addLookup(SkipTrans, { TokenType::ToggleTrans, 0, scriptData.type });

    for (auto aliasEntry : PositionalAliases) {
      if (aliasEntry.scriptType != scriptData.type) {
        continue;
      }
      for (auto& alias : aliasEntry.alts) {
        tokenMap.addLookup(alias, { aliasEntry.tokenType, aliasEntry.idx, aliasEntry.scriptType });
      }
    }
  }

  void addScript(const std::string_view name, const ScriptData& scriptData) noexcept {
    if (!scriptIsReadable(name)) {
      return;
    }
    addCharMap(name, TokenType::Vowel, scriptData.type, scriptData.vowels.data(), scriptData.vowels.size());
    addCharMap(name, TokenType::VowelDiacritic, scriptData.type, scriptData.vowelDiacritics.data(),
        scriptData.vowelDiacritics.size());
    addCharMap(name, TokenType::Consonant, scriptData.type, scriptData.consonants.data(), scriptData.consonants.size());
    addCharMap(name, TokenType::ConsonantDiacritic, scriptData.type, scriptData.consonantDiacritic.data(),
        scriptData.consonantDiacritic.size());
    addCharMap(name, TokenType::Symbol, scriptData.type, scriptData.symbols.data(), scriptData.symbols.size());
  }

  template <typename CharType> inline LookupResult lookupToken(const CharType* text) const noexcept {
    return tokenMap.lookup(text);
  }

  ScriptType getType() const noexcept { return scriptData.type; }
  std::string_view getName() const noexcept { return name; }
  const ScriptData& getScriptData() const noexcept { return scriptData; }

private:
  void addCharMap(const std::string_view name, TokenType tokenType, ScriptType scriptType, const std::string_view* map,
      size_t count) noexcept {
    for (size_t idx = 0; idx < count; idx++) {
      if (scriptType == ScriptType::Roman && tokenType == TokenType::VowelDiacritic && idx > SpecialIndices::Virama) {
        continue;
      }
      auto res = tokenMap.addLookup(map[idx], { tokenType, static_cast<uint8_t>(idx), scriptType });
      if (res != std::nullopt && inditransLogger != nullptr && (res->tokenType != tokenType || res->idx != idx)) {
        std::string error = "Error adding for: " + std::string(name)
            + ", tokenType: " + std::string(tokenTypeStr(tokenType)) + ", idx: " + std::to_string(idx)
            + ", new value: " + std::string(map[idx]);
        (*inditransLogger)(error);
      }
    }
  }

private:
  const std::string_view name;
  const ScriptData& scriptData;
  LookupTable tokenMap;
};

class ScriptWriterMap {
public:
  ScriptWriterMap(const std::string_view name, const ScriptData& charMaps) noexcept
      : name(name)
      , scriptType(charMaps.type) {
    addCharMap(name, TokenType::Vowel, scriptType, charMaps.vowels.data(), charMaps.vowels.size());
    addCharMap(
        name, TokenType::VowelDiacritic, scriptType, charMaps.vowelDiacritics.data(), charMaps.vowelDiacritics.size());
    addCharMap(name, TokenType::Consonant, scriptType, charMaps.consonants.data(), charMaps.consonants.size());
    addCharMap(name, TokenType::ConsonantDiacritic, scriptType, charMaps.consonantDiacritic.data(),
        charMaps.consonantDiacritic.size());
    addCharMap(name, TokenType::Symbol, scriptType, charMaps.symbols.data(), charMaps.symbols.size());
    const std::array<std::string_view, 21>& accentMap = scriptType == ScriptType::Roman ? RomanAccents : Accents;
    addCharMap(name, TokenType::Accent, scriptType, accentMap.data(), accentMap.size());
  }

  inline std::string_view lookupChar(TokenType type, size_t idx) const noexcept {
    return charMaps[static_cast<size_t>(type)][idx];
  }
  inline std::string_view lookupChar(const Token& token) const noexcept {
    return charMaps[static_cast<size_t>(token.tokenType)][token.idx];
  }

  ScriptType getType() const noexcept { return scriptType; }
  std::string_view getName() const noexcept { return name; }

private:
  void addCharMap(const std::string_view name, TokenType tokenType, ScriptType scriptType, const std::string_view* map,
      size_t count) noexcept {
    auto& charMap = charMaps[static_cast<size_t>(tokenType)];
    charMap.reserve(count);
    for (size_t idx = 0; idx < count; idx++) {
      charMap.emplace_back(map[idx]);
    }
  }

private:
  const std::string_view name;
  ScriptType scriptType;
  std::array<std::vector<std::string_view>, 6> charMaps {};
};

using TokenOrString = std::variant<ScriptToken, std::string_view>;
template <typename T> inline bool HoldsScriptToken(const T& var) { return std::holds_alternative<ScriptToken>(var); }
template <typename T> inline ScriptToken GetScriptToken(const T& var) { return std::get<ScriptToken>(var); }

struct TokenUnit {
  TokenUnit(ScriptToken leadToken) noexcept
      : leadToken(leadToken) { }

  ScriptToken leadToken;
  Token vowelDiacritic {};
  Token consonantDiacritic {};
  Token accent {};
};

using TokenUnitOrString = std::variant<TokenUnit, std::string_view>;
template <typename T> inline bool HoldsTokenUnit(const T& var) { return std::holds_alternative<TokenUnit>(var); }
template <typename T> inline TokenUnit GetTokenUnit(const T& var) { return std::get<TokenUnit>(var); }

// Common for TokenOrString & TokenUnitOrString
template <typename T> inline bool HoldsString(const T& var) { return std::holds_alternative<std::string_view>(var); }
template <typename T> inline std::string_view GetString(const T& var) { return std::get<std::string_view>(var); }

const TokenUnitOrString endOfText("");
const TokenUnit invalidTokenUnit { ScriptToken(TokenType::Ignore, InvalidToken, ScriptType::Others) };

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

class TamilPrefixLookup {
public:
  TamilPrefixLookup(const ScriptReaderMap& map) noexcept {
    if (!tamilPrefixes.isEmpty() || map.lookupToken("அ").value == std::nullopt) {
      return;
    }
    for(const auto& prefix : TamilPrefixes) {
      std::vector<TokenUnit> tokens;
      tokens.reserve(prefix.size());
      auto ptr = prefix.data();
      auto end = ptr + prefix.length();
      while (ptr < end) {
        auto match = map.lookupToken(ptr);
        if (match.value == std::nullopt) {
          break;
        }
        tokens.emplace_back(match.value.value());
      }
      if (tokens.size() == prefix.size()) {
        tamilPrefixes.addLookup(tokens, true);
      }
    }
  }
private:
  static StatefulTrie<TokenUnit, bool> tamilPrefixes;
};

class InputReader {
public:
  InputReader(const std::string_view& input, const ScriptReaderMap& map, const TranslitOptions& options) noexcept
      : options(options), prefixLookup(map) {
    auto ptr = input.data();
    auto end = ptr + input.length();
    bool skipTrans = false;
    tokenUnits.reserve(input.size());
    while (ptr < end) {
      if (skipTrans) {
        const auto* start = ptr;
        while (ptr[0] != '#' && ptr[1] != '#' && ptr < end) {
          ptr++;
        }
        if (ptr > start) {
          tokenUnits.emplace_back(std::string_view(start, ptr - start));
          continue;
        }
      }
      auto match = map.lookupToken(ptr);
      if (match.value != std::nullopt) {
        if (match.value->tokenType == TokenType::ToggleTrans) {
          skipTrans = !skipTrans;
        } else {
          if (match.value->tokenType != TokenType::Accent || options / TranslitOptions::IgnoreVedicAccents) {
            tokenUnits.emplace_back(*match.value);
          }
        }
        ptr += match.matchLen;
      } else {
        auto start = ptr++;
        while (ptr < end && map.lookupToken(ptr).value == std::nullopt) {
          ptr++;
        }
        tokenUnits.emplace_back(std::string_view(start, ptr - start));
      }
    }
    iter = tokenUnits.begin();
  }

  inline bool hasMore() noexcept { return iter < tokenUnits.end(); }

  TokenUnit lastToken = invalidTokenUnit;
  TokenUnitOrString getNext() noexcept {
    const auto& next = *iter++;
    if (HoldsString(next)) {
      wordStart = true;
      return GetString(next);
    }

    const auto& token = GetScriptToken(next);
    TokenUnit tokenUnit = { token };
    switch (token.scriptType) {
      case ScriptType::Brahmi:
        tokenUnit = readBrahmiTokenUnit(token);
        break;
      case ScriptType::Tamil:
        tokenUnit = readTamilTokenUnit(token);
        break;
      case ScriptType::Roman:
        tokenUnit = readRomanTokenUnit(token);
        break;
      default:
        break;
    }
    wordStart = (token.tokenType == TokenType::Symbol && token.idx < SpecialIndices::ZeroWidthSpace);
    if (wordStart) {
      lastToken = invalidTokenUnit;
    } else {
      lastToken = tokenUnit;
    }
    return tokenUnit;
  }

private:
  TokenUnit readBrahmiTokenUnit(const ScriptToken& start) noexcept {
    TokenUnit tokenUnit = { start };
    if (start.tokenType == TokenType::Consonant) {
      while (iter < tokenUnits.end() && HoldsScriptToken(*iter)) {
        const auto nextToken = GetScriptToken(*iter);
        switch (nextToken.tokenType) {
          case TokenType::ConsonantDiacritic:
            tokenUnit.consonantDiacritic = nextToken;
            break;
          case TokenType::VowelDiacritic:
            tokenUnit.vowelDiacritic = nextToken;
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
          case TokenType::ConsonantDiacritic:
            tokenUnit.consonantDiacritic = nextToken;
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
    TokenUnit tokenUnit = { start };
    if (start.tokenType == TokenType::Consonant) {
      bool isPrimary = start.idx <= 20 /* ப */ && start.idx % 5 == 0;
      bool hasVirama = false;

      while (iter < tokenUnits.end()) {
        if (HoldsScriptToken(*iter)) {
          auto nextToken = GetScriptToken(*iter);
          switch (nextToken.tokenType) {
            case TokenType::ConsonantDiacritic:
              iter++;
              tokenUnit.consonantDiacritic = nextToken;
              break;
            case TokenType::VowelDiacritic:
              iter++;
              tokenUnit.vowelDiacritic = nextToken;
              hasVirama = tokenUnit.vowelDiacritic.idx == SpecialIndices::Virama;
              break;
            case TokenType::Accent:
              iter++;
              tokenUnit.accent = nextToken;
              break;
            default:
              goto done;
          }
        } else if ((isPrimary || start.idx == 7 /* ஜ */) && iter < tokenUnits.end()) {
          auto superscriptPos = TamilSuperscripts.find(GetString(*iter));
          if (superscriptPos != TamilSuperscripts.npos) {
            tokenUnit.leadToken = start.clone(static_cast<uint8_t>(start.idx + superscriptPos / "²"_len));
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
        if (wordStart) {
          const std::array<uint8_t, 6> specialVowelDiactritics = { 2, 4, 10, 13, 16, InvalidToken };
          if (tokenUnit.leadToken.idx == 5 /* ச */ && lastToken.leadToken != tokenUnit.leadToken && !isVirama(lastToken)
              && std::find(specialVowelDiactritics.begin(), specialVowelDiactritics.end(), lastToken.vowelDiacritic.idx)
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
      return tokenUnit;
    } else if (start.tokenType == TokenType::Vowel) {
      if (iter < tokenUnits.end() && HoldsScriptToken(*iter)) {
        const auto nextToken = GetScriptToken(*iter);
        if (nextToken.tokenType == TokenType::Accent) {
          tokenUnit.accent = nextToken;
          iter++;
        }
      }
    }
    return tokenUnit;
  }

  TokenUnit readRomanTokenUnit(const ScriptToken& start) noexcept {
    TokenUnit tokenUnit = { start };
    if (start.tokenType == TokenType::Consonant) {
      bool vowelAdded = false;

      while (iter < tokenUnits.end() && HoldsScriptToken(*iter)) {
        auto nextToken = GetScriptToken(*iter);
        bool consume = false;
        switch (nextToken.tokenType) {
          case TokenType::Vowel:
          case TokenType::VowelDiacritic:
            if (!vowelAdded) {
              vowelAdded = true;
              if (nextToken.idx != SpecialIndices::Virama) {
                tokenUnit.vowelDiacritic = { TokenType::VowelDiacritic, nextToken.idx };
              }
              consume = true;
            }
            break;
          case TokenType::ConsonantDiacritic:
            tokenUnit.consonantDiacritic = nextToken;
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
        tokenUnit.vowelDiacritic = { TokenType::VowelDiacritic, 0 };
      }
    }
    return tokenUnit;
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
    return token.vowelDiacritic.tokenType == TokenType::VowelDiacritic
        && token.vowelDiacritic.idx == SpecialIndices::Virama;
  }

  bool isEndOfWord() const noexcept {
    if (iter >= tokenUnits.end()) {
      return true;
    }
    const auto& next = *iter;
    if (HoldsString(next)) {
      return TamilSuperscripts.find(GetString(next)) == TamilSuperscripts.npos;
    }
    const auto token = GetScriptToken(next);
    return (token.tokenType == TokenType::Symbol && token.idx < SpecialIndices::ZeroWidthSpace)
        || token.tokenType == TokenType::ToggleTrans;
  }

private:
  const TranslitOptions options;
  std::vector<TokenOrString> tokenUnits;
  std::vector<TokenOrString>::iterator iter;
  bool wordStart { true };
  TamilPrefixLookup prefixLookup;
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
      if (tokenUnit.leadToken.tokenType == TokenType::Symbol
          && tokenUnit.leadToken.idx >= SpecialIndices::ZeroWidthSpace
          && options / TranslitOptions::RetainZeroWidthChars) {
        return;
      }
      switch (scriptType) {
        case ScriptType::Brahmi:
          writeBrahmiTokenUnit(tokenUnit);
          break;
        case ScriptType::Tamil:
          writeTamilTokenUnit(tokenUnit, next);
          break;
        case ScriptType::Roman:
          writeRomanTokenUnit(tokenUnit, next);
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
      , options(options)
      , scriptType(map.getType())
      , anuswaraMissing(map.lookupChar(TokenType::ConsonantDiacritic, SpecialIndices::Anuswara).length() == 0) {
    buffer.reserve(inputSize);
    setNasalConsonantSize();
  }

protected:
  inline void push(const std::string_view& text) {
    if (options * TranslitOptions::IgnoreQuotedMarkers) {
      stripChars(text, QuotedMarkers, buffer);
    } else {
      buffer += text;
    }
  }

protected:
  void writeBrahmiTokenUnit(const TokenUnit& tokenUnit) noexcept {
    if (options * TranslitOptions::ASCIINumerals && tokenUnit.leadToken.tokenType == TokenType::Symbol
        && tokenUnit.leadToken.idx < 10) {
      char str[2] = { static_cast<char>('0' + tokenUnit.leadToken.idx), 0 };
      push(str);
    } else {
      push(map.lookupChar(tokenUnit.leadToken));
    }
    if (tokenUnit.vowelDiacritic.idx != InvalidToken) {
      push(map.lookupChar(tokenUnit.vowelDiacritic));
    }
    if (tokenUnit.consonantDiacritic.idx != InvalidToken) {
      push(map.lookupChar(tokenUnit.consonantDiacritic));
    }
    if (tokenUnit.accent.idx != InvalidToken && options / TranslitOptions::IgnoreVedicAccents) {
      push(map.lookupChar(tokenUnit.accent));
    }
  }

  void writeTamilTokenUnit(const TokenUnit& tokenUnit, const TokenUnitOrString& next) noexcept {
    auto leadIdx = tokenUnit.leadToken.idx;
    auto leadText = map.lookupChar(tokenUnit.leadToken);

    if (tokenUnit.leadToken.tokenType == TokenType::Consonant) {
      if (leadIdx == 19 /* ந */ && !wordStart) {
        if (next == endOfText || tokenUnit.vowelDiacritic.idx != SpecialIndices::Virama) {
          leadText = "ன";
        }
      } else if (options * TranslitOptions::TamilTraditional) {
        auto repl = tamilTraditionalMap.find(leadText);
        if (repl != tamilTraditionalMap.end()) {
          leadText = repl->second;
        }
      }

      auto superscript = Utf8String::trailingChar(leadText).view();
      if (leadText.length() > superscript.length() && TamilSuperscripts.find(superscript) != TamilSuperscripts.npos) {
        leadText = { leadText.data(), leadText.length() - superscript.length() };
      } else {
        superscript = "";
      }
      push(leadText);
      auto consonantPosition = buffer.size();

      if (tokenUnit.vowelDiacritic.idx != InvalidToken) {
        push(map.lookupChar(tokenUnit.vowelDiacritic));
      }

      if (options * TranslitOptions::TamilSuperscripted && superscript != "") {
        push(superscript);
      }

      if (tokenUnit.consonantDiacritic.idx != InvalidToken) {
        if (tokenUnit.consonantDiacritic.idx == SpecialIndices::Anuswara) {
          inferAnuswara(next);
        } else {
          push(map.lookupChar(tokenUnit.consonantDiacritic));
        }
      }
    } else {
      if (tokenUnit.leadToken.tokenType == TokenType::Vowel) {
        push(leadText);

        if (tokenUnit.consonantDiacritic.idx != InvalidToken) {
          if (tokenUnit.consonantDiacritic.idx == SpecialIndices::Anuswara) {
            inferAnuswara(next);
          } else {
            push(map.lookupChar(tokenUnit.consonantDiacritic));
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

  void writeRomanTokenUnit(const TokenUnit& tokenUnit, const TokenUnitOrString& next) noexcept {
    auto& leadToken = tokenUnit.leadToken;
    push(map.lookupChar(leadToken.tokenType, leadToken.idx));
    if (leadToken.tokenType == TokenType::Consonant) {
      if (tokenUnit.vowelDiacritic.idx == InvalidToken) {
        push(map.lookupChar(TokenType::Vowel, SpecialIndices::Virama));
      } else if (tokenUnit.vowelDiacritic.idx != SpecialIndices::Virama) {
        push(map.lookupChar(tokenUnit.vowelDiacritic));
      }
    }
    if (tokenUnit.accent.idx != InvalidToken && options / TranslitOptions::IgnoreVedicAccents) {
      push(map.lookupChar(tokenUnit.accent));
    }
    if (tokenUnit.consonantDiacritic.idx != InvalidToken) {
      auto lookup = map.lookupChar(tokenUnit.consonantDiacritic);
      if (tokenUnit.consonantDiacritic.idx == SpecialIndices::Anuswara && lookup.size() == 0) {
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
    if (scriptType == ScriptType::Tamil) {
      push(map.lookupChar(TokenType::VowelDiacritic, SpecialIndices::Virama));
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
    const auto& anuswara = map.lookupChar(TokenType::ConsonantDiacritic, SpecialIndices::Anuswara);
    if (options * TranslitOptions::IgnoreQuotedMarkers) {
      std::string out {};
      stripChars(anuswara, QuotedMarkers, out);
      anuswaraSize = out.size();
    } else {
      anuswaraSize = anuswara.size();
    }
  }

private:
  const ScriptWriterMap& map;
  const TranslitOptions options;
  const ScriptType scriptType;
  Utf8StringBuilder buffer {};
  bool anuswaraMissing {};
  size_t anuswaraSize {};
  std::unordered_map<std::string_view, std::string_view> tamilTraditionalMap {
    { "ஸ", "ச" },
    { "ஜ", "ச³" },
    { "ஜ²", "ச⁴" },
  };
  bool wordStart { true };
};

static constexpr auto scriptDataMap
    = ConstexprMap<std::string_view, ScriptData, ScriptDataMap.size()> { { ScriptDataMap } };

std::unique_ptr<InputReader> getInputReader(
    const std::string_view& text, std::string_view from, TranslitOptions options) noexcept {
  static std::unordered_map<std::string, ScriptReaderMap> readerMapCache {};

  if (!scriptIsReadable(from)) {
    return nullptr;
  }

  auto entry = readerMapCache.find(std::string(from));
  if (entry == readerMapCache.end()) {
    if (from == "indic") {
      auto mapEntry = scriptDataMap.get("devanagari");
      if (mapEntry == nullptr) {
        return nullptr;
      }
      entry = readerMapCache.emplace(from, ScriptReaderMap { from, *mapEntry }).first;

      for (const auto& scriptInfo : scriptDataMap) {
        if (scriptInfo.first == "devanagari" || scriptIsIndic(scriptInfo.first)) {
          continue;
        }

        entry->second.addScript(scriptInfo.first, scriptInfo.second);
      }
    } else {
      auto mapEntry = scriptDataMap.get(from);
      if (mapEntry == nullptr) {
        return nullptr;
      }

      entry = readerMapCache.emplace(from, ScriptReaderMap { from, *mapEntry }).first;
    }
  }

  return std::make_unique<InputReader>(text, entry->second, options);
}

std::unique_ptr<OutputWriter> getOutputWriter(std::string_view to, TranslitOptions options, size_t inputSize) noexcept {
  static std::unordered_map<std::string, ScriptWriterMap> writerMapCache {};

  auto entry = writerMapCache.find(std::string(to));
  if (entry == writerMapCache.end()) {
    auto mapEntry = scriptDataMap.get(to);
    if (mapEntry == nullptr) {
      return nullptr;
    }

    entry = writerMapCache.emplace(to, ScriptWriterMap { to, *mapEntry }).first;
  }

  return std::make_unique<OutputWriter>(entry->second, options, inputSize);
}

bool transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, std::unique_ptr<char>& output) noexcept {
  if (from == to) {
    return false;
  }
  auto reader = getInputReader(input, from, options);
  if (reader == nullptr) {
    return false;
  }
  auto writer = getOutputWriter(to, options, input.length() + 1);
  if (writer == nullptr) {
    return false;
  }

  TokenUnitOrString curr = (reader->hasMore() ? reader->getNext() : endOfText);
  while (curr != endOfText) {
    TokenUnitOrString next = (reader->hasMore() ? reader->getNext() : endOfText);
    writer->writeTokenUnit(curr, next);
    curr = next;
  }

  output.reset(writer->text().release());

  return true;
}

std::string transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options) noexcept {
  std::unique_ptr<char> output;
  if (!transliterate(input, from, to, options, output)) {
    return std::string();
  }

  return output.get();
}

extern "C" {

unsigned long CALL_CONV translitOptionsToInt(const char* optionStr) {
  return static_cast<unsigned long>(getTranslitOptions(optionStr));
}

char* CALL_CONV transliterate(const char* input, const char* from, const char* to, unsigned long options) {
  std::unique_ptr<char> output;
  std::string_view inputView(input);
  if (!transliterate(inputView, from, to, static_cast<TranslitOptions>(options), output)) {
    return nullptr;
  } else {
    auto retval = output.release();
    return retval;
  }
}

char* CALL_CONV transliterate2(const char* input, const char* from, const char* to, const char* optionStr) {
  return transliterate(input, from, to, translitOptionsToInt(optionStr));
}

void CALL_CONV releaseBuffer(char* buffer) {
  if (buffer) {
    delete[] buffer;
  }
}

} // extern "C"