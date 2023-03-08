#pragma once

#include "scripts-gen.h"
#include <vector>

enum SpecialIndices : uint8_t {
  Virama = 0,
  Anunasika = 0,
  Anuswara = 1,
  Visarga = 2,
  AyudaEzhuttu = 3,
  Nukta = 4,
  Om = 10,
  ZeroWidthSpace = 16,
  ZeroWidthNonJoiner = 17,
  ZeroWidthJoiner = 18,
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

std::string_view tokenTypeStr(TokenType type) noexcept {
  constexpr std::string_view str[] = {
    "Vowel",
    "VowelDiacritic",
    "Consonant",
    "ConsonantDiacritic",
    "Symbol",
    "Accent",
    "ToggleTrans",
  };
  return str[static_cast<size_t>(type)];
}

struct AliasEntry {
  TokenType tokenType;
  ScriptType scriptType;
  uint8_t idx;
  const std::vector<std::string_view> alts;
};

// clang-format off
static const AliasEntry PositionalAliases[] {
  { TokenType::Consonant,          ScriptType::Brahmi, 37 /* क़ */,     { "क़" } },
  { TokenType::Consonant,          ScriptType::Brahmi, 38 /* ख़ */,     { "ख़" }},
  { TokenType::Consonant,          ScriptType::Brahmi, 39 /* ग़ */,      { "ग़" }},
  { TokenType::Consonant,          ScriptType::Brahmi, 40 /* ज़ */,     { "ज़" }},
  { TokenType::Consonant,          ScriptType::Brahmi, 41 /* ड़ */,     { "ड़" }},
  { TokenType::Consonant,          ScriptType::Brahmi, 42 /* ढ़ */,      { "ढ़" }},
  { TokenType::Consonant,          ScriptType::Brahmi, 43 /* फ़ */,     { "फ़"}},
  { TokenType::Consonant,          ScriptType::Brahmi, 44 /* य़ */,      { "य़" }},
  { TokenType::VowelDiacritic,     ScriptType::Brahmi,  0 /* ् */,      { "᳭", "്‍" /*virama + ZWJ*/, "‍್" /*ZWJ+virama*/, "්‍" /*virama + ZWJ*/, "्‍" /*virama + ZWJ*/ }},
  { TokenType::VowelDiacritic,     ScriptType::Tamil,  13 /* ொ */,   { "ொ" }},
  { TokenType::VowelDiacritic,     ScriptType::Tamil,  14 /* ோ */,   { "ோ" }},
  { TokenType::VowelDiacritic,     ScriptType::Tamil,  15 /* ௌ */,  { "ௌ" }},
  { TokenType::Accent,             ScriptType::Brahmi,  1 /*꠰  ॑ */,      { "◌॑" }},
  { TokenType::Accent,             ScriptType::Roman,   0 /*꠰  ॒ */,      { "̱", "↓", "\\_", "\\`", "'", "`" }},
  { TokenType::Accent,             ScriptType::Roman,   1 /*꠰  ॑ */,      { "̍", "↑", "\\'", "\\’", "̭" }},
  { TokenType::Accent,             ScriptType::Roman,   2 /*꠰  ᳚ */,      { "̎", "↑↑", "\\\"", "\\''", "\\’’" }},
  { TokenType::ConsonantDiacritic, ScriptType::Tamil,   0 /* ँ */,      { "ம்ˮ" }},
  { TokenType::ConsonantDiacritic, ScriptType::Tamil,   1 /* ं */,      { "ம்ʼ" }},
  { TokenType::ConsonantDiacritic, ScriptType::Roman,   0 /* ँ */,      { "~", "m̐", "ṁ" }},
  { TokenType::ConsonantDiacritic, ScriptType::Roman,   1 /* ं */,      { "ṃ", "ṃ" }},
  { TokenType::ConsonantDiacritic, ScriptType::Roman,   2 /* ः */,      { "ḥ", "ḥ" }},
  { TokenType::Symbol,             ScriptType::Brahmi, 10 /* ॐ */,     { "ओ३म्" }},
  { TokenType::Symbol,             ScriptType::Roman,  10 /* ॐ */,     { "o3m", "OM", "AUM", "oṃ", "ŏṃ" }},
  { TokenType::Symbol,             ScriptType::Roman,  12 /* ꠰ */,       { "|", "." }},
  { TokenType::Symbol,             ScriptType::Roman,  13 /* ॥ */,      { "||", "꠰꠰", ".." }},
  { TokenType::Symbol,             ScriptType::Roman,  14 /* ꣳ */,      { "gͫ", "\\m+", "{\\m+}", "\\м+" }},
  { TokenType::Symbol,             ScriptType::Roman,  15 /* ꣴ */,      { "gͫ̄", "\\m++", "\\м++" }},
  { TokenType::Symbol,             ScriptType::Roman,  17 /* \u200C */, { "{}", "^^" }},
  { TokenType::Symbol,             ScriptType::Roman,  18 /* \u200D */, { "()", "^" }},
};

constexpr std::array<std::string_view, 21> Accents      = { /*꠰*/ "॒", "॑", "᳚", "᳡", "꣡", "꣢", "꣣", "꣤", "꣥", "꣦", "꣧", "꣨", "꣩", "꣪", "꣫", "꣬", "꣭", "꣮", "꣯", "꣰", "꣱" };
constexpr std::array<std::string_view, 21> RomanAccents = { /*꠰*/ "̱", "̍", "̎", "᳡", "꣡", "꣢", "꣣", "꣤", "꣥", "꣦", "꣧", "꣨", "꣩", "꣪", "꣫", "꣬", "꣭", "꣮", "꣯", "꣰", "꣱" };

constexpr std::string_view TamilSuperscripts = "¹²³⁴";
constexpr std::string_view SkipTrans { "##" };
// clang-format on

constexpr std::string_view TamilSpecialChars { "ʼˮˇ꞉ஃ·" };
constexpr std::string_view QuotedMarkers { "ʼˮ" };