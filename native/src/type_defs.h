#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>

using namespace std::literals::string_view_literals;

enum class ScriptType : char { Indic = 'i', Tamil = 't', Latin = 'l', Others = 'o' };
static_assert(sizeof(ScriptType) == sizeof(char));

bool inline isIndicScript(ScriptType script) noexcept {
  return script == ScriptType::Indic || script == ScriptType::Tamil;
}

using ScriptStringRef = uint16_t;

struct ScriptMetadataRange {
  uint16_t offset;
  uint16_t count;
};

struct ScriptMetadataGroup {
  ScriptStringRef key;
  ScriptMetadataRange values;
};

inline constexpr uint8_t ScriptMetadataFlagVedic = 1;

using ScriptVowels = std::array<ScriptStringRef, 19>;
using ScriptVowelMarks = std::array<ScriptStringRef, 19>;
using ScriptConsonants = std::array<ScriptStringRef, 50>;
using ScriptOtherDiacritics = std::array<ScriptStringRef, 4>;
using ScriptSymbols = std::array<ScriptStringRef, 13>;
using ScriptVedicSymbols = std::array<ScriptStringRef, 3>;

struct ScriptCharacterData {
  ScriptVowels vowels;
  ScriptVowelMarks vowelMarks;
  ScriptConsonants consonants;
  ScriptOtherDiacritics otherDiacritics;
  ScriptSymbols symbols;
  ScriptVedicSymbols vedicSymbols;
};

struct ScriptMetadataRecord {
  ScriptStringRef name;
  ScriptType type;
  uint8_t flags;
  ScriptCharacterData characters;
  ScriptMetadataRange aliases;
  ScriptMetadataRange equivalents;
  ScriptMetadataRange languages;
};

static_assert(sizeof(ScriptMetadataRange) == 4);
static_assert(sizeof(ScriptMetadataGroup) == 6);
static_assert(sizeof(ScriptCharacterData) == 216);
static_assert(sizeof(ScriptMetadataRecord) == 232);

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

