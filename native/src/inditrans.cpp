#include "inditrans.h"
#include "script_constants.h"
#include "script_metadata.h"
#include "type_defs.h"
#include "utilities.h"
#include <array>
#include <cctype>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

using InditransLogger = void(const std::string&);
InditransLogger* inditransLogger = nullptr;

constexpr std::string_view tamilTraditionalReplacement(std::string_view text) noexcept {
  if (text == "ஸ") return "ச";
  if (text == "ஜ") return "ச³";
  if (text == "ஜ²") return "ச⁴";
  return text;
}

inline constexpr bool operator*(const TranslitOptions& mask, const TranslitOptions& val) noexcept {
  return (mask & val) == val;
}
inline constexpr bool operator/(const TranslitOptions& mask, const TranslitOptions& val) noexcept {
  return (mask & val) != val;
}

namespace generated = inditrans::generated;
using ScriptInfo = generated::ScriptMetadataRecord;

namespace {

ScriptType getScriptType(char type) noexcept {
  switch (type) {
    case 'v':
    case 'i':
      return ScriptType::Indic;
    case 't':
      return ScriptType::Tamil;
    case 'l':
      return ScriptType::Latin;
    default:
      return ScriptType::Others;
  }
}

bool equalScriptName(std::string_view left, std::string_view right) noexcept {
  if (left.size() != right.size()) {
    return false;
  }
  for (size_t idx = 0; idx < left.size(); ++idx) {
    const auto leftChar = static_cast<unsigned char>(left[idx]);
    const auto rightChar = static_cast<unsigned char>(right[idx]);
    if (std::tolower(leftChar) != std::tolower(rightChar)) {
      return false;
    }
  }
  return true;
}

template <typename T, size_t N>
std::span<const T> metadataSlice(const std::array<T, N>& values, generated::ScriptMetadataRange range) noexcept {
  assert(range.offset <= N && range.count <= N - range.offset);
  if (range.count == 0) {
    return {};
  }
  return { values.data() + range.offset, range.count };
}

} // namespace

class ScriptMetadata {
public:
  static constexpr const auto& getScripts() noexcept {
    return generated::scriptMetadataScripts;
  }

  static const ScriptInfo* getScript(std::string_view name) noexcept {
    for (const auto& script : getScripts()) {
      if (equalScriptName(generated::stringView(script.name), name)) {
        return &script;
      }
    }

    for (const auto& script : getScripts()) {
      for (const auto aliasRef : metadataSlice(generated::scriptAliases, script.aliases)) {
        if (equalScriptName(generated::stringView(aliasRef), name)) {
          return &script;
        }
      }
    }

    return nullptr;
  }
};

using LookupTable = Char32Trie<ScriptToken>;
using LookupResult = LookupTable::LookupResult;

class ScriptReaderMap {
public:
  ScriptReaderMap(const std::string_view name, const ScriptInfo& scriptData) noexcept
      : name(name)
      , scriptData(scriptData)
      , caseInsensitive(isCaseInsensitiveScripts(name)) {
    if (isWriteOnlyScript(name)) {
      return;
    }

    addScript(name, scriptData);

    for (const auto& entry : metadataSlice(generated::scriptEquivalentGroups, scriptData.equivalents)) {
      const auto tokenStr = generated::stringView(entry.key);
      ScriptToken token = invalidScriptToken;
      if (tokenStr.length() >= 3 && tokenStr[1] == ':') {
        const auto type = getTokenType(tokenStr[0]);
        if (type == TokenType::Ignore) {
          continue;
        }

        unsigned int index = 0;
        bool validIndex = true;
        for (size_t pos = 2; pos < tokenStr.size(); ++pos) {
          const auto ch = tokenStr[pos];
          if (ch < '0' || ch > '9') {
            validIndex = false;
            break;
          }
          index = index * 10u + static_cast<unsigned int>(ch - '0');
          if (index > std::numeric_limits<uint8_t>::max()) {
            validIndex = false;
            break;
          }
        }
        if (!validIndex) {
          continue;
        }
        token = ScriptToken { type, static_cast<uint8_t>(index), getScriptType(scriptData.type) };
      } else {
        auto ptr = tokenStr.data();
        LookupResult result = lookupToken(ptr);
        if (result.value != std::nullopt) {
          std::vector<ScriptToken> extra {};
          token = *result.value;
          ptr += result.matchLen;

          while ((result = lookupToken(ptr)).value) {
            extra.push_back(*result.value);
            ptr += result.matchLen;
          }
          if (!extra.empty()) {
            token.extra = static_cast<uint8_t>(extraTokens.size());
            extraTokens.emplace_back(std::move(extra));
          }
        }
      }
      if (token == invalidScriptToken) {
        continue;
      }
      for (const auto aliasRef : metadataSlice(generated::scriptEquivalentValues, entry.values)) {
        tokenMap.addLookup(generated::stringView(aliasRef), token);
      }
    }
  }

