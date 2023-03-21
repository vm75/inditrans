#pragma once

#include "type_defs.h"
#include <algorithm>
#include <array>
#include <string_view>

constexpr std::array<std::string_view, 1> WriteOnlyScripts { "readablelatin" };
auto isWriteOnlyScript = [](std::string_view script) noexcept {
  return std::find(WriteOnlyScripts.begin(), WriteOnlyScripts.end(), script) != WriteOnlyScripts.end();
};

constexpr std::array<std::string_view, 2> CaseInsensitiveScripts { "iast", "iso" };
auto isCaseInsensitiveScripts = [](std::string_view script) noexcept {
  return std::find(CaseInsensitiveScripts.begin(), CaseInsensitiveScripts.end(), script)
      != CaseInsensitiveScripts.end();
};

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

constexpr std::array<std::string_view, 41> VedicAccents = {
  /*꠰*/ "॒", "॑", "᳚",
  /*꠰*/ "᳐", "᳑", "᳒", "᳓", "᳔", "᳕", "᳖", "᳗", "᳘", "᳙", "᳛", "᳜", "᳝", "᳞", "᳟",
  /*꠰*/ "᳠", "᳢", "᳣", "᳤", "᳥", "᳦", "᳧", "᳨", "ᳩ", "ᳪ", "ᳫ", "ᳬ", "ᳮ", "ᳯ",
  /*꠰*/ "ᳰ", "ᳱ", "ᳲ", "ᳳ", "᳴", "᳷", "᳸", "᳹", "ᳺ"
};
constexpr std::array<std::string_view, 3> LatinAccents = {
  /*꠰*/ "̱", "̍", "̎",
};

constexpr std::array<std::string_view, 3> IndicZeroWidthChars = {
  "‌", "‍"
};
constexpr std::array<std::string_view, 3> DefaultZeroWidthChars = {
  "", ""
};

constexpr std::array<std::string_view, 28> DevanagariExtSymbols = {
  /*꠰*/ "᳡",
  /*꠰*/ "꣡", "꣢", "꣣", "꣤", "꣥", "꣦", "꣧", "꣨", "꣩",
  /*꠰*/ "꣪", "꣫", "꣬", "꣭", "꣮", "꣯", "꣰", "꣱",
  /*꠰*/ "ꣲ", "ꣵ", "ꣶ", "ꣷ", "꣸", "꣹", "꣺", "ꣻ", "꣼", "ꣽ",
};

constexpr std::string_view TamilSuperscripts { "¹²³⁴" };
constexpr std::string_view TamilSubscripts { "₁₂₃₄" };

constexpr std::string_view TamilSpecialChars { "ʼˮˇ꞉ஃ·" };
constexpr std::string_view SpecialMarkers { "ʽʼˮˇ" };

// clang-format on

constexpr std::array<std::string_view, 6> TamilPrefixes {
  "அது",
  "இது",
  "மா",
  "ஒரு",
  "அந்த",
  "இந்த",
};
