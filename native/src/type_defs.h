#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string_view>
#include <vector>

using namespace std::literals::string_view_literals;

enum class ScriptType : uint8_t { Indic, Tamil, Latin, Others };

constexpr bool inline isIndicScript(ScriptType script) noexcept {
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

  constexpr bool operator<(const Token& other) const noexcept { return tokenType < other.tokenType || idx < other.idx; }
  constexpr bool operator==(const Token& other) const noexcept {
    return tokenType == other.tokenType && idx == other.idx;
  }
  constexpr bool operator!=(const Token& other) const noexcept {
    return tokenType != other.tokenType || idx != other.idx;
  }
};

// Keep word-sized copies and the existing TokenUnit field layout. Expansion
// metadata lives in static spans; this byte carries no reader state.
struct ScriptToken : public Token {
  ScriptType scriptType;
  uint8_t reserved { };

  constexpr ScriptToken(
      TokenType tokenType = TokenType::Ignore, uint8_t idx = InvalidToken, ScriptType scriptType = ScriptType::Others) noexcept
      : Token(tokenType, idx)
      , scriptType(scriptType) { }

  constexpr bool operator==(const Token& other) const noexcept {
    return tokenType == other.tokenType && idx == other.idx;
  }
  constexpr bool operator!=(const Token& other) const noexcept {
    return tokenType != other.tokenType || idx != other.idx;
  }
  constexpr bool operator==(const ScriptToken& other) const noexcept {
    return tokenType == other.tokenType && scriptType == other.scriptType && idx == other.idx;
  }
  constexpr bool operator!=(const ScriptToken& other) const noexcept {
    return tokenType != other.tokenType || scriptType != other.scriptType || idx != other.idx;
  }

  constexpr ScriptToken clone(uint8_t newIdx) const noexcept { return { tokenType, newIdx, scriptType }; }
  constexpr ScriptToken clone(ScriptType newScriptType) const noexcept { return { tokenType, idx, newScriptType }; }
};

constexpr const ScriptToken invalidScriptToken(TokenType::Ignore, InvalidToken, ScriptType::Others);

struct TokenUnit {
  constexpr TokenUnit(ScriptToken leadToken) noexcept
      : leadToken(leadToken) { }
  ScriptToken leadToken;
  Token vowelMark { };
  Token otherDiacritic { };
  Token accent { };
  constexpr bool operator==(const TokenUnit& other) const noexcept {
    return leadToken == other.leadToken && vowelMark == other.vowelMark && otherDiacritic == other.otherDiacritic
        && accent == other.accent;
  }
  constexpr bool operator!=(const TokenUnit& other) const noexcept { return !(*this == other); }
};