  void addScript(const std::string_view name, const ScriptInfo& scriptData) noexcept {
    if (isWriteOnlyScript(name)) {
      return;
    }
    const auto scriptType = getScriptType(scriptData.type);
    addCharMap(name, TokenType::Vowel, scriptType, metadataSlice(generated::scriptVowels, scriptData.vowels));
    addCharMap(
        name, TokenType::VowelMark, scriptType, metadataSlice(generated::scriptVowelMarks, scriptData.vowelMarks));
    addCharMap(
        name, TokenType::Consonant, scriptType, metadataSlice(generated::scriptConsonants, scriptData.consonants));
    addCharMap(name, TokenType::OtherDiacritic, scriptType,
        metadataSlice(generated::scriptOtherDiacritics, scriptData.otherDiacritics));
    addCharMap(name, TokenType::Symbol, scriptType, metadataSlice(generated::scriptSymbols, scriptData.symbols));
    addCharMap(name, TokenType::VedicSymbol, scriptType,
        metadataSlice(generated::scriptVedicSymbols, scriptData.vedicSymbols));

    if (isIndicScript(scriptType)) {
      addCharMap(name, TokenType::Accent, scriptType, VedicAccents);
      addCharMap(name, TokenType::ExclusiveSymbol, scriptType, ExclusiveSymbols);
    } else {
      addCharMap(name, TokenType::Accent, scriptType, LatinAccents);
    }
  }

  template <typename CharType> inline LookupResult lookupToken(const CharType* text) const noexcept {
    return tokenMap.lookup(text, caseInsensitive);
  }

  const std::vector<ScriptToken>& getExtraTokens(uint8_t index) const noexcept { return extraTokens[index]; };

  ScriptType getType() const noexcept { return getScriptType(scriptData.type); }

private:
  void addCharMapEntry(const std::string_view name, TokenType tokenType, ScriptType scriptType,
      std::string_view text, size_t idx) noexcept {
    auto res = tokenMap.addLookup(text, { tokenType, static_cast<uint8_t>(idx), scriptType });
    if (res != std::nullopt && inditransLogger != nullptr && (res->tokenType != tokenType || res->idx != idx)) {
      std::string error = "Error adding for: " + std::string(name)
          + ", tokenType: " + std::to_string(static_cast<int>(tokenType)) + ", idx: " + std::to_string(idx)
          + ", new value: " + std::string(text);
      (*inditransLogger)(error);
    }
  }

  void addCharMap(const std::string_view name, TokenType tokenType, ScriptType scriptType,
      std::span<const generated::StringRef> map) noexcept {
    for (size_t idx = 0; idx < map.size(); idx++) {
      if (scriptType == ScriptType::Latin && tokenType == TokenType::VowelMark && idx != Diacritic_Virama) {
        continue;
      }
      addCharMapEntry(name, tokenType, scriptType, generated::stringView(map[idx]), idx);
    }
  }

