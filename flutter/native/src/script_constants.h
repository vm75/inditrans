#pragma once

#include "type_defs.h"
#include <array>
#include <string_view>

constexpr std::array<std::string_view, 3> WriteOnlyScripts { "readablelatin", "romancolloquial" };
auto isWriteOnlyScript = [](std::string_view script) noexcept {
  return std::find(WriteOnlyScripts.begin(), WriteOnlyScripts.end(), script) != WriteOnlyScripts.end();
};

// clang-format off

constexpr std::array<std::string_view, 21> Accents      = { 
  /*꠰*/ "॒", "॑", "᳚", "᳡", "꣡", "꣢", "꣣", "꣤", "꣥", "꣦", "꣧", "꣨", "꣩", "꣪", "꣫", "꣬", "꣭", "꣮", "꣯", "꣰", "꣱" 
};
constexpr std::array<std::string_view, 21> LatinAccents = { 
  /*꠰*/ "̱", "̍", "̎", "᳡", "꣡", "꣢", "꣣", "꣤", "꣥", "꣦", "꣧", "꣨", "꣩", "꣪", "꣫", "꣬", "꣭", "꣮", "꣯", "꣰", "꣱" 
};

constexpr std::array<std::string_view, 43> VedicSigns = { 
 /*꠰*/ "᳐", "᳑", "᳒", "᳓", "᳔", "᳕", "᳖", "᳗", "᳘", "᳙", "᳚", "᳛", "᳜", "᳝", "᳞", "᳟",
 /*꠰*/ "᳠", "᳡", "᳢", "᳣", "᳤", "᳥", "᳦", "᳧", "᳨", "ᳩ", "ᳪ", "ᳫ", "ᳬ", "᳭", "ᳮ", "ᳯ",
 /*꠰*/ "ᳰ", "ᳱ", "ᳲ", "ᳳ", "᳴", "ᳵ", "ᳶ", "᳷", "᳸", "᳹", "ᳺ" 
};

constexpr std::array<std::string_view, 18> SpecialVedicAccents = { 
  /*꠰*/ "᳡", "꣡", "꣢", "꣣", "꣤", "꣥", "꣦", "꣧", "꣨", "꣩", "꣪", "꣫", "꣬", "꣭", "꣮", "꣯", "꣰", "꣱" 
};

constexpr std::array<std::string_view, 12> SpecialVedicSymbols = { 
  /*꠰*/ "ꣲ", "ꣵ", "ꣶ", "ꣷ", "꣸", "꣹", "꣺", "ꣻ", "꣼", "ꣽ", 
};

constexpr std::string_view TamilSuperscripts = "¹²³⁴";
constexpr std::string_view TamilSubscripts = "₁₂₃₄";
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
