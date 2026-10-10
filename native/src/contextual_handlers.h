#pragma once

#include "inditrans.h"
#include "script_constants.h"
#include "static_script_types.h"
#include "token_stream.h"
#include <array>
#include <cstdint>
#include <string_view>

namespace inditrans::handlers {

/// Assimilates anuswara (nasalization mark) to the class nasal of the following consonant
/// (e.g. ṅ before velars, ñ before palatals, ṇ before retroflex, n before dentals, m before labials).
class AnuswaraAssimilator {
public:
  /// Infers the canonical consonant phoneme index for the assimilated nasal.
  /// Defaults to 'म' (idx 24) when the following phoneme does not belong to the five vargas.
  static constexpr size_t inferNasalIndex(const TokenUnitOrString& next) noexcept {
    size_t idx = 24 /* म */;
    if (next != endOfText && !HoldsString(next)) {
      const auto tokenUnit = GetTokenUnit(next);
      if (tokenUnit.leadToken.idx < 24) {
        idx = (((tokenUnit.leadToken.idx / 5) * 5) + 4);
      }
    }
    return idx;
  }

  /// Emits the assimilated nasal consonant into the destination output.
  /// If TargetScript is Tamil, appends a pulli (virama) to keep the nasal vowelless.
  template <ScriptType TargetScript, typename PushFn>
  static void emit(const TokenUnitOrString& next, const ScriptWriterMap& map, PushFn&& push) noexcept {
    const size_t idx = inferNasalIndex(next);
    push(map.lookupChar(TokenType::Consonant, idx));
    if constexpr (TargetScript == ScriptType::Tamil) {
      push(map.lookupChar(Virama));
    }
  }
};

/// Formats Indic/Tamil numerals as ASCII digits '0'..'9' when the ASCIINumerals option is enabled.
class AsciiNumeralFormatter {
public:
  static constexpr bool matches(const Token& token, TranslitOptions options) noexcept {
    return (options * TranslitOptions::ASCIINumerals)
        && token.tokenType == TokenType::Symbol
        && token.idx < 10;
  }

  template <typename PushFn>
  static void emit(uint8_t idx, PushFn&& push) noexcept {
    char str[2] = { static_cast<char>('0' + idx), 0 };
    push(str);
  }
};

/// Declarative replacement table and helper for traditional Tamil consonant substitutions.
/// Replaces modern Grantha consonants (ஸ, ஜ, ஜ²) with traditional Tamil orthography (ச, ச³, ச⁴).
class TamilTraditionalReplacements {
public:
  struct Substitution {
    std::string_view modern;
    std::string_view traditional;
  };

  static constexpr std::array<Substitution, 3> Table {{
    { "ஸ", "ச" },
    { "ஜ", "ச³" },
    { "ஜ²", "ச⁴" },
  }};

  static constexpr std::string_view replace(std::string_view text) noexcept {
    for (const auto& entry : Table) {
      if (text == entry.modern)
        return entry.traditional;
    }
    return text;
  }
};

} // namespace inditrans::handlers
