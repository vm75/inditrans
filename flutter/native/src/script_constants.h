#pragma once

#include "type_defs.h"
#include <array>
#include <string>

constexpr std::array<std::string_view, 3> WriteOnlyScripts { "readablelatin", "romanreadable", "romancolloquial" };
auto isWriteOnlyScript = [](std::string_view script) noexcept {
  return std::find(WriteOnlyScripts.begin(), WriteOnlyScripts.end(), script) != WriteOnlyScripts.end();
};

constexpr std::array<std::string_view, 11> IndicScripts { "assamese", "bengali", "devanagari", "gujarati", "gurmukhi",
  "kannada", "malayalam", "oriya", "sinhala", "tamil", "telugu" };
auto isIndicScript = [](std::string_view script) noexcept {
  return std::find(IndicScripts.begin(), IndicScripts.end(), script) != IndicScripts.end();
};

// clang-format off
constexpr std::array<AliasEntry, 64> PositionalAliases {{
  { TokenType::Consonant,          ScriptType::Brahmi, 37 /* क़ */,      "क़" },
  { TokenType::Consonant,          ScriptType::Brahmi, 38 /* ख़ */,      "ख़" },
  { TokenType::Consonant,          ScriptType::Brahmi, 39 /* ग़ */,       "ग़" },
  { TokenType::Consonant,          ScriptType::Brahmi, 40 /* ज़ */,      "ज़" },
  { TokenType::Consonant,          ScriptType::Brahmi, 41 /* ड़ */,      "ड़" },
  { TokenType::Consonant,          ScriptType::Brahmi, 42 /* ढ़ */,       "ढ़" },
  { TokenType::Consonant,          ScriptType::Brahmi, 43 /* फ़ */,      "फ़" },
  { TokenType::Consonant,          ScriptType::Brahmi, 44 /* य़ */,       "य़" },
  { TokenType::VowelDiacritic,     ScriptType::Brahmi,  0 /* ् */,       "᳭" }, 
  { TokenType::VowelDiacritic,     ScriptType::Brahmi,  0 /* ् */,       "്‍"sv /*virama + ZWJ*/ }, 
  { TokenType::VowelDiacritic,     ScriptType::Brahmi,  0 /* ् */,       "‍್"sv /*ZWJ+virama*/ }, 
  { TokenType::VowelDiacritic,     ScriptType::Brahmi,  0 /* ् */,       "්‍"sv /*virama + ZWJ*/ }, 
  { TokenType::VowelDiacritic,     ScriptType::Brahmi,  0 /* ् */,       "्‍"sv /*virama + ZWJ*/ },
  { TokenType::VowelDiacritic,     ScriptType::Tamil,  13 /* ொ */,    "ொ" },
  { TokenType::VowelDiacritic,     ScriptType::Tamil,  14 /* ோ */,    "ோ" },
  { TokenType::VowelDiacritic,     ScriptType::Tamil,  15 /* ௌ */,   "ௌ" },
  { TokenType::Accent,             ScriptType::Brahmi,  1 /*꠰  ॑ */,       "◌॑" },
  { TokenType::Accent,             ScriptType::Roman,   0 /*꠰  ॒ */,       "̱" }, 
  { TokenType::Accent,             ScriptType::Roman,   0 /*꠰  ॒ */,       "↓" }, 
  { TokenType::Accent,             ScriptType::Roman,   0 /*꠰  ॒ */,       "\\_" }, 
  { TokenType::Accent,             ScriptType::Roman,   0 /*꠰  ॒ */,       "\\`" }, 
  { TokenType::Accent,             ScriptType::Roman,   0 /*꠰  ॒ */,       "'" }, 
  { TokenType::Accent,             ScriptType::Roman,   0 /*꠰  ॒ */,       "`" },
  { TokenType::Accent,             ScriptType::Roman,   1 /*꠰  ॑ */,       "̍" }, 
  { TokenType::Accent,             ScriptType::Roman,   1 /*꠰  ॑ */,       "↑" }, 
  { TokenType::Accent,             ScriptType::Roman,   1 /*꠰  ॑ */,       "\\'" }, 
  { TokenType::Accent,             ScriptType::Roman,   1 /*꠰  ॑ */,       "\\’" }, 
  { TokenType::Accent,             ScriptType::Roman,   1 /*꠰  ॑ */,       "̭" },
  { TokenType::Accent,             ScriptType::Roman,   2 /*꠰  ᳚ */,       "̎" }, 
  { TokenType::Accent,             ScriptType::Roman,   2 /*꠰  ᳚ */,       "↑↑" }, 
  { TokenType::Accent,             ScriptType::Roman,   2 /*꠰  ᳚ */,       "\\\"" }, 
  { TokenType::Accent,             ScriptType::Roman,   2 /*꠰  ᳚ */,       "\\''" }, 
  { TokenType::Accent,             ScriptType::Roman,   2 /*꠰  ᳚ */,       "\\’’" },
  { TokenType::ConsonantDiacritic, ScriptType::Tamil,   0 /* ँ */,       "ம்ˮ" },
  { TokenType::ConsonantDiacritic, ScriptType::Tamil,   1 /* ं */,       "ம்ʼ" },
  { TokenType::ConsonantDiacritic, ScriptType::Roman,   0 /* ँ */,       "~" }, 
  { TokenType::ConsonantDiacritic, ScriptType::Roman,   0 /* ँ */,       "m̐" }, 
  { TokenType::ConsonantDiacritic, ScriptType::Roman,   0 /* ँ */,       "ṁ" },
  { TokenType::ConsonantDiacritic, ScriptType::Roman,   1 /* ं */,       "ṃ" }, 
  { TokenType::ConsonantDiacritic, ScriptType::Roman,   1 /* ं */,       "ṃ" },
  { TokenType::ConsonantDiacritic, ScriptType::Roman,   2 /* ः */,       "ḥ" }, 
  { TokenType::ConsonantDiacritic, ScriptType::Roman,   2 /* ः */,       "ḥ" },
  { TokenType::Symbol,             ScriptType::Brahmi, 10 /* ॐ */,      "ओ३म्" },
  { TokenType::Symbol,             ScriptType::Roman,  10 /* ॐ */,      "o3m" }, 
  { TokenType::Symbol,             ScriptType::Roman,  10 /* ॐ */,      "OM" }, 
  { TokenType::Symbol,             ScriptType::Roman,  10 /* ॐ */,      "AUM" }, 
  { TokenType::Symbol,             ScriptType::Roman,  10 /* ॐ */,      "oṃ" }, 
  { TokenType::Symbol,             ScriptType::Roman,  10 /* ॐ */,      "ŏṃ" },
  { TokenType::Symbol,             ScriptType::Roman,  12 /* ꠰ */,        "|" }, 
  { TokenType::Symbol,             ScriptType::Roman,  12 /* ꠰ */,        "." },
  { TokenType::Symbol,             ScriptType::Roman,  13 /* ॥ */,       "||" }, 
  { TokenType::Symbol,             ScriptType::Roman,  13 /* ॥ */,       "꠰꠰" }, 
  { TokenType::Symbol,             ScriptType::Roman,  13 /* ॥ */,       ".." },
  { TokenType::Symbol,             ScriptType::Roman,  14 /* ꣳ */,       "gͫ" }, 
  { TokenType::Symbol,             ScriptType::Roman,  14 /* ꣳ */,       "\\m+" }, 
  { TokenType::Symbol,             ScriptType::Roman,  14 /* ꣳ */,       "{\\m+}" }, 
  { TokenType::Symbol,             ScriptType::Roman,  14 /* ꣳ */,       "\\м+" },
  { TokenType::Symbol,             ScriptType::Roman,  15 /* ꣴ */,       "gͫ̄" }, 
  { TokenType::Symbol,             ScriptType::Roman,  15 /* ꣴ */,       "\\m++" }, 
  { TokenType::Symbol,             ScriptType::Roman,  15 /* ꣴ */,       "\\м++" },
  { TokenType::Symbol,             ScriptType::Roman,  17 /* \u200C */,  "{}" }, 
  { TokenType::Symbol,             ScriptType::Roman,  17 /* \u200C */,  "^^" },
  { TokenType::Symbol,             ScriptType::Roman,  18 /* \u200D */,  "()" },
  { TokenType::Symbol,             ScriptType::Roman,  18 /* \u200D */,  "^" },
}};

constexpr std::array<std::string_view, 21> Accents      = { 
  /*꠰*/ "॒", "॑", "᳚", "᳡", "꣡", "꣢", "꣣", "꣤", "꣥", "꣦", "꣧", "꣨", "꣩", "꣪", "꣫", "꣬", "꣭", "꣮", "꣯", "꣰", "꣱" 
};
constexpr std::array<std::string_view, 21> RomanAccents = { 
  /*꠰*/ "̱", "̍", "̎", "᳡", "꣡", "꣢", "꣣", "꣤", "꣥", "꣦", "꣧", "꣨", "꣩", "꣪", "꣫", "꣬", "꣭", "꣮", "꣯", "꣰", "꣱" 
};

constexpr std::string_view TamilSuperscripts = "¹²³⁴";
constexpr std::string_view SkipTrans { "##" };

constexpr std::string_view TamilSpecialChars { "ʼˮˇ꞉ஃ·" };
constexpr std::string_view QuotedMarkers { "ʼˮ" };

// clang-format on

constexpr std::array<std::string_view, 6> TamilPrefixes {
  "அது",
  "இது",
  "மா",
  "ஒரு",
  "அந்த",
  "இந்த",
};