  template <size_t N>
  void addCharMap(const std::string_view name, TokenType tokenType, ScriptType scriptType,
      const std::array<std::string_view, N>& map) noexcept {
    for (size_t idx = 0; idx < map.size(); idx++) {
      if (scriptType == ScriptType::Latin && tokenType == TokenType::VowelMark && idx != Diacritic_Virama) {
        continue;
      }
      addCharMapEntry(name, tokenType, scriptType, map[idx], idx);
    }
  }

  TokenType getTokenType(char ch) {
    switch (ch) {
      case 'v':
        return TokenType::Vowel;
      case 'm':
        return TokenType::VowelMark;
      case 'c':
        return TokenType::Consonant;
      case 'o':
        return TokenType::OtherDiacritic;
      case 's':
        return TokenType::Symbol;
      case 'S':
        return TokenType::VedicSymbol;
      case 'a':
        return TokenType::Accent;
      case 'x':
        return TokenType::ExclusiveSymbol;
      default:
        return TokenType::Ignore;
    }
  }

  const std::string_view name;
  const ScriptInfo& scriptData;
  const bool caseInsensitive;
  LookupTable tokenMap {};
  std::vector<std::vector<ScriptToken>> extraTokens {};
};

class ScriptWriterMap {
public:
  ScriptWriterMap(const std::string_view name, const ScriptInfo& scriptInfo) noexcept
      : name(name)
      , scriptInfo(scriptInfo)
      , scriptType(getScriptType(scriptInfo.type)) {
    charMaps[static_cast<size_t>(TokenType::Vowel)] = metadataSlice(generated::scriptVowels, scriptInfo.vowels);
    charMaps[static_cast<size_t>(TokenType::VowelMark)] =
        metadataSlice(generated::scriptVowelMarks, scriptInfo.vowelMarks);
    charMaps[static_cast<size_t>(TokenType::Consonant)] =
        metadataSlice(generated::scriptConsonants, scriptInfo.consonants);
    charMaps[static_cast<size_t>(TokenType::OtherDiacritic)] =
        metadataSlice(generated::scriptOtherDiacritics, scriptInfo.otherDiacritics);
    charMaps[static_cast<size_t>(TokenType::Symbol)] = metadataSlice(generated::scriptSymbols, scriptInfo.symbols);
    charMaps[static_cast<size_t>(TokenType::VedicSymbol)] =
        metadataSlice(generated::scriptVedicSymbols, scriptInfo.vedicSymbols);
  }

  inline std::string_view lookupChar(TokenType type, size_t idx) const noexcept {
    if (type == TokenType::Accent) {
      if (isIndicScript(scriptType)) {
        return idx < VedicAccents.size() ? VedicAccents[idx] : std::string_view {};
      }
      return idx < LatinAccents.size() ? LatinAccents[idx] : std::string_view {};
    }
    if (type == TokenType::ExclusiveSymbol) {
      if (!isIndicScript(scriptType) || idx >= ExclusiveSymbols.size()) {
        return {};
      }
      return ExclusiveSymbols[idx];
    }

    const auto& map = charMaps[static_cast<size_t>(type)];
    if (idx >= map.size()) {
      return {};
    }
    return generated::stringView(map[idx]);
  }

  inline std::string_view lookupChar(const Token& token) const noexcept {
    return lookupChar(token.tokenType, token.idx);
  }

  ScriptType getType() const noexcept { return scriptType; }
  bool isVedic() const noexcept {
    return (scriptInfo.flags & generated::ScriptMetadataFlagVedic) != 0;
  }

private:
  const std::string_view name;
  const ScriptInfo& scriptInfo;
  ScriptType scriptType;
  std::array<std::span<const generated::StringRef>, 8> charMaps {};
};

using TokenOrString = std::variant<ScriptToken, std::string_view>;
template <typename T> inline bool HoldsScriptToken(const T& var) { return std::holds_alternative<ScriptToken>(var); }
template <typename T> inline ScriptToken GetScriptToken(const T& var) { return std::get<ScriptToken>(var); }

struct TokenUnit {
  constexpr TokenUnit(ScriptToken leadToken) noexcept
      : leadToken(leadToken) { }

  ScriptToken leadToken;
  Token vowelMark {};
  Token otherDiacritic {};
  Token accent {};

