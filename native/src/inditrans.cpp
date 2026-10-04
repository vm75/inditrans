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
      : ptr(input.data()), end(input.data() + input.length()), map(map), skipStart(skipStart), skipEnd(skipEnd), options(options) {
    if (map.nonRoman) {
      if (map.source)
        policyFn = &InputReader::pull<ReaderPolicy::Explicit>;
      else
        policyFn = &InputReader::pull<ReaderPolicy::Indic>;
    } else {
      if (map.folded)
        policyFn = &InputReader::pull<ReaderPolicy::FoldedRoman>;
      else
        policyFn = &InputReader::pull<ReaderPolicy::Roman>;
    }
  }

private:
  const char* ptr;
  const char* end;
  const ScriptReaderMap& map;
  const std::string_view skipStart;
  const std::string_view skipEnd;
  const TranslitOptions options;
  std::vector<TokenOrString> buffer;
  size_t head = 0;
  void (InputReader::*policyFn)();

  void ensure(size_t requiredSize) {
    if (buffer.size() - head >= requiredSize) return;
    if (head > 16) {
      buffer.erase(buffer.begin(), buffer.begin() + head);
      head = 0;
    }
    while (buffer.size() - head < requiredSize && ptr < end) {
      (this->*policyFn)();
    }
  }

  template <ReaderPolicy Policy>
  void pull() noexcept {
    auto match = map.lookupToken<Policy>(ptr, end);
    if (match.sequence) {
      const auto tokens = match.tokens();
      if (tokens.front().tokenType != TokenType::Accent || options / TranslitOptions::IgnoreVedicAccents) {
        buffer.emplace_back(tokens.front());
        for (size_t i = 1; i < tokens.size(); ++i)
          buffer.emplace_back(tokens[i]);
      }
      ptr += match.matchLen;
    } else {
      ptr = pullUnrecognized<Policy>(ptr, end, map, skipStart, skipEnd);
    }
  }

  template <ReaderPolicy Policy>
  [[gnu::noinline]] const char* pullUnrecognized(const char* ptr, const char* end, const ScriptReaderMap& map,
      const std::string_view& skipStart, const std::string_view& skipEnd) noexcept {
    const bool skipXml = !(options * TranslitOptions::NoXMLTagHandling);
    const auto* start = ptr;
    if (skipXml && *ptr == '<') {
      const auto close = std::string_view(ptr, end - ptr).find('>');
      ptr = close == std::string_view::npos ? end : ptr + close + 1;
      buffer.emplace_back(std::string_view(start, ptr - start));
    } else if (*ptr == skipStart[0] && ptr + skipStart.length() - 1 < end
        && std::string_view(ptr, skipStart.length()) == skipStart) {
      ptr += skipStart.length();
      start = ptr;
      const auto close = skipEnd.empty() ? std::string_view::npos : std::string_view(ptr, end - ptr).find(skipEnd);
      if (close == std::string_view::npos) {
        ptr = end;
      } else {
        buffer.emplace_back(std::string_view(start, close));
        ptr += close + skipEnd.length();
      }
    } else {
      ptr++;
      while (ptr < end && *ptr != skipStart[0] && *ptr != '<' && map.lookupToken<Policy>(ptr, end).sequence == 0) {
        ptr++;
      }
      buffer.emplace_back(std::string_view(start, ptr - start));
    }
    return ptr;
  }

public:
  inline bool hasMore() noexcept { 
    ensure(1);
    return head < buffer.size(); 
  }

  TokenUnit lastToken = invalidTokenUnit;
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
  TokenUnit readIndicTokenUnit(const ScriptToken& start) noexcept {
    TokenUnit tokenUnit = { start };
    if (start.tokenType == TokenType::Consonant) {
      while (true) {
        ensure(1);
        if (head >= buffer.size()) break;
        if (!HoldsScriptToken(buffer[head])) break;
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
        if (head >= buffer.size()) break;
        if (!HoldsScriptToken(buffer[head])) break;
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

      while (true) {
        ensure(1);
        if (head >= buffer.size()) break;
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
        return tokenUnit;
      }

      // lookup before it is modified
      prefixLookup.lookup(tokenUnit, prefixLookupState);

      // ... existing Tamil logic ...
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
      while (true) {
        ensure(1);
        if (head >= buffer.size()) break;
        if (!HoldsScriptToken(buffer[head])) break;
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

  TokenUnit readLatinTokenUnit(const ScriptToken& start) noexcept {
    TokenUnit tokenUnit = { start };
    if (start.tokenType == TokenType::Consonant) {
      bool vowelAdded = false;

      while (true) {
        ensure(1);
        if (head >= buffer.size()) break;
        if (!HoldsScriptToken(buffer[head])) break;
        auto nextToken = GetScriptToken(buffer[head]);
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
        head++;
      }

      if (!vowelAdded) {
        tokenUnit.vowelMark = { TokenType::VowelMark, 0 };
      }
    }
    return tokenUnit;
  }

  TokenUnit inferGurmukhiAdhak() {
    ensure(1);
    if (head >= buffer.size())
      return invalidTokenUnit;
    const auto peek = buffer[head];
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

  bool isEndOfWord() noexcept {
    ensure(1);
    if (head >= buffer.size()) {
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
    if (head + offset >= buffer.size()) {
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

template <typename Sink>
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


  OutputWriter(const ScriptWriterMap& map, const TranslitOptions options, Sink& sink) noexcept
      : map(map)
      , options(options)
      , buffer(sink) {
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
  Sink& buffer;
  bool wordStart { true };
  TamilPrefixLookup prefixLookup;
  TamilPrefixLookup::LookupState prefixLookupState { };
};

template <typename Sink>
bool transliterate_core(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, Sink& sink, const std::string_view& skipStart,
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
  OutputWriter<Sink> writer(*writerMap, options, sink);

  TokenUnitOrString curr = (reader.hasMore() ? reader.getNext() : endOfText);
  while (curr != endOfText) {
    TokenUnitOrString next = (reader.hasMore() ? reader.getNext() : endOfText);
    writer.writeTokenUnit(curr, next);
    curr = next;
  }

  return true;
}

bool transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, TranslitBuffer& output, const std::string_view& skipStart,
    const std::string_view& skipEnd) noexcept {
  Utf8StringBuilder sink;
  sink.reserve(input.length() + 1);
  if (!transliterate_core(input, from, to, options, sink, skipStart, skipEnd)) {
    return false;
  }
  output.reset(sink.release());
  return true;
}

struct StdStringSink {
  std::string& str;
  StdStringSink(std::string& s) : str(s) {}
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

bool transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, std::string& output, const std::string_view& skipStart,
    const std::string_view& skipEnd) noexcept {
  output.reserve(input.length() + 1);
  StdStringSink sink(output);
  return transliterate_core(input, from, to, options, sink, skipStart, skipEnd);
}

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

/// returns a comma-separated list of scripts
int CALL_CONV isScriptSupported(const char* script) { return inditrans::static_data::findScript(script) >= 0; }

/// releaseBuffer
void CALL_CONV releaseBuffer(char* buffer) { std::free(buffer); }

} // extern "C"
