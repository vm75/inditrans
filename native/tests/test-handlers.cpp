#include "ut.hpp"

#include "contextual_handlers.h"
#include "gurmukhi_handler.h"
#include "indic_handler.h"
#include "inditrans.h"
#include "latin_handler.h"
#include "preservation_policy.h"
#include "script_constants.h"
#include "static_scripts.h"
#include "tamil_handler.h"
#include "token_stream.h"
#include "type_defs.h"
#include <string>
#include <vector>

using namespace boost::ut;
using namespace inditrans;
using namespace inditrans::handlers;

namespace {

struct VectorStream {
  std::vector<TokenOrString> buffer;
  size_t head { 0 };
  size_t bufferSize { 0 };

  void add(TokenOrString t) {
    buffer.push_back(std::move(t));
    bufferSize = buffer.size();
  }

  void ensure(size_t /*n*/) const noexcept {}

  const TokenOrString* peek() noexcept {
    if (head >= bufferSize) return nullptr;
    return &buffer[head];
  }

  void advance() noexcept { head++; }

  bool isEndOfWord() const noexcept {
    if (head >= bufferSize)
      return true;
    const auto& next = buffer[head];
    if (HoldsString(next))
      return true;
    return GetScriptToken(next).tokenType == TokenType::Symbol;
  }
};

} // namespace

suite<"Preservation handler tests"> preservationTests = [] {
  "XML tag preservation"_test = [] {
    std::string text = "<div class=\"foo\">content</div>";
    const auto* map = getScriptReaderMap("devanagari");
    expect(map != nullptr);

    TokenOrString buffer[10];
    size_t outSize = 0;

    const char* ptr = text.data();
    const char* end = ptr + text.size();

    ptr = PreservedSpanHandler::pullUnrecognized<ReaderPolicy::Indic, TokenOrString>(
        ptr, end, *map, "##", "##", TranslitOptions::None, buffer, outSize);

    expect(outSize == 1u);
    expect(GetString(buffer[0]) == "<div class=\"foo\">");
    expect(ptr == text.data() + 17);
  };

  "Protected skip span"_test = [] {
    std::string text = "##skip this text##rest";
    const auto* map = getScriptReaderMap("devanagari");
    expect(map != nullptr);

    TokenOrString buffer[10];
    size_t outSize = 0;

    const char* ptr = text.data();
    const char* end = ptr + text.size();

    ptr = PreservedSpanHandler::pullUnrecognized<ReaderPolicy::Indic, TokenOrString>(
        ptr, end, *map, "##", "##", TranslitOptions::None, buffer, outSize);

    expect(outSize == 1u);
    expect(GetString(buffer[0]) == "skip this text");
    expect(std::string_view(ptr, end - ptr) == "rest");
  };

  "Special marker stripping"_test = [] {
    std::string in = "kʽaʼrˮtˇa";
    std::string out;
    SpecialMarkerFilter::push(in, out, TranslitOptions::None);
    expect(out == "karta");

    std::string outRetained;
    SpecialMarkerFilter::push(in, outRetained, TranslitOptions::RetainSpecialMarkers);
    expect(outRetained == in);
  };
};

suite<"Contextual handler tests"> contextualTests = [] {
  "Anuswara assimilation across vargas"_test = [] {
    // Velar / Guttural (0..3) -> 4 (ङ)
    for (uint8_t i = 0; i < 4; ++i) {
      TokenUnit unit(ScriptToken(TokenType::Consonant, i, ScriptType::Indic));
      expect(AnuswaraAssimilator::inferNasalIndex(unit) == 4u);
    }
    // Palatal (5..8) -> 9 (ञ)
    for (uint8_t i = 5; i < 9; ++i) {
      TokenUnit unit(ScriptToken(TokenType::Consonant, i, ScriptType::Indic));
      expect(AnuswaraAssimilator::inferNasalIndex(unit) == 9u);
    }
    // Retroflex (10..13) -> 14 (ण)
    for (uint8_t i = 10; i < 14; ++i) {
      TokenUnit unit(ScriptToken(TokenType::Consonant, i, ScriptType::Indic));
      expect(AnuswaraAssimilator::inferNasalIndex(unit) == 14u);
    }
    // Dental (15..18) -> 19 (न)
    for (uint8_t i = 15; i < 19; ++i) {
      TokenUnit unit(ScriptToken(TokenType::Consonant, i, ScriptType::Indic));
      expect(AnuswaraAssimilator::inferNasalIndex(unit) == 19u);
    }
    // Labial (20..23) -> 24 (म)
    for (uint8_t i = 20; i < 24; ++i) {
      TokenUnit unit(ScriptToken(TokenType::Consonant, i, ScriptType::Indic));
      expect(AnuswaraAssimilator::inferNasalIndex(unit) == 24u);
    }
    // End of text or non-varga consonant -> default 24 (म)
    expect(AnuswaraAssimilator::inferNasalIndex(endOfText) == 24u);
    TokenUnit semiUnit(ScriptToken(TokenType::Consonant, 25 /* y */, ScriptType::Indic));
    expect(AnuswaraAssimilator::inferNasalIndex(semiUnit) == 24u);
  };

  "ASCII numeral formatting"_test = [] {
    Token numToken(TokenType::Symbol, 5);
    expect(AsciiNumeralFormatter::matches(numToken, TranslitOptions::ASCIINumerals));
    expect(!AsciiNumeralFormatter::matches(numToken, TranslitOptions::None));

    Token highSymbol(TokenType::Symbol, 12);
    expect(!AsciiNumeralFormatter::matches(highSymbol, TranslitOptions::ASCIINumerals));

    std::string out;
    struct DummyWriter {
      std::string& out;
      void push(std::string_view s) { out += s; }
    } writer { out };
    AsciiNumeralFormatter::emit(5, writer);
    expect(out == "5");
  };

  "Tamil traditional replacements"_test = [] {
    expect(TamilTraditionalReplacements::replace("ஸ") == "ச");
    expect(TamilTraditionalReplacements::replace("ஜ") == "ச³");
    expect(TamilTraditionalReplacements::replace("ஜ²") == "ச⁴");
    expect(TamilTraditionalReplacements::replace("க") == "க");
    expect(TamilTraditionalReplacements::replace("anything") == "anything");
  };
};

