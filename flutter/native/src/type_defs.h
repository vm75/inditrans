#pragma once

#include <cassert>
#include <limits>
#include <map>
#include <optional>
#include <string_view>
#include <vector>

using namespace std::literals::string_view_literals;

enum class ScriptType : uint8_t { Indic, Tamil, Latin, Others };

enum SpecialIndices : uint8_t {
  Virama = 0,
  Anunasika = 0,
  Anuswara = 1,
  Visarga = 2,
  AyudaEzhuttu = 3,
  Nukta = 4,
  Om = 10,
  InWordSymbolStart = 14,
  ZeroWidthSymbolStart = 16,
  // clang-format off
  // क = 0, ख = 1, ग = 2, घ = 3, ङ = 4,
  // च = 5, छ = 6, ज = 7, झ = 8, ञ = 9,
  // ट = 10, ठ = 11, ड = 12, ढ = 13, ण = 14,
  // त = 15, थ = 16, द = 17, ध = 18, न = 19,
  // प = 20, फ = 21, ब = 22, भ = 23, म = 24,
  // य = 25, र = 26, ल = 27, व = 28, श = 29, ष = 30, स = 31, ह = 32,
  // ळ = 33, ழ = 34, ற = 35, ன = 36,
  // क़ = 37, ख़ = 38, ग़ = 39, ज़ = 40, ड़ = 41, ढ़ = 42, फ़ = 43, य़ = 44,
  // ඟ = 45, ඦ = 46, ඬ = 47, ඳ = 48, ඹ = 49,
  // க = 0, ச = 5, ஜ = 7, ட = 10, த = 15, ந = 19, ப = 20, ஸ = 31,
  // clang-format on
};

enum class TokenType : uint8_t {
  Vowel,
  VowelDiacritic,
  Consonant,
  ConsonantDiacritic,
  Symbol,
  Accent,
  ToggleTrans,
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

  constexpr ScriptToken(TokenType tokenType, uint8_t idx, ScriptType scriptType) noexcept
      : Token(tokenType, idx)
      , scriptType(scriptType) { }

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

struct AliasEntry {
  const TokenType tokenType;
  const ScriptType scriptType;
  const uint8_t idx;
  const std::string_view alt;

  constexpr AliasEntry(TokenType tokenType, ScriptType scriptType, uint8_t idx, std::string_view alt) noexcept
      : tokenType(tokenType)
      , scriptType(scriptType)
      , idx(idx)
      , alt(alt) { }
};

struct ScriptInfo {
  ScriptType type {};
  std::vector<std::string_view> vowels {};
  std::vector<std::string_view> vowelDiacritics {};
  std::vector<std::string_view> consonants {};
  std::vector<std::string_view> consonantDiacritic {};
  std::vector<std::string_view> symbols {};
  std::vector<AliasEntry> alts {};
};
