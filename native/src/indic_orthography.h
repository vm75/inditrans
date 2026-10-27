#pragma once

#include "indic_handler.h"
#include "utf.h"

namespace inditrans::handlers {

/// Narrow Unicode ordering and gemination rules for selected Indic scripts.
/// A byte offset into the existing output allows Thai preposed vowels to move
/// before an arbitrarily long consonant cluster without another text buffer.
class IndicOrthographyHandler {
public:
  void onWordBoundary() noexcept {
    joining = false;
    afterVowel = false;
  }

  template <typename Writer, typename Sink>
  void write(const TokenUnit& unit, const TokenUnitOrString& next, const ScriptWriterMap& map, TranslitOptions options,
      Writer& writer, Sink& sink) noexcept {
    if (map.orthography == IndicOrthography::Plain) {
      IndicWriterHandler::write(unit, map, options, writer);
      return;
    }
    const bool consonant = unit.leadToken.tokenType == TokenType::Consonant;
    if (map.orthography == IndicOrthography::Gurmukhi) {
      const auto following = HoldsString(next) ? invalidTokenUnit : GetTokenUnit(next);
      auto repeatedIndex = following.leadToken.idx;
      // Match the reader's unaspirated first half of k+kh, g+gh, etc.
      if (repeatedIndex < 24)
        repeatedIndex -= repeatedIndex % 5 % 2;
      // Addak replaces the first half of a geminate after a vowel. Leave
      // initial, longer, aspirate+aspirate and marked clusters explicit.
      const bool geminate = afterVowel && consonant && unit.vowelMark.idx == 0
          && unit.otherDiacritic.idx == InvalidToken && unit.accent.idx == InvalidToken
          && following.leadToken.tokenType == TokenType::Consonant
          && following.vowelMark.idx != 0 && unit.leadToken.idx == repeatedIndex;
      afterVowel = (consonant || unit.leadToken.tokenType == TokenType::Vowel) && unit.vowelMark.idx != 0;
      if (geminate)
        writer.push(map.lookupChar(GurmukhiAdhak));
      else
        IndicWriterHandler::write(unit, map, options, writer);
      return;
    }
    const bool continues = consonant && unit.vowelMark.idx == 0 && next != endOfText && !HoldsString(next)
        && GetTokenUnit(next).leadToken.tokenType == TokenType::Consonant;
    if (!joining)
      clusterStart = sink.size();
    auto lead = map.lookupChar(unit.leadToken);
    if (AsciiNumeralFormatter::matches(unit.leadToken, options)) {
      AsciiNumeralFormatter::emit(unit.leadToken.idx, writer);
    } else if (map.orthography == IndicOrthography::Tibetan && joining && consonant && !lead.empty()) {
      const auto cp = Utf8Char(lead.data()).char32().ch;
      if (cp >= 0x0f40 && cp <= 0x0f69) {
        const auto subjoined = cp + 0x50;
        const char bytes[] { char(0xe0 | (subjoined >> 12)), char(0x80 | ((subjoined >> 6) & 63)),
          char(0x80 | (subjoined & 63)) };
        writer.push(std::string_view(bytes, 3));
        writer.push(lead.substr(3));
      } else {
        // Balti extensions have no parallel subjoined forms. Keep an explicit
        // vowel killer instead of turning them into unrelated fixed-form letters.
        writer.push(map.lookupChar(Virama));
        writer.push(lead);
      }
    } else {
      writer.push(lead);
    }
    auto mark = unit.vowelMark.idx == InvalidToken ? std::string_view() : map.lookupChar(unit.vowelMark);
    if (continues) {
      if (map.orthography == IndicOrthography::Tibetan)
        mark = { };
      else if (map.orthography == IndicOrthography::Burmese)
        mark = "္";
      else if (map.orthography == IndicOrthography::Khmer)
        mark = "្";
    }
    if (map.orthography == IndicOrthography::Thai && consonant && !mark.empty()) {
      const auto first = Utf8Char(mark.data()).view();
      if (first == "เ" || first == "แ" || first == "โ" || first == "ไ" || first == "ใ") {
        sink.replace(clusterStart, 0, first);
        mark.remove_prefix(first.size());
      }
    }
    writer.push(mark);
    if (unit.otherDiacritic.idx != InvalidToken)
      writer.push(map.lookupChar(unit.otherDiacritic));
    if (unit.accent.idx != InvalidToken && options / TranslitOptions::IgnoreVedicAccents)
      writer.push(map.lookupChar(unit.accent));
    joining = continues;
  }

private:
  bool joining { false };
  bool afterVowel { false };
  size_t clusterStart { 0 };
};

} // namespace inditrans::handlers
