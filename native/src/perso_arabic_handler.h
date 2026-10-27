#pragma once

#include "contextual_handlers.h"
#include "script_constants.h"
#include "static_script_types.h"
#include "token_stream.h"
#include "utf.h"

namespace inditrans::handlers {

/// Explicit vowels only: an unmarked Urdu consonant has no inferred vowel.
/// Shadda produces two ordinary units, without expanding or rebuilding tables.
class PersoArabicReaderHandler {
public:
  bool hasPending() const noexcept { return pending; }
  TokenUnit takePending() noexcept {
    pending = false;
    return repeated;
  }

  template <typename Stream> [[gnu::noinline]] TokenUnit assemble(const ScriptToken& start, Stream& stream) noexcept {
    TokenUnit unit { start };
    if (start.tokenType != TokenType::Consonant && start.tokenType != TokenType::Vowel)
      return unit;
    if (start.tokenType == TokenType::Consonant)
      unit.vowelMark = Virama;
    bool doubled = false;
    while (const auto* item = stream.peek()) {
      if (!HoldsScriptToken(*item))
        break;
      const auto token = GetScriptToken(*item);
      if (token.scriptType != ScriptType::PersoArabic)
        break;
      if (token.tokenType == TokenType::VowelMark && start.tokenType == TokenType::Consonant) {
        unit.vowelMark = token;
      } else if (token.tokenType == TokenType::Consonant
          && ((unit.vowelMark.idx == 2 && token.idx == 25) || (unit.vowelMark.idx == 4 && token.idx == 28))) {
        // A point before aspiration can be separated from its long-vowel letter.
        unit.vowelMark.idx = unit.vowelMark.idx == 2 ? 3 : 5;
      } else if (token.tokenType == TokenType::OtherDiacritic && unit.otherDiacritic.idx == InvalidToken) {
        unit.otherDiacritic = token;
      } else if (token.tokenType == TokenType::ExclusiveSymbol && token.idx == 5
          && start.tokenType == TokenType::Consonant) {
        unit.vowelMark = { }; // fatha is the canonical inherent a, not a carrier
      } else if (token.tokenType == TokenType::ExclusiveSymbol && token.idx == 3
          && start.tokenType == TokenType::Consonant && !doubled) {
        doubled = true;
      } else if (token.tokenType == TokenType::ExclusiveSymbol && token.idx == 4 && unit.leadToken.idx <= 22
          && unit.leadToken.idx % 5 <= 2 && unit.leadToken.idx % 5 != 1) {
        ++unit.leadToken.idx; // a vowel mark can intervene before do-chashmi-he
      } else {
        break;
      }
      stream.advance();
    }
    if (doubled) {
      repeated = unit;
      pending = true;
      unit = TokenUnit { unit.leadToken };
      unit.vowelMark = Virama;
    }
    return unit;
  }

private:
  bool pending { false };
  TokenUnit repeated { invalidScriptToken };
};

/// Emits logical Unicode Urdu with explicit short vowels and contextual yeh.
class PersoArabicWriterHandler {
public:
  template <typename Writer>
  void write(const TokenUnit& unit, const TokenUnitOrString& next, const ScriptWriterMap& map, TranslitOptions options,
      Writer& writer, bool wordStart) noexcept {
    const auto& lead = unit.leadToken;
    if (!geminate && lead.tokenType == TokenType::Consonant && unit.vowelMark.idx == 0
        && unit.otherDiacritic.idx == InvalidToken && next != endOfText && !HoldsString(next)
        && GetTokenUnit(next).leadToken.tokenType == TokenType::Consonant
        && GetTokenUnit(next).leadToken.idx == lead.idx) {
      geminate = true;
      return;
    }
    if (AsciiNumeralFormatter::matches(lead, options))
      AsciiNumeralFormatter::emit(lead.idx, writer);
    else {
      const auto text = map.lookupChar(lead);
      if (!wordStart && lead.tokenType == TokenType::Vowel && text.starts_with("ا")
          && (lead.idx == 2 || lead.idx == 3 || lead.idx == 10 || lead.idx == 11)) {
        writer.push("ئ"); // vowel hiatus uses hamza on yeh, not a new initial alif
        writer.push(text.substr(2));
      } else {
        writer.push(text);
      }
    }
    if (lead.tokenType == TokenType::Consonant) {
      auto mark = unit.vowelMark.idx == InvalidToken ? std::string_view("َ") : map.lookupChar(unit.vowelMark);
      const bool final
          = next == endOfText || HoldsString(next) || GetTokenUnit(next).leadToken.tokenType == TokenType::Symbol;
      if (!final && (unit.vowelMark.idx == 10 || unit.vowelMark.idx == 11))
        mark = "ی";
      if (geminate) {
        // Place shadda on the consonant, before any long-vowel letter.
        if (!mark.empty() && (mark.starts_with("َ") || mark.starts_with("ِ") || mark.starts_with("ُ"))) {
          writer.push(mark.substr(0, 2));
          mark.remove_prefix(2);
        }
        writer.push("ّ");
        geminate = false;
      }
      writer.push(mark);
    }
    if (unit.otherDiacritic.idx != InvalidToken)
      writer.push(map.lookupChar(unit.otherDiacritic));
  }

private:
  bool geminate { false };
};

} // namespace inditrans::handlers
