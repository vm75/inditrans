#pragma once

#include "contextual_handlers.h"
#include "inditrans.h"
#include "script_constants.h"
#include "static_scripts.h"
#include "static_script_types.h"
#include "token_stream.h"
#include "utf.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace inditrans::handlers {

/// Disambiguation marker payload for Tamil consonants.
struct TamilMarker {
  uint8_t phonemeOffset; ///< Canonical phoneme offset (0..3) within the varga
  uint8_t byteLength;    ///< UTF-8 byte length of the matched marker
};

/// Decodes an attached Tamil superscript (¹²³⁴) or subscript (₁₂₃₄) disambiguation marker
/// into its canonical phoneme offset (0..3) and UTF-8 byte length.
inline constexpr std::optional<TamilMarker> matchTamilMarker(std::string_view str) noexcept {
  if (str.size() >= 2 && static_cast<uint8_t>(str[0]) == 0xC2) {
    switch (static_cast<uint8_t>(str[1])) {
      case 0xB9: return TamilMarker { 0, 2 }; // ¹
      case 0xB2: return TamilMarker { 1, 2 }; // ²
      case 0xB3: return TamilMarker { 2, 2 }; // ³
      default:   return std::nullopt;
    }
  }
  if (str.size() >= 3 && static_cast<uint8_t>(str[0]) == 0xE2) {
    const auto second = static_cast<uint8_t>(str[1]);
    const auto third = static_cast<uint8_t>(str[2]);
    if (second == 0x81 && third == 0xB4) {
      return TamilMarker { 3, 3 }; // ⁴
    }
    if (second == 0x82) {
      switch (third) {
        case 0x81: return TamilMarker { 0, 3 }; // ₁
        case 0x82: return TamilMarker { 1, 3 }; // ₂
        case 0x83: return TamilMarker { 2, 3 }; // ₃
        case 0x84: return TamilMarker { 3, 3 }; // ₄
        default:   return std::nullopt;
      }
    }
  }
  return std::nullopt;
}

/// Returns true if the entire preserved span represents an isolated Tamil marker (superscript or subscript).
inline constexpr bool isTamilMarkerSpan(std::string_view str) noexcept {
  const auto marker = matchTamilMarker(str);
  return marker.has_value() && marker->byteLength == str.size();
}

/// Phonological predicates for Tamil consonants and clusters.
inline constexpr bool isPrimaryStop(uint8_t idx) noexcept {
  return idx <= 20 /* ப */ && idx % 5 == 0;
}

inline constexpr bool isHardConsonant(const TokenUnit& token) noexcept {
  return token.leadToken.tokenType == TokenType::Consonant
      && ((token.leadToken.idx <= 20 && token.leadToken.idx % 5 == 0 /* க ச ட த ப */)
          || token.leadToken.idx == 35 /* ற */);
}

inline constexpr bool isSoftConsonant(const TokenUnit& token) noexcept {
  const auto idx = token.leadToken.idx;
  return token.leadToken.tokenType == TokenType::Consonant
      && ((idx <= 24 && idx % 5 == 4 /* ங ஞ ண ந ம */) || idx == 36 /* ன */ || (idx >= 25 && idx <= 28 /* ய ர ல வ */)
          || idx == 33 /* ள */ || idx == 34 /* ழ */);
}

inline constexpr bool isVirama(const TokenUnit& token) noexcept {
  return token.vowelMark.tokenType == TokenType::VowelMark && token.vowelMark.idx == Diacritic_Virama;
}

inline bool isTamilEndOfWord(const TokenUnitOrString& next) noexcept {
  if (next == endOfText) {
    return true;
  }
  if (HoldsString(next)) {
    return !isTamilMarkerSpan(GetString(next));
  }
  const auto token = GetTokenUnit(next);
  return token.leadToken.tokenType == TokenType::Symbol;
}

/// Groups tokens for Tamil script, applying Tamil-specific phonological rules:
/// - Parses attached superscript/subscript numbers (¹²³⁴) to distinguish aspirated/voiced allophones
/// - Updates prefix recognizer state across prefix-stem boundaries
/// - Applies contextual phonetic rules (e.g. converting ச to ஸ at word start or after certain vowels,
///   or voicing intervocalic / post-nasal stops)
class TamilReaderHandler {
public:
  void onWordBoundary() noexcept { prefixLookupState.reset(); }
  void reset() noexcept { prefixLookupState.reset(); }