  bool operator==(const TokenUnit& other) const noexcept {
    return leadToken == other.leadToken && vowelMark == other.vowelMark && otherDiacritic == other.otherDiacritic
        && accent == other.accent;
  }

  bool operator!=(const TokenUnit& other) const noexcept { return !(*this == other); }
};

namespace std {

template <> struct hash<TokenUnit> {
  std::size_t operator()(const TokenUnit& k) const {
    return (k.leadToken.idx ^ (k.vowelMark.idx << 1) >> 1) ^ (k.otherDiacritic.idx << 1) ^ (k.accent.idx << 1);
  }
};

}

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

const ScriptReaderMap* getScriptReaderMap(std::string_view script) noexcept {
  static std::unordered_map<std::string, ScriptReaderMap> readerMapCache {};

  if (isWriteOnlyScript(script)) {
    return nullptr;
  }

#ifdef FORCE_INDIC
  auto mapEntry = ScriptMetadata::getScript(script);
  if (mapEntry != nullptr) {
    const auto scriptType = getScriptType(mapEntry->type);
    if (scriptType == ScriptType::Indic || scriptType == ScriptType::Tamil) {
      script = "indic";
    }
  }
#endif

  auto entry = readerMapCache.find(std::string(script));
  if (entry == readerMapCache.end()) {
    if (script == "indic") {
      auto mapEntry = ScriptMetadata::getScript("devanagari");
      if (mapEntry == nullptr) {
        return nullptr;
      }
      entry = readerMapCache.emplace(script, ScriptReaderMap { script, *mapEntry }).first;

      for (const auto& scriptInfo : ScriptMetadata::getScripts()) {
        const auto scriptName = generated::stringView(scriptInfo.name);
        if (scriptName == "devanagari" || getScriptType(scriptInfo.type) == ScriptType::Latin) {
          continue;
        }

        entry->second.addScript(scriptName, scriptInfo);
      }
    } else {
      auto mapEntry = ScriptMetadata::getScript(script);
      if (mapEntry == nullptr) {
        return nullptr;
      }

      entry = readerMapCache.emplace(script, ScriptReaderMap { script, *mapEntry }).first;
    }
  }

  return &entry->second;
}

const ScriptWriterMap* getScriptWriterMap(std::string_view script) noexcept {
  static std::unordered_map<std::string, ScriptWriterMap> writerMapCache {};
  auto entry = writerMapCache.find(std::string(script));
  if (entry == writerMapCache.end()) {
    auto mapEntry = ScriptMetadata::getScript(script);
    if (mapEntry == nullptr) {
      return nullptr;
    }

    entry = writerMapCache.emplace(script, ScriptWriterMap { script, *mapEntry }).first;
  }

  return &entry->second;
}

class TamilPrefixLookup {
public:
  TamilPrefixLookup() noexcept {
    if (!tamilPrefixes.isEmpty()) {
      return;
    }
    auto map = getScriptReaderMap("tamil");
    if (map == nullptr) {
      return;
    }
    for (const auto& prefix : TamilPrefixes) {
      std::vector<TokenUnit> tokens;
      tokens.reserve(prefix.size());
      auto ptr = prefix.data();
      auto end = ptr + prefix.length();
      while (ptr < end) {
        auto match = map->lookupToken(ptr);
        if (match.value == std::nullopt) {
          break;
        }
        switch (match.value->tokenType) {
          case TokenType::Consonant:
          case TokenType::Vowel:
          case TokenType::Symbol:
          case TokenType::ExclusiveSymbol:
            tokens.emplace_back(match.value.value());
            break;
          case TokenType::OtherDiacritic:
            tokens.back().otherDiacritic = match.value.value();
            break;
          case TokenType::VowelMark:
            tokens.back().vowelMark = match.value.value();
            break;
          case TokenType::Accent:
            tokens.back().accent = match.value.value();
            break;
          default:
            break;
        }
        ptr += match.matchLen;
      }
      if (ptr == end) {
        tamilPrefixes.addLookup(tokens, true);
      }
    }
  }

