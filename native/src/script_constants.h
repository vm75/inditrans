#pragma once

#include "type_defs.h"
#include <algorithm>
#include <array>
#include <string_view>

/// Scripts that can only be used as destination targets ('to'), not as input sources ('from').
constexpr std::array<std::string_view, 1> WriteOnlyScripts { "readablelatin" };

/// Returns true if the script is write-only.
inline constexpr auto isWriteOnlyScript = [](std::string_view script) noexcept {
  return std::find(WriteOnlyScripts.begin(), WriteOnlyScripts.end(), script) != WriteOnlyScripts.end();
};

/// Romanization scripts whose matching policy performs ASCII case-folding.
constexpr std::array<std::string_view, 2> CaseInsensitiveScripts { "iast", "iso" };

/// Returns true if the script scheme uses case-insensitive ASCII matching.
inline constexpr auto isCaseInsensitiveScripts = [](std::string_view script) noexcept {
  return std::find(CaseInsensitiveScripts.begin(), CaseInsensitiveScripts.end(), script)
      != CaseInsensitiveScripts.end();
};

// clang-format off

// Canonical Phoneme Consonant Indices:
// - Varga 1 (Guttural/Velar):  क = 0,  ख = 1,  ग = 2,  घ = 3,  ङ = 4
// - Varga 2 (Palatal):         च = 5,  छ = 6,  ज = 7,  झ = 8,  ञ = 9
// - Varga 3 (Retroflex):       ट = 10, ठ = 11, ड = 12, ढ = 13, ण = 14
// - Varga 4 (Dental):          त = 15, थ = 16, द = 17, ध = 18, न = 19
// - Varga 5 (Labial):          प = 20, फ = 21, ब = 22, भ = 23, म = 24
// - Semivowels:                य = 25, र = 26, ल = 27, व = 28
// - Sibilants & Aspirate:      श = 29, ष = 30, स = 31, ह = 32
// - Dravidian consonants:      ळ = 33, ழ = 34, ற = 35, ன = 36
// - Nukta / Urdu consonants:   क़ = 37, ख़ = 38, ग़ = 39, ज़ = 40, ड़ = 41, ढ़ = 42, फ़ = 43, य़ = 44
// - Sinhala prenasalized:      ඟ = 45, ඦ = 46, ඬ = 47, ඳ = 48, ඹ = 49
// - Tamil primary mappings:    க = 0,  ச = 5,  ஜ = 7,  ட = 10, த = 15, ந = 19, ப = 20, ஸ = 31

/// Index for Virama / Halant (vowel suppression mark) in the VowelMark category.
constexpr uint8_t Diacritic_Virama = 0;

/// Index for Anuswara (nasalization mark) in the OtherDiacritic category.
constexpr uint8_t Diacritic_Anuswara = 1;

/// Index for the Skip exclusive symbol (sentinel to ignore token).
constexpr uint8_t ExclusiveSymbol_Skip = 2;

/// Index for the Gurmukhi Adhak (consonant gemination mark).
constexpr uint8_t ExclusiveSymbol_Adhak = 4;

/// Virama / Halant token representing vowel suppression.
constexpr ScriptToken Virama { TokenType::VowelMark, Diacritic_Virama, ScriptType::Indic };

/// Anuswara token representing nasalization.
constexpr ScriptToken Anuswara { TokenType::VowelMark, Diacritic_Anuswara, ScriptType::Indic };

/// Gurmukhi Adhak token representing consonant doubling.
constexpr ScriptToken GurmukhiAdhak { TokenType::ExclusiveSymbol, ExclusiveSymbol_Adhak, ScriptType::Indic };

/// Sentinel token indicating an input sequence to skip.
constexpr Token Skip { TokenType::ExclusiveSymbol, ExclusiveSymbol_Skip };

/// Superscript numerals (¹²³⁴) used for Tamil allophone disambiguation.
constexpr std::string_view TamilSuperscripts { "¹²³⁴" };

/// Subscript numerals (₁₂₃₄) used for Tamil allophone disambiguation.
constexpr std::string_view TamilSubscripts { "₁₂₃₄" };

/// Special Tamil modifier and punctuation characters.
constexpr std::string_view TamilSpecialChars { "ʼˮˇ꞉ஃ·" };

/// Disambiguation marker characters stripped unless RetainSpecialMarkers is set.
constexpr std::string_view SpecialMarkers { "ʽʼˮˇ" };

// clang-format on