  template <typename Stream>
  [[gnu::always_inline]] TokenUnit assemble(const ScriptToken& start, Stream& stream, bool wordStart,
      const TokenUnit& lastToken, TranslitOptions options) noexcept {
    bool endOfPrefix = false;
    if (wordStart) {
      prefixLookupState.reset();
    } else {
      // A recognized prefix keeps start-of-stem pronunciation rules active.
      endOfPrefix = (prefixLookupState.value != std::nullopt);
    }

    TokenUnit tokenUnit = { start };
    if (start.tokenType == TokenType::Consonant) {
      const bool isPrimary = isPrimaryStop(start.idx);
      bool hasVirama = false;
      bool hasMarker = false;

      while (true) {
        const auto* peek = stream.peek();
        if (!peek)
          break;
        if (HoldsScriptToken(*peek)) {
          const auto nextToken = GetScriptToken(*peek);
          switch (nextToken.tokenType) {
            case TokenType::OtherDiacritic:
              stream.advance();
              tokenUnit.otherDiacritic = nextToken;
              break;
            case TokenType::VowelMark:
              stream.advance();
              tokenUnit.vowelMark = nextToken;
              hasVirama = tokenUnit.vowelMark == Virama;
              break;
            case TokenType::Accent:
              stream.advance();
              tokenUnit.accent = nextToken;
              break;
            default:
              goto done;
          }
        } else if ((isPrimary || start.idx == 7 /* ஜ */) && !hasMarker) {
          const auto str = GetString(*peek);
          if (const auto marker = matchTamilMarker(str)) {
            const auto maxOffset = (start.idx == 7) ? 1 : 3;
            if (marker->phonemeOffset <= maxOffset) {
              tokenUnit.leadToken = start.clone(static_cast<uint8_t>(start.idx + marker->phonemeOffset));
              hasMarker = true;
              stream.advance(marker->byteLength);
            } else {
              break;
            }
          } else {
            break;
          }
        } else {
          break;
        }
      }
    done:
      if (options * TranslitOptions::TamilSuperscripted || options * TranslitOptions::DisableInference) {
        // Explicit consonant variants and opt-out bypass contextual pronunciation inference.
        return tokenUnit;
      }

      // Match written Tamil before changing stop indices to their voiced forms.
      prefixLookup.lookup(tokenUnit, prefixLookupState);

      if (hasVirama) {
        // The legacy pronunciation rule drops a word-final primary stop with
        // pulli as a whole unit, including any attached modifiers.
        return (isPrimary && stream.isEndOfWord()) ? invalidTokenUnit : tokenUnit;
      }
      if (isPrimary) {
        if (wordStart || endOfPrefix) {
          constexpr std::array<uint8_t, 6> specialVowelDiactritics = { 2, 4, 10, 13, 16, InvalidToken };
          if (tokenUnit.leadToken.idx == 5 /* ச */ && lastToken.leadToken != tokenUnit.leadToken && !isVirama(lastToken)
              && std::find(specialVowelDiactritics.begin(), specialVowelDiactritics.end(), lastToken.vowelMark.idx)
                  == specialVowelDiactritics.end()) {
            tokenUnit.leadToken = start.clone(31 /* ஸ */);
          }
        } else if (isVirama(lastToken)) {
          // Geminates and stops after hard consonants stay unvoiced. Other
          // clusters voice the stop (+2 in its varga), with a separate ச→ஸ rule.
          if (lastToken.leadToken != tokenUnit.leadToken && !isHardConsonant(lastToken)) {
            if (tokenUnit.leadToken.idx == 5 /* ச */ && !isSoftConsonant(lastToken)) {
              tokenUnit.leadToken = start.clone(31 /* ஸ */);
            } else {
              tokenUnit.leadToken = start.clone(static_cast<uint8_t>(start.idx + 2));
            }
          }
        } else {
          if (tokenUnit.leadToken.idx == 5 /* ச */) {
            tokenUnit.leadToken = start.clone(31 /* ஸ */);
          } else {
            tokenUnit.leadToken = start.clone(static_cast<uint8_t>(start.idx + 2));
          }
        }
      }
    } else if (start.tokenType == TokenType::Vowel) {
      while (true) {
        const auto* peek = stream.peek();
        if (!peek || !HoldsScriptToken(*peek))
          break;
        const auto nextToken = GetScriptToken(*peek);
        if (nextToken.tokenType == TokenType::OtherDiacritic) {
          tokenUnit.otherDiacritic = nextToken;
        } else if (nextToken.tokenType == TokenType::Accent) {
          tokenUnit.accent = nextToken;
        } else {
          break;
        }
        stream.advance();
      }
    }
    if (tokenUnit.leadToken.tokenType != TokenType::Consonant) {
      prefixLookup.lookup(tokenUnit, prefixLookupState);
    }
    return tokenUnit;
  }

private:
  TamilPrefixLookup prefixLookup;
  TamilPrefixLookup::LookupState prefixLookupState { };
};

/// Writes a TokenUnit in Tamil script, enforcing Tamil orthographic constraints:
/// - Distributes dental 'ந' at word/prefix start vs alveolar 'ன' word-medially/finally
/// - Applies optional TamilTraditional consonant replacements (e.g. ஸ -> ச)
/// - Places superscripts (¹²³⁴) after consonant or after matra as appropriate
/// - Inserts superscripts between the components of multi-part vowel marks
/// - Assimilates anuswara into following varga nasal stop
class TamilWriterHandler {
public:
  void onWordBoundary() noexcept { prefixLookupState.reset(); }
  void reset() noexcept { prefixLookupState.reset(); }