suite<"Gurmukhi handler tests"> gurmukhiTests = [] {
  "Adhak recognition and expansion"_test = [] {
    expect(GurmukhiAdhakHandler::isAdhak(GurmukhiAdhak));
    expect(!GurmukhiAdhakHandler::isAdhak(Virama));

    // Followed by unaspirated consonant 'k' (idx 0) -> emits k + virama
    ScriptToken k(TokenType::Consonant, 0, ScriptType::Indic);
    TokenOrString peekK(k);
    auto unitK = GurmukhiAdhakHandler::handleAdhak([&]() { return &peekK; });
    expect(unitK.leadToken == k);
    expect(unitK.vowelMark == Virama);

    // Followed by aspirated consonant 'kh' (idx 1) -> de-aspirates to k (idx 0) + virama
    ScriptToken kh(TokenType::Consonant, 1, ScriptType::Indic);
    TokenOrString peekKh(kh);
    auto unitKh = GurmukhiAdhakHandler::handleAdhak([&]() { return &peekKh; });
    expect(unitKh.leadToken.idx == 0u);
    expect(unitKh.vowelMark == Virama);

    // Followed by aspirated voiced 'gh' (idx 3) -> de-aspirates to g (idx 2) + virama
    ScriptToken gh(TokenType::Consonant, 3, ScriptType::Indic);
    TokenOrString peekGh(gh);
    auto unitGh = GurmukhiAdhakHandler::handleAdhak([&]() { return &peekGh; });
    expect(unitGh.leadToken.idx == 2u);
    expect(unitGh.vowelMark == Virama);

    // Followed by non-consonant (e.g. vowel) -> returns invalidTokenUnit
    ScriptToken v(TokenType::Vowel, 0, ScriptType::Indic);
    TokenOrString peekV(v);
    auto unitV = GurmukhiAdhakHandler::handleAdhak([&]() { return &peekV; });
    expect(unitV == invalidTokenUnit);

    // Stream exhausted -> returns invalidTokenUnit
    auto unitNull = GurmukhiAdhakHandler::handleAdhak([]() -> const TokenOrString* { return nullptr; });
    expect(unitNull == invalidTokenUnit);
  };
};