  bool lookup(const TokenUnit& token, StatefulTrie<TokenUnit, bool>::LookupState& lookupState) noexcept {
    if (token.leadToken.scriptType == ScriptType::Tamil) {
      return tamilPrefixes.lookup(token, lookupState);
    }
    TokenUnit tokenCopy = token;
    tokenCopy.leadToken.scriptType = ScriptType::Tamil;
    if (tokenCopy.leadToken.tokenType == TokenType::Consonant && tokenCopy.leadToken.idx <= 24) {
      if (tokenCopy.leadToken.idx % 5 != 4) {
        tokenCopy.leadToken.idx -= tokenCopy.leadToken.idx % 5;
      }
    }
    return tamilPrefixes.lookup(tokenCopy, lookupState);
  }

private:
  static StatefulTrie<TokenUnit, bool> tamilPrefixes;
};
StatefulTrie<TokenUnit, bool> TamilPrefixLookup::tamilPrefixes {};

class InputReader {
public:
  InputReader(const std::string_view& input, const ScriptReaderMap& map, const TranslitOptions& options,
      const std::string_view& skipStart = "##", const std::string_view& skipEnd = "##") noexcept
      : options(options) {
    auto ptr = input.data();
    auto end = ptr + input.length();
    tokenUnits.reserve(input.size());

    bool skipXml = !(options * TranslitOptions::NoXMLTagHandling);

    while (ptr < end) {
      auto match = map.lookupToken(ptr);
      if (match.value != std::nullopt) {
        if (match.value->tokenType != TokenType::Accent || options / TranslitOptions::IgnoreVedicAccents) {
          auto token = *match.value;
          tokenUnits.emplace_back(token);
          if (token.extra != 0xFF) {
            for (const auto& extraToken : map.getExtraTokens(token.extra)) {
              tokenUnits.emplace_back(extraToken);
            }
          }
        }
        ptr += match.matchLen;
      } else {
        while (ptr < end) {
          const auto* start = ptr;
          if (skipXml && *ptr == '<') {
            while (ptr < end && *ptr != '>') {
              ptr++;
            }
            if (*ptr == '>') {
              ptr++;
            }
            tokenUnits.emplace_back(std::string_view(start, ptr - start));
          } else if (*ptr == skipStart[0] && ptr + skipStart.length() - 1 < end
              && std::string_view(ptr, skipStart.length()) == skipStart) {
            ptr += skipStart.length();
            start = ptr;
            while (ptr < end) {
              if (*ptr == skipEnd[0] && ptr + skipEnd.length() - 1 < end
                  && std::string_view(ptr, skipEnd.length()) == skipEnd) {
                tokenUnits.emplace_back(std::string_view(start, ptr - start));
                ptr += skipEnd.length();
                break;
              } else {
                ptr++;
              }
            }
          } else if (map.lookupToken(ptr).value != std::nullopt) {
            break;
          } else {
            ptr++;
            while (ptr < end && *ptr != skipStart[0] && *ptr != '<' && map.lookupToken(ptr).value == std::nullopt) {
              ptr++;
            }
            tokenUnits.emplace_back(std::string_view(start, ptr - start));
          }
        }
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
    const auto peek = *iter;
    if (hasMore() && HoldsScriptToken(peek) && GetScriptToken(peek).tokenType == TokenType::Consonant) {
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
  StatefulTrie<TokenUnit, bool>::LookupState prefixLookupState {};
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

      std::string_view vowelMarkReminder {};
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
      std::string out {};
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
  Utf8StringBuilder buffer {};
  bool wordStart { true };
  TamilPrefixLookup prefixLookup;
  StatefulTrie<TokenUnit, bool>::LookupState prefixLookupState {};
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
int CALL_CONV isScriptSupported(const char* script) { return ScriptMetadata::getScript(script) != nullptr; }

/// releaseBuffer
void CALL_CONV releaseBuffer(char* buffer) {
  std::free(buffer);
}

} // extern "C"