  template <typename Writer, typename Buffer>
  [[gnu::always_inline]] void write(const TokenUnit& tokenUnit, const TokenUnitOrString& next,
      const ScriptWriterMap& map, TranslitOptions options,
      Writer& writer, Buffer& buffer, bool wordStart) noexcept {
    const bool endOfPrefix = prefixLookupState.value != std::nullopt;
    if (wordStart) {
      prefixLookupState.reset();
    }
    prefixLookup.lookup(tokenUnit, prefixLookupState);

    const auto leadIdx = tokenUnit.leadToken.idx;
    auto leadText = map.lookupChar(tokenUnit.leadToken);

    if (tokenUnit.leadToken.tokenType == TokenType::Consonant) {
      // Keep dental ந at word/prefix starts and within consonant clusters;
      // use alveolar ன for medial syllables with vowels and final bare consonants.
      if (leadIdx == 19 /* ந */ && options / TranslitOptions::DisableInference && !(wordStart || endOfPrefix)
          && (tokenUnit.vowelMark.idx != Diacritic_Virama || isTamilEndOfWord(next))) {
        leadText = "ன";
      } else if (options * TranslitOptions::TamilTraditional) {
        leadText = TamilTraditionalReplacements::replace(leadText);
      }

      auto superscript = Utf8String::trailingChar(leadText).view();
      if (leadText.length() > superscript.length() && TamilSuperscripts.find(superscript) != TamilSuperscripts.npos) {
        leadText = { leadText.data(), leadText.length() - superscript.length() };
      } else {
        superscript = "";
      }
      writer.push(leadText);

      std::string_view vowelMarkReminder { };
      if (tokenUnit.vowelMark.idx != InvalidToken) {
        const auto vowelMark = map.lookupChar(tokenUnit.vowelMark);
        if (vowelMark.size() > UtfUtils::nextCharLen(vowelMark.data())) {
          writer.push(UtfUtils::nextUtf8Char(vowelMark.data()));
          vowelMarkReminder = vowelMark.substr(UtfUtils::nextCharLen(vowelMark.data()));
        } else {
          writer.push(vowelMark);
        }
      }

      if (options * TranslitOptions::TamilSuperscripted && !superscript.empty()) {
        writer.push(superscript);
      }
      if (!vowelMarkReminder.empty()) {
        writer.push(vowelMarkReminder);
      }

      if (tokenUnit.otherDiacritic.idx != InvalidToken) {
        if (tokenUnit.otherDiacritic.idx == Diacritic_Anuswara
              && options / TranslitOptions::DisableInference) {
          AnuswaraAssimilator::emit<ScriptType::Tamil>(next, map, writer);
        } else {
          writer.push(map.lookupChar(tokenUnit.otherDiacritic));
        }
      }
    } else {
      if (tokenUnit.leadToken.tokenType == TokenType::Vowel) {
        writer.push(leadText);

        if (tokenUnit.otherDiacritic.idx != InvalidToken) {
          if (tokenUnit.otherDiacritic.idx == Diacritic_Anuswara
              && options / TranslitOptions::DisableInference) {
            AnuswaraAssimilator::emit<ScriptType::Tamil>(next, map, writer);
          } else {
            writer.push(map.lookupChar(tokenUnit.otherDiacritic));
          }
        }
      } else if (AsciiNumeralFormatter::matches(tokenUnit.leadToken, options)) {
        AsciiNumeralFormatter::emit(tokenUnit.leadToken.idx, writer);
      } else {
        writer.push(leadText);
      }
    }

    if ((options / TranslitOptions::IgnoreVedicAccents) && (tokenUnit.accent.idx != InvalidToken)
        && (tokenUnit.leadToken.tokenType == TokenType::Consonant
            || tokenUnit.leadToken.tokenType == TokenType::Vowel)) {
      const auto lastChar = buffer.back().view();
      if (TamilSpecialChars.find(lastChar) != TamilSpecialChars.npos) {
        // Place the accent before a trailing modifier. Copy the borrowed view
        // before appending, since growing the sink can invalidate its storage.
        const std::string lastCharStr { lastChar };
        buffer.pop_back();
        writer.push(map.lookupChar(tokenUnit.accent));
        writer.push(lastCharStr);
      } else {
        writer.push(map.lookupChar(tokenUnit.accent));
      }
    }
  }

private:
  TamilPrefixLookup prefixLookup;
  TamilPrefixLookup::LookupState prefixLookupState { };
};

/// Empty handler used when Tamil writer rules are not applicable.
struct EmptyWriterHandler { };

} // namespace inditrans::handlers
