#include "ut.hpp"
#include "pronunciation_pack.h"
#include "urdu_pronunciation.h"
#include <array>

using namespace boost::ut;

suite<"Generic pronunciation lexicon"> pronunciationLexiconTests = [] {
  "Tamil uses the same packed units and reader extension"_test = [] {
    // Independent a, g with inherent a, and final m with explicit virama.
    std::vector<uint8_t> value {1, 0, 3, 0};
    for (unsigned bits : {0u | (22u << 7) | (8u << 12), 24u | (22u << 7) | (8u << 12), 46u | (8u << 12)}) {
      value.push_back(bits); value.push_back(bits >> 8);
    }
    PackedLexicon dictionary;
    const std::array records {PackedLexicon::Record {"அகம்", value}};
    expect(dictionary.build(records));
    std::vector<uint8_t> bytes {'I','U','L','2',0,0,0,0,0,0,0,0};
    bytes.insert(bytes.end(), dictionary.bytes().begin(), dictionary.bytes().end());
    uint32_t crc = 0xffffffffu;
    for (size_t i = 8; i < bytes.size(); ++i) {
      crc ^= bytes[i];
      for (unsigned j = 0; j < 8; ++j) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    crc = ~crc;
    for (unsigned i = 0; i < 4; ++i) bytes[4+i] = crc >> (8*i);
    PronunciationLexiconPack pack;
    expect(pack.load(bytes));
    bytes.clear(); bytes.shrink_to_fit();
    std::string output;
    output.clear();
    expect(transliterateWithLexicon("அகம்", "tamil", "iast", None, pack, output));
    expect(output == "agam");
    output.clear();
    expect(transliterateWithLexicon("அகம்", "tamil", "devanagari", None, pack, output));
    expect(output == "अगम्");
    output.clear();
    expect(transliterateWithLexicon("##அகம்## அகம்", "tamil", "iast", None, pack, output));
    expect(output == "அகம் agam");
    for (const auto text : {"வனம்", "அகம்அகம்", "xஅகம்"}) {
      output.clear();
      expect(transliterateWithLexicon(text, "tamil", "iast", None, pack, output));
      expect(output == transliterate(text, "tamil", "iast", None));
    }
    output.clear();
    expect(transliterateWithLexicon("அகம்", "tamil", "iast", DisableInference, pack, output));
    expect(output == transliterate("அகம்", "tamil", "iast", DisableInference));
    output.clear();
    expect(transliterateWithLexicon("அகம்", "tamil", "tamil", None, pack, output));
    expect(output == "அகம்");
  };
  "script policies retain the original reader storage size"_test = [] {
    expect(sizeof(inditrans::handlers::PronunciationReader<>) == 6 * sizeof(void*));
    expect(sizeof(inditrans::handlers::UrduPronunciationReader) == 6 * sizeof(void*));
  };
};
