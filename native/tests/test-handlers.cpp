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

  void advance(size_t bytes = 0) noexcept {
    if (bytes == 0) {
      head++;
      return;
    }
    if (head < bufferSize && HoldsString(buffer[head])) {
      const auto str = GetString(buffer[head]);
      if (bytes >= str.size()) {
        head++;
      } else {
        buffer[head] = TokenOrString(str.substr(bytes));
      }
    } else {
      head++;
    }
  }

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
    expect(!AnuswaraAssimilator::canAssimilate(endOfText));

    TokenUnit semiUnit(ScriptToken(TokenType::Consonant, 25 /* y */, ScriptType::Indic));
    expect(AnuswaraAssimilator::inferNasalIndex(semiUnit) == 24u);
    expect(!AnuswaraAssimilator::canAssimilate(semiUnit));

    TokenUnit sibilantUnit(ScriptToken(TokenType::Consonant, 31 /* s */, ScriptType::Indic));
    expect(AnuswaraAssimilator::inferNasalIndex(sibilantUnit) == 24u);
    expect(!AnuswaraAssimilator::canAssimilate(sibilantUnit));

    // Non-consonant tokens (vowels, symbols) with low indices must never trigger nasal inference
    TokenUnit vowelUnit(ScriptToken(TokenType::Vowel, 0 /* a */, ScriptType::Indic));
    expect(AnuswaraAssimilator::inferNasalIndex(vowelUnit) == 24u);
    expect(!AnuswaraAssimilator::canAssimilate(vowelUnit));

    TokenUnit symbolUnit(ScriptToken(TokenType::Symbol, 0 /* '0' */, ScriptType::Indic));
    expect(AnuswaraAssimilator::inferNasalIndex(symbolUnit) == 24u);
    expect(!AnuswaraAssimilator::canAssimilate(symbolUnit));

    TokenUnitOrString strSpan(std::string_view("raw"));
    expect(AnuswaraAssimilator::inferNasalIndex(strSpan) == 24u);
    expect(!AnuswaraAssimilator::canAssimilate(strSpan));
  };

  "Inglish contextual anuswara inference and DisableInference option"_test = [] {
    constexpr auto disabled = TranslitOptions::DisableInference;
    constexpr auto superscripted = TranslitOptions::TamilSuperscripted;

    // Varga stop group assimilation in inglish:
    // Velar stop (k/kh/g/gh) -> n
    expect(transliterate("अंक", "devanagari", "inglish", TranslitOptions::None) == "anka");
    expect(transliterate("अंक", "devanagari", "inglish", disabled) == "aṁka");

    // Palatal stop (ch/CH/j/jh) -> n
    expect(transliterate("अंच", "devanagari", "inglish", TranslitOptions::None) == "ancha");
    expect(transliterate("अंच", "devanagari", "inglish", disabled) == "aṁcha");

    // Retroflex stop (T/TH/D/DH) -> N
    expect(transliterate("अंट", "devanagari", "inglish", TranslitOptions::None) == "aNTa");
    expect(transliterate("अंट", "devanagari", "inglish", disabled) == "aṁTa");
    expect(transliterate("पंडित्", "devanagari", "inglish", TranslitOptions::None) == "paNDit");
    expect(transliterate("पंडित्", "devanagari", "inglish", disabled) == "paṁDit");

    // Dental stop (t/th/d/dh) -> n
    expect(transliterate("अंत", "devanagari", "inglish", TranslitOptions::None) == "anta");
    expect(transliterate("अंत", "devanagari", "inglish", disabled) == "aṁta");

    // Labial stop (p/ph/b/bh) -> m
    expect(transliterate("अंप", "devanagari", "inglish", TranslitOptions::None) == "ampa");
    expect(transliterate("अंप", "devanagari", "inglish", disabled) == "aṁpa");
    expect(transliterate("अंबा", "devanagari", "inglish", TranslitOptions::None) == "ambA");
    expect(transliterate("अंबा", "devanagari", "inglish", disabled) == "aṁbA");

    // Non-varga consonants (sibilants, semivowels, aspirate) retain literal ṁ
    expect(transliterate("संस्कृतम्", "devanagari", "inglish", TranslitOptions::None) == "saṁskr̥tam");
    expect(transliterate("संस्कृतम्", "devanagari", "inglish", disabled) == "saṁskr̥tam");
    expect(transliterate("अंश", "devanagari", "inglish", TranslitOptions::None) == "aṁsha");
    expect(transliterate("अंश", "devanagari", "inglish", disabled) == "aṁsha");
    expect(transliterate("किंवा", "devanagari", "inglish", TranslitOptions::None) == "kiṁvA");
    expect(transliterate("किंवा", "devanagari", "inglish", disabled) == "kiṁvA");

    // End-of-input or word boundary retains literal ṁ
    expect(transliterate("अहं", "devanagari", "inglish", TranslitOptions::None) == "ahaṁ");
    expect(transliterate("अहं", "devanagari", "inglish", disabled) == "ahaṁ");
    expect(transliterate("अं", "devanagari", "inglish", TranslitOptions::None) == "aṁ");
    expect(transliterate("अं", "devanagari", "inglish", disabled) == "aṁ");

    // Chandrabindu remains m̐ (never assimilated)
    expect(transliterate("अँ", "devanagari", "inglish", TranslitOptions::None) == "am̐");
    expect(transliterate("अँ", "devanagari", "inglish", disabled) == "am̐");

    // Lookahead vowel or symbol does not trigger nasal inference
    expect(transliterate("अंअ", "devanagari", "inglish", TranslitOptions::None) == "aṁa");
    expect(transliterate("अं1", "devanagari", "inglish", TranslitOptions::None) == "aṁ1");
    expect(transliterate("अं,क", "devanagari", "inglish", TranslitOptions::None) == "aṁ,ka");

    // Nasal inference does not cross word boundaries or raw-string skip spans
    expect(transliterate("अं क", "devanagari", "inglish", TranslitOptions::None) == "aṁ ka");
    expect(transliterate("अं##क##", "devanagari", "inglish", TranslitOptions::None) == "aṁक");

    // Non-inglish Roman outputs remain unaffected
    expect(transliterate("अंक", "devanagari", "iso", TranslitOptions::None) == "aṁka");
    expect(transliterate("अंक", "devanagari", "itrans", TranslitOptions::None) == "aMka");
    expect(transliterate("अंक", "devanagari", "hk", TranslitOptions::None) == "aMka");
    expect(transliterate("अंक", "devanagari", "iast", TranslitOptions::None) == "aṃka");

    // Explicit Tamil source with TamilSuperscripted automatically disables inference
    expect(transliterate("அம்ʼக¹", "tamil", "inglish", superscripted) == "aṁka");
  };

  "DisableInference option and source-side Tamil superscript behavior"_test = [] {
    constexpr auto disabled = TranslitOptions::DisableInference;
    constexpr auto superscripted = TranslitOptions::TamilSuperscripted;

    // Defaults remain inferred. Explicit opt-out uses the target's literal nasal mapping.
    expect(transliterate("अंक", "devanagari", "tamil", TranslitOptions::None) == "அங்க");
    expect(transliterate("अंक", "devanagari", "tamil", disabled) == "அம்க");

    // TamilSuperscripted is an OUTPUT formatting option here: it must not disable inference.
    expect(transliterate("अंक", "devanagari", "tamil", superscripted) == "அங்க");

    // Explicit DisableInference bypasses Tamil reader voicing even without TamilSuperscripted.
    expect(transliterate("அக¹", "tamil", "devanagari", TranslitOptions::None) == "अग");
    expect(transliterate("அக¹", "tamil", "devanagari", disabled) == "अक");
    expect(transliterate("அக¹", "tamil", "devanagari", superscripted) == "अक");
    expect(transliterate("அக¹", "tamil", "devanagari", disabled | superscripted) == "अक");

    // Tamil positional nasal substitutions are also contextual inference.
    expect(transliterate("अन्", "devanagari", "tamil", TranslitOptions::None) == "அன்");
    expect(transliterate("अन्", "devanagari", "tamil", disabled) == "அந்");

    // Source names are case-insensitive, including automatic writer normalization.
    for (const auto source : { "tamil", "TAMIL", "Tamil" }) {
      expect(transliterate("அந", source, "TaMiL", superscripted) == "அந");
      TranslitBuffer output;
      expect(transliterate("அம்ʼக¹", source, "TaMiL", superscripted, output));
      expect(std::string_view(output.get()) == "அம்க");
    }
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

  "matchTamilMarker helper"_test = [] {
    // All 4 superscripts
    expect(matchTamilMarker("¹")->phonemeOffset == 0u);
    expect(matchTamilMarker("¹")->byteLength == 2u);
    expect(matchTamilMarker("²")->phonemeOffset == 1u);
    expect(matchTamilMarker("²")->byteLength == 2u);
    expect(matchTamilMarker("³")->phonemeOffset == 2u);
    expect(matchTamilMarker("³")->byteLength == 2u);
    expect(matchTamilMarker("⁴")->phonemeOffset == 3u);
    expect(matchTamilMarker("⁴")->byteLength == 3u);

    // All 4 subscripts
    expect(matchTamilMarker("₁")->phonemeOffset == 0u);
    expect(matchTamilMarker("₁")->byteLength == 3u);
    expect(matchTamilMarker("₂")->phonemeOffset == 1u);
    expect(matchTamilMarker("₂")->byteLength == 3u);
    expect(matchTamilMarker("₃")->phonemeOffset == 2u);
    expect(matchTamilMarker("₃")->byteLength == 3u);
    expect(matchTamilMarker("₄")->phonemeOffset == 3u);
    expect(matchTamilMarker("₄")->byteLength == 3u);

    // Markers with trailing text
    expect(matchTamilMarker("² ")->phonemeOffset == 1u);
    expect(matchTamilMarker("² ")->byteLength == 2u);
    expect(matchTamilMarker("₃xyz")->phonemeOffset == 2u);
    expect(matchTamilMarker("₃xyz")->byteLength == 3u);

    // Non-markers
    expect(!matchTamilMarker("").has_value());
    expect(!matchTamilMarker(" ").has_value());
    expect(!matchTamilMarker("a").has_value());
    expect(!matchTamilMarker("5").has_value());
    expect(!matchTamilMarker("⁵").has_value());
    expect(!matchTamilMarker("·").has_value());
  };

  "isTamilMarkerSpan helper"_test = [] {
    // Isolated markers (superscripts and subscripts) must be full marker spans
    expect(isTamilMarkerSpan("¹"));
    expect(isTamilMarkerSpan("²"));
    expect(isTamilMarkerSpan("³"));
    expect(isTamilMarkerSpan("⁴"));
    expect(isTamilMarkerSpan("₁"));
    expect(isTamilMarkerSpan("₂"));
    expect(isTamilMarkerSpan("₃"));
    expect(isTamilMarkerSpan("₄"));

    // Preserved spans with trailing whitespace, punctuation, or text must NOT be full marker spans
    expect(!isTamilMarkerSpan("² "));
    expect(!isTamilMarkerSpan("²  "));
    expect(!isTamilMarkerSpan("²xyz"));
    expect(!isTamilMarkerSpan("²,"));
    expect(!isTamilMarkerSpan("²."));
    expect(!isTamilMarkerSpan("²\n"));
    expect(!isTamilMarkerSpan("₃ "));
    expect(!isTamilMarkerSpan("₃xyz"));
    expect(!isTamilMarkerSpan("⁴ "));

    // Multiple consecutive markers or non-markers
    expect(!isTamilMarkerSpan("²³"));
    expect(!isTamilMarkerSpan(""));
    expect(!isTamilMarkerSpan(" "));
    expect(!isTamilMarkerSpan("a"));
    expect(!isTamilMarkerSpan("5"));
    expect(!isTamilMarkerSpan("⁵"));
    expect(!isTamilMarkerSpan("·"));
  };

  "Tamil writer word-boundary and nasal selection"_test = [] {
    // A. Word-final dental/alveolar nasal selection
    expect(transliterate("अन्", "devanagari", "tamil", TranslitOptions::None) == "அன்");
    expect(transliterate("अन् ", "devanagari", "tamil", TranslitOptions::None) == "அன் ");
    expect(transliterate("अन्,", "devanagari", "tamil", TranslitOptions::None) == "அன்,");
    expect(transliterate("अन्.", "devanagari", "tamil", TranslitOptions::None) == "அன்.");
    expect(transliterate("अन्\n", "devanagari", "tamil", TranslitOptions::None) == "அன்\n");
    expect(transliterate("अन् xyz", "devanagari", "tamil", TranslitOptions::None) == "அன் xyz");
    expect(transliterate("अन्त", "devanagari", "tamil", TranslitOptions::None) == "அந்த");
    expect(transliterate("अन्न", "devanagari", "tamil", TranslitOptions::None) == "அந்ன");

    // B. Preserved spans beginning with a superscript or subscript
    expect(transliterate("अन्² ", "devanagari", "tamil", TranslitOptions::None) == "அன்² ");
    expect(transliterate("अन्²xyz", "devanagari", "tamil", TranslitOptions::None) == "அன்²xyz");
    expect(transliterate("अन्²,", "devanagari", "tamil", TranslitOptions::None) == "அன்²,");
    expect(transliterate("अन्²\n", "devanagari", "tamil", TranslitOptions::None) == "அன்²\n");
    expect(transliterate("अन्⁴ ", "devanagari", "tamil", TranslitOptions::None) == "அன்⁴ ");
    expect(transliterate("अन्₁ ", "devanagari", "tamil", TranslitOptions::None) == "அன்₁ ");
    expect(transliterate("अन्₂ ", "devanagari", "tamil", TranslitOptions::None) == "அன்₂ ");
    expect(transliterate("अन्₃ ", "devanagari", "tamil", TranslitOptions::None) == "அன்₃ ");
    expect(transliterate("अन्₄ ", "devanagari", "tamil", TranslitOptions::None) == "அன்₄ ");
    expect(transliterate("अन्₃xyz", "devanagari", "tamil", TranslitOptions::None) == "அன்₃xyz");

    // C. Marker-only spans (isolated marker retains dental nasal attachment)
    expect(transliterate("अन्¹", "devanagari", "tamil", TranslitOptions::None) == "அந்¹");
    expect(transliterate("अन्²", "devanagari", "tamil", TranslitOptions::None) == "அந்²");
    expect(transliterate("अन्³", "devanagari", "tamil", TranslitOptions::None) == "அந்³");
    expect(transliterate("अन्⁴", "devanagari", "tamil", TranslitOptions::None) == "அந்⁴");
    expect(transliterate("अन्₁", "devanagari", "tamil", TranslitOptions::None) == "அந்₁");
    expect(transliterate("अन्₂", "devanagari", "tamil", TranslitOptions::None) == "அந்₂");
    expect(transliterate("अन्₃", "devanagari", "tamil", TranslitOptions::None) == "அந்₃");
    expect(transliterate("अन्₄", "devanagari", "tamil", TranslitOptions::None) == "அந்₄");
  };

  "TamilReaderHandler superscript and subscript parsing"_test = [] {
    TamilReaderHandler reader;
    ScriptToken ka(TokenType::Consonant, 0, ScriptType::Tamil);

    // Consonant followed by superscript "²"
    VectorStream stream;
    stream.add(TokenOrString(std::string_view("²")));
    auto result = reader.assemble(ka, stream, true, invalidTokenUnit, TranslitOptions::TamilSuperscripted);
    expect(result.leadToken.idx == 1u); // ka + offset 1

    // Subscript ₃ (must be phoneme offset 2 -> idx 2)
    VectorStream streamSub3;
    streamSub3.add(TokenOrString(std::string_view("₃")));
    auto resultSub3 = reader.assemble(ka, streamSub3, true, invalidTokenUnit, TranslitOptions::TamilSuperscripted);
    expect(resultSub3.leadToken.idx == 2u);

    // Subscript ₄ (must be phoneme offset 3 -> idx 3)
    VectorStream streamSub4;
    streamSub4.add(TokenOrString(std::string_view("₄")));
    auto resultSub4 = reader.assemble(ka, streamSub4, true, invalidTokenUnit, TranslitOptions::TamilSuperscripted);
    expect(resultSub4.leadToken.idx == 3u);
  };

  "Tamil superscripts and subscripts production pipeline"_test = [] {
    // A. Superscript recognition (bare consonants, vowel marks, whitespace, punctuation, EOI)
    expect(transliterate("க¹", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "क");
    expect(transliterate("க²", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख");
    expect(transliterate("க³", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ग");
    expect(transliterate("க⁴", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "घ");

    expect(transliterate("கா¹", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "का");
    expect(transliterate("கா²", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "खा");
    expect(transliterate("கா³", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "गा");
    expect(transliterate("கா⁴", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "घा");

    expect(transliterate("கி²", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "खि");
    expect(transliterate("கு³", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "गु");

    expect(transliterate("க¹ ", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "क ");
    expect(transliterate("க² ", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख ");
    expect(transliterate("க³ ", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ग ");
    expect(transliterate("க⁴ ", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "घ ");

    expect(transliterate("க¹.", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "क.");
    expect(transliterate("க².", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख.");
    expect(transliterate("க³.", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ग.");
    expect(transliterate("க⁴.", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "घ.");

    expect(transliterate("க¹,", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "क,");
    expect(transliterate("க²,", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख,");

    // B. Subscript recognition (all 4 subscripts, bare, vowel marks, all eligible consonants + ja)
    expect(transliterate("க₁", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "क");
    expect(transliterate("க₂", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख");
    expect(transliterate("க₃", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ग");
    expect(transliterate("க₄", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "घ");

    expect(transliterate("ச₁", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "च");
    expect(transliterate("ச₂", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "छ");
    expect(transliterate("ச₃", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ज");
    expect(transliterate("ச₄", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "झ");

    expect(transliterate("ட₁", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ट");
    expect(transliterate("ட₂", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ठ");
    expect(transliterate("ட₃", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ड");
    expect(transliterate("ட₄", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ढ");

    expect(transliterate("த₁", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "त");
    expect(transliterate("த₂", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "थ");
    expect(transliterate("த₃", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "द");
    expect(transliterate("த₄", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ध");

    expect(transliterate("ப₁", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "प");
    expect(transliterate("ப₂", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "फ");
    expect(transliterate("ப₃", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ब");
    expect(transliterate("ப₄", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "भ");

    expect(transliterate("ஜ₁", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ज");
    expect(transliterate("ஜ₂", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "झ");
    expect(transliterate("ஜ¹", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ज");
    expect(transliterate("ஜ²", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "झ");

    // C. Lookahead and whitespace boundaries
    expect(transliterate("க² ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख प");
    expect(transliterate("க²  ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख  प");
    expect(transliterate("க²\nப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख\nप");
    expect(transliterate("க²,ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख,प");
    expect(transliterate("க².", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख.");
    expect(transliterate("க²", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख");
    expect(transliterate("க₃ ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ग प");
    expect(transliterate("க₄ ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "घ प");
    expect(transliterate("கு³ ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "गु प");
    expect(transliterate("க²xyz", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "खxyz");

    // D. Option states and combinations
    // When TamilSuperscripted is disabled, intervocalic stop voicing applies:
    expect(transliterate("அக¹", "tamil", "devanagari", TranslitOptions::None) == "अग");
    expect(transliterate("அக¹", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "अक");
    expect(transliterate("ச³", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ज");
    expect(transliterate("ச⁴", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "झ");

    // Vedic accents with superscripts:
    expect(transliterate("க²॒", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख॒");
    expect(transliterate("க²᳚", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख᳚");

    // Destination scripts: Latin (inglish, iast, iso)
    expect(transliterate("க² ப", "tamil", "inglish", TranslitOptions::TamilSuperscripted) == "kha pa");
    expect(transliterate("க² ப", "tamil", "iast", TranslitOptions::TamilSuperscripted) == "kha pa");
    expect(transliterate("க² ப", "tamil", "iso", TranslitOptions::TamilSuperscripted) == "kha pa");

    // E. Preservation behavior
    // Standalone markers without preceding consonant
    expect(transliterate("² ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "² प");
    expect(transliterate("₁ ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "₁ प");
    // Non-eligible consonants: ர (ra) is not a primary stop
    expect(transliterate("ர²", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "र²");
    // XML/HTML tags
    expect(transliterate("க²<span class=\"foo\">text</span>ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted)
        == "ख<span class=\"foo\">text</span>प");
    // Skip delimiters
    expect(transliterate("க²##protected##ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted)
        == "खprotectedप");
    // Unsupported marker combinations on same consonant
    expect(transliterate("க²³ ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख³ प");
    expect(transliterate("க₂₃ ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख₃ प");
    expect(transliterate("க்²³", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख्³");
    // Unsupported markers
    expect(transliterate("க⁵", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "क⁵");
    expect(transliterate("க₀", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "क₀");
    expect(transliterate("ஜ³ ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ज³ प");
    expect(transliterate("ஜ⁴ ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ज⁴ प");
    expect(transliterate("ஜ₃ ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ज₃ प");
    expect(transliterate("ஜ₄ ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ज₄ प");
    // Non-eligible consonants
    expect(transliterate("ல³", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ल³");
    expect(transliterate("ய⁴", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "य⁴");
    expect(transliterate("வ₁", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "व₁");
    expect(transliterate("ஸ₂", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "स₂");
    expect(transliterate("ஹ₃", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ह₃");
    expect(transliterate("ன⁴", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ऩ⁴");

    // F. Marker and virama interactions
    // Marker immediately after consonant with virama
    expect(transliterate("க்¹", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "क्");
    expect(transliterate("க்²", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख्");
    expect(transliterate("க்³", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ग्");
    expect(transliterate("க்⁴", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "घ्");
    expect(transliterate("க்₁", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "क्");
    expect(transliterate("க்₂", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख्");
    expect(transliterate("க்₃", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ग्");
    expect(transliterate("க்₄", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "घ्");
    expect(transliterate("ச்²", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "छ्");
    expect(transliterate("ட்²", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ठ्");
    expect(transliterate("த்²", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "थ्");
    expect(transliterate("ப்²", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "फ्");
    expect(transliterate("ஜ்²", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "झ्");

    // Marker immediately before virama
    expect(transliterate("க²்", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख्");
    expect(transliterate("க³்", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ग्");
    expect(transliterate("க₂்", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख्");
    expect(transliterate("க₃்", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ग्");

    // Additional vowel marks
    expect(transliterate("கே⁴", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "घे");
    expect(transliterate("கொ³", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "गॊ");
    expect(transliterate("கா₂", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "खा");
    expect(transliterate("கி₃", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "गि");
    expect(transliterate("கு₄", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "घु");

    // Vedic accents with markers
    expect(transliterate("கா²॒", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "खा॒");
    expect(transliterate("க₃॒", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ग॒");
    expect(transliterate("க்²॒", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख्॒");

    // Virama marker clusters followed by boundaries
    expect(transliterate("க்² ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख् प");
    expect(transliterate("க்²\nப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख्\nप");
    expect(transliterate("க்²,ப", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख्,प");
    expect(transliterate("க்².", "tamil", "devanagari", TranslitOptions::TamilSuperscripted) == "ख्.");

    // Non-Tamil source script
    expect(transliterate("क² प", "devanagari", "tamil", TranslitOptions::TamilSuperscripted) == "க² ப");
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

  "TamilReaderHandler word-final soft pulli retention"_test = [] {
    TamilReaderHandler reader;
    // Word-final soft consonants (m, n, l, r) with pulli must NOT be dropped
    for (uint8_t idx : { 24 /* m */, 27 /* l */, 36 /* n */ }) {
      ScriptToken token(TokenType::Consonant, idx, ScriptType::Tamil);
      VectorStream stream;
      stream.add(TokenOrString(Virama));

      auto result = reader.assemble(token, stream, false, invalidTokenUnit, TranslitOptions::None);
      expect(result != invalidTokenUnit);
      expect(result.leadToken == token);
      expect(result.vowelMark == Virama);
    }
  };

  "TamilReaderHandler post-nasal stop voicing"_test = [] {
    TamilReaderHandler reader;
    // For each varga stop (k=0, c=5, T=10, t=15, p=20), preceding nasal virama voices it (+2)
    const std::vector<std::pair<uint8_t, uint8_t>> pairs = {
      { 4 /* ṅ */, 0 /* k */ },
      { 9 /* ñ */, 5 /* c */ },
      { 14 /* ṇ */, 10 /* ṭ */ },
      { 19 /* n */, 15 /* t */ },
      { 24 /* m */, 20 /* p */ },
    };
    for (const auto& [nasalIdx, stopIdx] : pairs) {
      TokenUnit nasalUnit(ScriptToken(TokenType::Consonant, nasalIdx, ScriptType::Tamil));
      nasalUnit.vowelMark = Virama;

      ScriptToken stop(TokenType::Consonant, stopIdx, ScriptType::Tamil);
      VectorStream stream;
      auto result = reader.assemble(stop, stream, false, nasalUnit, TranslitOptions::None);
      expect(result.leadToken.idx == static_cast<uint8_t>(stopIdx + 2));
    }
  };

  "TamilReaderHandler prefix boundary prevents intervocalic voicing"_test = [] {
    TamilReaderHandler reader;
    // "அது" prefix: a (vowel 0), tu (consonant 15, vowelMark 4)
    ScriptToken aVowel(TokenType::Vowel, 0, ScriptType::Tamil);
    VectorStream emptyStream;
    reader.assemble(aVowel, emptyStream, true, invalidTokenUnit, TranslitOptions::None);

    ScriptToken taCons(TokenType::Consonant, 15, ScriptType::Tamil);
    VectorStream uStream;
    uStream.add(TokenOrString(ScriptToken(TokenType::VowelMark, 4, ScriptType::Tamil)));
    auto tuUnit = reader.assemble(taCons, uStream, false, TokenUnit(aVowel), TranslitOptions::None);

    // Stem start: pa (consonant 20). Because prefix "அது" was recognized,
    // endOfPrefix is true, so pa stays 20 (p) and is NOT voiced to 22 (b)!
    ScriptToken paCons(TokenType::Consonant, 20, ScriptType::Tamil);
    VectorStream paStream;
    auto stemResult = reader.assemble(paCons, paStream, false, tuUnit, TranslitOptions::None);
    expect(stemResult.leadToken.idx == 20u); // Unvoiced 'p' preserved at prefix boundary
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
