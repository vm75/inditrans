#pragma once

#include "type_defs.h"
#include <array>
#include <string_view>

constexpr std::array<std::string_view, 3> WriteOnlyScripts { "readablelatin", "romancolloquial" };
auto isWriteOnlyScript = [](std::string_view script) noexcept {
  return std::find(WriteOnlyScripts.begin(), WriteOnlyScripts.end(), script) != WriteOnlyScripts.end();
};

constexpr std::array<std::string_view, 11> IndicScripts { "assamese", "bengali", "devanagari", "gujarati", "gurmukhi",
  "kannada", "malayalam", "oriya", "sinhala", "tamil", "telugu" };
auto isIndicScript = [](std::string_view script) noexcept {
  return std::find(IndicScripts.begin(), IndicScripts.end(), script) != IndicScripts.end();
};

// clang-format off
constexpr std::array<AliasEntry, 50> PositionalAliases {{
  { TokenType::VowelDiacritic,     ScriptType::Indic,   0 /* ् */,       "᳭" }, 
  { TokenType::VowelDiacritic,     ScriptType::Indic,   0 /* ् */,       "്‍"sv /*virama + ZWJ*/ }, 
  { TokenType::VowelDiacritic,     ScriptType::Indic,   0 /* ् */,       "‍್"sv /*ZWJ+virama*/ }, 
  { TokenType::VowelDiacritic,     ScriptType::Indic,   0 /* ् */,       "්‍"sv /*virama + ZWJ*/ }, 
  { TokenType::VowelDiacritic,     ScriptType::Indic,   0 /* ् */,       "्‍"sv /*virama + ZWJ*/ },
  { TokenType::Accent,             ScriptType::Indic,   1 /*꠰  ॑ */,       "◌॑" },
  { TokenType::Accent,             ScriptType::Latin,   0 /*꠰  ॒ */,       "̱" }, 
  { TokenType::Accent,             ScriptType::Latin,   0 /*꠰  ॒ */,       "↓" }, 
  { TokenType::Accent,             ScriptType::Latin,   0 /*꠰  ॒ */,       "\\_" }, 
  { TokenType::Accent,             ScriptType::Latin,   0 /*꠰  ॒ */,       "\\`" }, 
  { TokenType::Accent,             ScriptType::Latin,   0 /*꠰  ॒ */,       "'" }, 
  { TokenType::Accent,             ScriptType::Latin,   0 /*꠰  ॒ */,       "`" },
  { TokenType::Accent,             ScriptType::Latin,   1 /*꠰  ॑ */,       "̍" }, 
  { TokenType::Accent,             ScriptType::Latin,   1 /*꠰  ॑ */,       "↑" }, 
  { TokenType::Accent,             ScriptType::Latin,   1 /*꠰  ॑ */,       "\\'" }, 
  { TokenType::Accent,             ScriptType::Latin,   1 /*꠰  ॑ */,       "\\’" }, 
  { TokenType::Accent,             ScriptType::Latin,   1 /*꠰  ॑ */,       "̭" },
  { TokenType::Accent,             ScriptType::Latin,   2 /*꠰  ᳚ */,       "̎" }, 
  { TokenType::Accent,             ScriptType::Latin,   2 /*꠰  ᳚ */,       "↑↑" }, 
  { TokenType::Accent,             ScriptType::Latin,   2 /*꠰  ᳚ */,       "\\\"" }, 
  { TokenType::Accent,             ScriptType::Latin,   2 /*꠰  ᳚ */,       "\\''" }, 
  { TokenType::Accent,             ScriptType::Latin,   2 /*꠰  ᳚ */,       "\\’’" },
  { TokenType::ConsonantDiacritic, ScriptType::Latin,   0 /* ँ */,       "~" }, 
  { TokenType::ConsonantDiacritic, ScriptType::Latin,   0 /* ँ */,       "m̐" }, 
  { TokenType::ConsonantDiacritic, ScriptType::Latin,   0 /* ँ */,       "ṁ" },
  { TokenType::ConsonantDiacritic, ScriptType::Latin,   1 /* ं */,       "ṃ" }, 
  { TokenType::ConsonantDiacritic, ScriptType::Latin,   1 /* ं */,       "ṃ" },
  { TokenType::ConsonantDiacritic, ScriptType::Latin,   2 /* ः */,       "ḥ" }, 
  { TokenType::ConsonantDiacritic, ScriptType::Latin,   2 /* ः */,       "ḥ" },
  { TokenType::Symbol,             ScriptType::Latin,  10 /* ॐ */,      "o3m" }, 
  { TokenType::Symbol,             ScriptType::Latin,  10 /* ॐ */,      "OM" }, 
  { TokenType::Symbol,             ScriptType::Latin,  10 /* ॐ */,      "AUM" }, 
  { TokenType::Symbol,             ScriptType::Latin,  10 /* ॐ */,      "oṃ" }, 
  { TokenType::Symbol,             ScriptType::Latin,  10 /* ॐ */,      "ŏṃ" },
  { TokenType::Symbol,             ScriptType::Latin,  12 /* ꠰ */,        "|" }, 
  { TokenType::Symbol,             ScriptType::Latin,  12 /* ꠰ */,        "." },
  { TokenType::Symbol,             ScriptType::Latin,  13 /* ॥ */,       "||" }, 
  { TokenType::Symbol,             ScriptType::Latin,  13 /* ॥ */,       "꠰꠰" }, 
  { TokenType::Symbol,             ScriptType::Latin,  13 /* ॥ */,       ".." },
  { TokenType::Symbol,             ScriptType::Latin,  14 /* ꣳ */,       "gͫ" }, 
  { TokenType::Symbol,             ScriptType::Latin,  14 /* ꣳ */,       "\\m+" }, 
  { TokenType::Symbol,             ScriptType::Latin,  14 /* ꣳ */,       "{\\m+}" }, 
  { TokenType::Symbol,             ScriptType::Latin,  14 /* ꣳ */,       "\\м+" },
  { TokenType::Symbol,             ScriptType::Latin,  15 /* ꣴ */,       "gͫ̄" }, 
  { TokenType::Symbol,             ScriptType::Latin,  15 /* ꣴ */,       "\\m++" }, 
  { TokenType::Symbol,             ScriptType::Latin,  15 /* ꣴ */,       "\\м++" },
  { TokenType::Symbol,             ScriptType::Latin,  17 /* \u200C */,  "{}" }, 
  { TokenType::Symbol,             ScriptType::Latin,  17 /* \u200C */,  "^^" },
  { TokenType::Symbol,             ScriptType::Latin,  18 /* \u200D */,  "()" },
  { TokenType::Symbol,             ScriptType::Latin,  18 /* \u200D */,  "^" },
}};

constexpr std::array<std::string_view, 21> Accents      = { 
  /*꠰*/ "॒", "॑", "᳚", "᳡", "꣡", "꣢", "꣣", "꣤", "꣥", "꣦", "꣧", "꣨", "꣩", "꣪", "꣫", "꣬", "꣭", "꣮", "꣯", "꣰", "꣱" 
};
constexpr std::array<std::string_view, 21> LatinAccents = { 
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
