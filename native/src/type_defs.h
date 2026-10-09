#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string_view>
#include <vector>

using namespace std::literals::string_view_literals;

/// High-level script family classification governing transliteration rules.
enum class ScriptType : uint8_t {
  Indic, ///< Standard Brahmic abugidas (Devanagari, Bengali, Telugu, Kannada, etc.)
  Tamil, ///< Tamil script, which features a reduced consonant set, allophones, and superscripts
  Latin, ///< Romanization schemes (ITRANS, Harvard-Kyoto, IAST, SLP1, Velthuis, etc.)
  Others ///< Non-alphabetic scripts or special character tables
};

/// Returns true if the script belongs to the Brahmic family (Indic or Tamil).
constexpr bool inline isIndicScript(ScriptType script) noexcept {
  return script == ScriptType::Indic || script == ScriptType::Tamil;
}

/// Phonological category of a token within the transliteration engine.
enum class TokenType : uint8_t {
  Vowel, ///< Independent vowels (e.g., अ, आ, a, aa)
  VowelMark, ///< Dependent vowel signs / matras (e.g., ा, ि) and virama/halant (्)
  Consonant, ///< Base consonants (e.g., क, ख, k, kh)
  OtherDiacritic, ///< Anuswara (ं), visarga (ः), candrabindu (ँ), etc.
  Accent, ///< Vedic pitch/accent marks (udatta, anudatta, svarita)
  Symbol, ///< Punctuation, danda, numerals, and non-phonetic glyphs
  VedicSymbol, ///< Specialized Vedic chanting symbols (ardhavisarga, etc.)
  ExclusiveSymbol, ///< Script-exclusive symbols (e.g., Gurmukhi adhak, skip markers)
  Ignore ///< Sentinel category for characters ignored during transliteration
};

/// Sentinel value indicating an absent or unassigned token index.
constexpr auto InvalidToken = std::numeric_limits<uint8_t>::max();

/// Fundamental 2-byte token identifying a phoneme by its category and canonical table index.
struct Token {
  TokenType tokenType; ///< Phonological category of the token
  uint8_t idx; ///< Canonical phoneme index within the tokenType category

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

/// 4-byte token structure augmenting Token with its originating ScriptType.
///
/// Designed to fit in a 32-bit word for fast register passing and copying,
/// matching the memory layout requirements of TokenUnit.
struct ScriptToken : public Token {
  ScriptType scriptType; ///< Script family of this token
  uint8_t reserved { }; ///< Explicit fourth byte for the uint32_t bit-cast payload

  constexpr ScriptToken(TokenType tokenType = TokenType::Ignore, uint8_t idx = InvalidToken,
      ScriptType scriptType = ScriptType::Others) noexcept
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

/// Global sentinel representing an invalid or uninitialized ScriptToken.
constexpr const ScriptToken invalidScriptToken(TokenType::Ignore, InvalidToken, ScriptType::Others);

/// Represents a composite grapheme cluster or Indian-script syllabic unit (akshara).
///
/// An akshara consists of a lead token (a consonant or an independent vowel) with up to three
/// attached dependent modifiers: a vowel mark (matra or virama), an additional diacritic
/// (such as anuswara or visarga), and an accent (such as a Vedic tone mark).
struct TokenUnit {
  constexpr TokenUnit(ScriptToken leadToken) noexcept
      : leadToken(leadToken) { }
  ScriptToken leadToken; ///< Base consonant or independent vowel initiating this cluster
  // An absent mark on a consonant means inherent 'a'; VowelMark index 0
  // explicitly suppresses it. Vowel index 0 instead denotes independent 'a'.
  Token vowelMark { }; ///< Attached dependent vowel sign (matra) or virama (halant)
  Token otherDiacritic { }; ///< Attached diacritic (e.g. anuswara, visarga, candrabindu)
  Token accent { }; ///< Attached Vedic accent mark (udatta, anudatta, svarita)

  constexpr bool operator==(const TokenUnit& other) const noexcept {
    return leadToken == other.leadToken && vowelMark == other.vowelMark && otherDiacritic == other.otherDiacritic
        && accent == other.accent;
  }
  constexpr bool operator!=(const TokenUnit& other) const noexcept { return !(*this == other); }
};