suite<"Tamil handler tests"> tamilTests = [] {
  "Phonological predicates"_test = [] {
    expect(isPrimaryStop(0));  // க
    expect(isPrimaryStop(5));  // ச
    expect(isPrimaryStop(10)); // ட
    expect(isPrimaryStop(15)); // த
    expect(isPrimaryStop(20)); // ப
    expect(!isPrimaryStop(1)); // kh
    expect(!isPrimaryStop(24)); // m

    TokenUnit kaUnit(ScriptToken(TokenType::Consonant, 0, ScriptType::Tamil));
    expect(isHardConsonant(kaUnit));
    TokenUnit raUnit(ScriptToken(TokenType::Consonant, 35, ScriptType::Tamil));
    expect(isHardConsonant(raUnit));

    TokenUnit maUnit(ScriptToken(TokenType::Consonant, 24, ScriptType::Tamil));
    expect(isSoftConsonant(maUnit));
    TokenUnit naUnit(ScriptToken(TokenType::Consonant, 36, ScriptType::Tamil));
    expect(isSoftConsonant(naUnit));

    TokenUnit viramaUnit(ScriptToken(TokenType::Consonant, 0, ScriptType::Tamil));
    viramaUnit.vowelMark = Virama;
    expect(isVirama(viramaUnit));
  };

  "TamilReaderHandler superscript and subscript parsing"_test = [] {
    TamilReaderHandler reader;
    ScriptToken ka(TokenType::Consonant, 0, ScriptType::Tamil);

    // Consonant followed by superscript "²"
    VectorStream stream;
    stream.add(TokenOrString(std::string_view("²")));

    auto result = reader.assemble(ka, stream, true, invalidTokenUnit, TranslitOptions::TamilSuperscripted);
    expect(result.leadToken.idx == 1u); // ka + offset 1
  };

  "TamilReaderHandler ச to ஸ rule"_test = [] {
    TamilReaderHandler reader;
    ScriptToken ca(TokenType::Consonant, 5 /* ச */, ScriptType::Tamil);

    // At sentence/input start (invalidTokenUnit), ச stays ச (5)
    VectorStream stream1;
    auto resultStart = reader.assemble(ca, stream1, true, invalidTokenUnit, TranslitOptions::None);
    expect(resultStart.leadToken.idx == 5u);

    // Intervocalic within word (preceded by vowel, wordStart=false), ச becomes ஸ (31)
    TokenUnit prevVowel(ScriptToken(TokenType::Vowel, 0, ScriptType::Tamil));
    VectorStream stream2;
    auto resultIntervocalic = reader.assemble(ca, stream2, false, prevVowel, TranslitOptions::None);
    expect(resultIntervocalic.leadToken.idx == 31u);

    // At word start preceded by a word ending in vowel mark 3 (e.g. 'i', not in specialVowelDiactritics)
    TokenUnit prevWordEnding(ScriptToken(TokenType::Consonant, 0, ScriptType::Tamil));
    prevWordEnding.vowelMark = Token(TokenType::VowelMark, 3);
    VectorStream stream3;
    auto resultCrossWord = reader.assemble(ca, stream3, true, prevWordEnding, TranslitOptions::None);
    expect(resultCrossWord.leadToken.idx == 31u);
  };

  "TamilReaderHandler intervocalic stop voicing"_test = [] {
    TamilReaderHandler reader;
    ScriptToken ka(TokenType::Consonant, 0 /* க */, ScriptType::Tamil);

    // Preceded by a vowel token (not virama)
    TokenUnit prevVowel(ScriptToken(TokenType::Vowel, 0, ScriptType::Tamil));
    VectorStream stream;

    auto result = reader.assemble(ka, stream, false, prevVowel, TranslitOptions::None);
    expect(result.leadToken.idx == 2u); // voiced க³ (idx + 2)
  };

  "TamilReaderHandler word-final pulli dropping"_test = [] {
    TamilReaderHandler reader;
    ScriptToken ka(TokenType::Consonant, 0 /* க */, ScriptType::Tamil);

    // Attached virama at end of word
    VectorStream stream;
    stream.add(TokenOrString(Virama));

    auto result = reader.assemble(ka, stream, false, invalidTokenUnit, TranslitOptions::None);
    expect(result == invalidTokenUnit); // Word-final primary stop with pulli is dropped
  };
};

suite<"Indic and Latin handler tests"> indicLatinTests = [] {
  "IndicReaderHandler cluster assembly"_test = [] {
    ScriptToken ka(TokenType::Consonant, 0, ScriptType::Indic);
    ScriptToken aa(TokenType::VowelMark, 1, ScriptType::Indic);
    ScriptToken anuswara(TokenType::OtherDiacritic, 1, ScriptType::Indic);

    VectorStream stream;
    stream.add(TokenOrString(aa));
    stream.add(TokenOrString(anuswara));

    auto unit = IndicReaderHandler::assemble(ka, stream);
    expect(unit.leadToken == ka);
    expect(unit.vowelMark == aa);
    expect(unit.otherDiacritic == anuswara);
  };

  "LatinReaderHandler virama synthesis"_test = [] {
    ScriptToken ka(TokenType::Consonant, 0, ScriptType::Latin);
    VectorStream emptyStream;

    auto unit = LatinReaderHandler::assemble(ka, emptyStream);
    expect(unit.leadToken == ka);
    expect(unit.vowelMark.tokenType == TokenType::VowelMark);
    expect(unit.vowelMark.idx == 0u); // Explicit virama synthesized when no vowel follows
  };

  "LatinReaderHandler vowel grouping"_test = [] {
    ScriptToken ka(TokenType::Consonant, 0, ScriptType::Latin);
    ScriptToken iMark(TokenType::Vowel, 2, ScriptType::Latin);

    VectorStream stream;
    stream.add(TokenOrString(iMark));

    auto unit = LatinReaderHandler::assemble(ka, stream);
    expect(unit.leadToken == ka);
    expect(unit.vowelMark.tokenType == TokenType::VowelMark);
    expect(unit.vowelMark.idx == 2u);
  };
};
