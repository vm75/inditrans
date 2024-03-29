#pragma once

#include <cassert>
#include <limits>
#include <map>
#include <optional>
#include <string_view>
#include <vector>

using namespace std::literals::string_view_literals;

enum class ScriptType : uint8_t { Indic, Tamil, Latin, Others };

bool inline isIndicScript(ScriptType script) noexcept {
  return script == ScriptType::Indic || script == ScriptType::Tamil;
}

enum class TokenType : uint8_t {
  Vowel,
  VowelMark,
  Consonant,
  OtherDiacritic,
  Accent,
  Symbol,
  VedicSymbol,
  ExclusiveSymbol,
  Ignore
};

constexpr auto InvalidToken = std::numeric_limits<uint8_t>::max();

struct Token {
  TokenType tokenType;
  uint8_t idx;

  constexpr Token(TokenType type = TokenType::Ignore, uint8_t idx = InvalidToken) noexcept
      : tokenType(type)
      , idx(idx) { }

  bool operator<(const Token& other) const noexcept { return tokenType < other.tokenType || idx < other.idx; }
  bool operator==(const Token& other) const noexcept { return tokenType == other.tokenType && idx == other.idx; }
  bool operator!=(const Token& other) const noexcept { return tokenType != other.tokenType || idx != other.idx; }
};

struct ScriptToken : public Token {
  ScriptType scriptType;
  uint8_t extra { 0xFF };

  constexpr ScriptToken(TokenType tokenType, uint8_t idx, ScriptType scriptType) noexcept
      : Token(tokenType, idx)
      , scriptType(scriptType) { }

  bool operator==(const Token& other) const noexcept { return tokenType == other.tokenType && idx == other.idx; }
  bool operator!=(const Token& other) const noexcept { return tokenType != other.tokenType || idx != other.idx; }
  bool operator==(const ScriptToken& other) const noexcept {
    return tokenType == other.tokenType && scriptType == other.scriptType && idx == other.idx;
  }
  bool operator!=(const ScriptToken& other) const noexcept {
    return tokenType != other.tokenType || scriptType != other.scriptType || idx != other.idx;
  }

  ScriptToken clone(uint8_t newIdx) const noexcept { return { tokenType, newIdx, scriptType }; }
  ScriptToken clone(ScriptType newScriptType) const noexcept { return { tokenType, idx, newScriptType }; }
};

constexpr const ScriptToken invalidScriptToken(TokenType::Ignore, InvalidToken, ScriptType::Others);

struct ScriptInfo {
  bool isVedic { false };
  ScriptType type {};
  std::vector<std::string_view> vowels {};
  std::vector<std::string_view> vowelMarks {};
  std::vector<std::string_view> consonants {};
  std::vector<std::string_view> otherDiacritics {};
  std::vector<std::string_view> symbols {};
  std::vector<std::string_view> vedicSymbols {};
  std::vector<std::string_view> aliases {};
  std::map<std::string_view, std::vector<std::string_view>> equivalents {};
  std::map<std::string_view, std::vector<std::string_view>> languages {};
};
