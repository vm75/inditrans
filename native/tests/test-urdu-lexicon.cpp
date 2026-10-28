#include "ut.hpp"
#include "inditrans.h"
#include "urdu_lexicon.h"
#include "urdu_lexicon_pack.h"
#include <array>

using namespace boost::ut;

namespace {
TokenUnit consonant(uint8_t index, uint8_t vowel) {
  TokenUnit result({TokenType::Consonant, index, ScriptType::Indic});
  result.vowelMark = {TokenType::VowelMark, vowel};
  return result;
}

std::string resolve(std::string_view text, std::string_view to, TranslitOptions options, UrduLexicon lexicon) {
  std::string result;
  expect(transliterateUrduWithLexicon(text, to, options, lexicon, result));
  return result;
}

void updateChecksum(std::vector<uint8_t>& bytes) {
  uint32_t crc = 0xffffffffu;
  for (size_t i = 8; i < bytes.size(); ++i) {
    crc ^= bytes[i];
    for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
  }
  crc = ~crc;
  for (size_t i = 0; i < 4; ++i) bytes[4 + i] = uint8_t(crc >> (8 * i));
}
}

suite<"Urdu pronunciation seam"> urduPronunciationTests = [] {
  const std::array reading {consonant(0, 2), consonant(15, 1), consonant(22, 0)};
  const std::array entries {UrduLexiconEntry{"کتاب", reading}};
  "canonical units pass through target writers"_test = [&] {
    expect(resolve("کتاب", "iast", None, entries) == "kitāb");
    expect(resolve("کتاب", "devanagari", None, entries) == "किताब्");
    expect(resolve("كتاب", "iast", None, entries) == "kitāb");
    for (auto target : {"tamil", "telugu", "iso"})
      expect(resolve("کتاب", target, None, entries) == transliterate("किताब्", "devanagari", target, None));
  };
  "fallback and protected spans"_test = [&] {
    for (auto text : {"کتِاب", "xکتاب", "کتابx", "نام", "کتابّ"})
      expect(resolve(text, "iast", None, entries) == transliterate(text, "urdu", "iast", None));
    expect(resolve("کتاب، ##کتاب## <b>کتاب</b>", "iast", None, entries)
        == "kitāb, کتاب <b>kitāb</b>");
    expect(resolve("کتاب", "iast", DisableInference, entries)
        == transliterate("کتاب", "urdu", "iast", DisableInference));
    expect(resolve("کتاب", "urdu", None, entries) == "کتاب");
  };
  "lexical aspiration and nasalization reach the existing writers"_test = [] {
    auto nasal = consonant(24, 11);
    nasal.otherDiacritic = {TokenType::OtherDiacritic, 0};
    const std::array nasalReading {nasal};
    const std::array aspiratedReading {consonant(1, 255), consonant(27, 0)};
    const std::array lexicon {UrduLexiconEntry{"میں", nasalReading}, UrduLexiconEntry{"کھل", aspiratedReading}};
    expect(resolve("میں", "iast", None, lexicon) == transliterate("मेँ", "devanagari", "iast", None));
    expect(resolve("کھل", "devanagari", None, lexicon) == "खल्");
  };
  "Unicode punctuation and boundaries preserve unrecognized text"_test = [&] {
    for (const auto text : {
             "کتاب۔کتاب", "کتاب،کتاب", "کتاب؛کتاب", "کتاب؟کتاب",
             "کتاب کتاب", "کتاب کتاب", "کتاب—کتاب", "«کتاب» کتاب",
             "کتاب…کتاب", "کتاب।کتاب"}) {
      const auto assisted = resolve(text, "iast", None, entries);
      expect(assisted.find("kitāb") != assisted.npos);
      expect(assisted.rfind("kitāb") != assisted.find("kitāb"));
    }
    expect(resolve("کتاب۔کتاب", "iast", None, entries) == "kitāb۔kitāb");
    expect(resolve("کتاب کتاب", "iast", None, entries) == "kitāb kitāb");
    expect(resolve("کتابx", "iast", None, entries) == transliterate("کتابx", "urdu", "iast", None));
  };
  "sorted canonical lexicon rejects invalid and duplicate keys"_test = [&] {
    const std::array duplicate {UrduLexiconEntry{"کتاب", reading}, UrduLexiconEntry{"کتاب", reading}};
    const std::array unsorted {UrduLexiconEntry{"کتاب", reading}, UrduLexiconEntry{"اردو", reading}};
    const std::array alias {UrduLexiconEntry{"كتاب", reading}};
    expect(resolve("کتاب", "iast", None, duplicate) == transliterate("کتاب", "urdu", "iast", None));
    expect(resolve("کتاب", "iast", None, unsorted) == transliterate("کتاب", "urdu", "iast", None));
    expect(resolve("كتاب", "iast", None, alias) == transliterate("كتاب", "urdu", "iast", None));
  };
  "invalid data falls back"_test = [&] {
    const std::array invalidReading {consonant(255, 2)};
    const std::array invalid {UrduLexiconEntry{"کتاب", invalidReading}};
    expect(resolve("کتاب", "iast", None, invalid) == transliterate("کتاب", "urdu", "iast", None));
    for (auto word : {"آب", "ٴب"}) {
      const std::array marked {UrduLexiconEntry{word, reading}};
      expect(resolve(word, "iast", None, marked) == transliterate(word, "urdu", "iast", None));
    }
  };
  "portable pack and truncated pack"_test = [&] {
    std::vector<uint8_t> bytes {'I', 'U', 'L', '1', 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 8, 0, 1, 0};
    std::string_view word = "کتاب";
    bytes.insert(bytes.end(), word.begin(), word.end());
    for (auto b : {3, 0, 1, 0, 2, 255, 1, 15, 1, 255, 1, 22, 0, 255}) bytes.push_back(b);
    updateChecksum(bytes);
    std::string out;
    expect(transliterateUrduWithPack("کتاب", "iast", None, bytes, out));
    expect(out == "kitāb");
    auto duplicate = bytes;
    duplicate[12] = 2;
    duplicate.insert(duplicate.end(), bytes.begin() + 16, bytes.end());
    updateChecksum(duplicate);
    UrduLexiconPack duplicatePack;
    expect(!duplicatePack.load(duplicate));
    expect(duplicatePack.lexicon().empty());
    auto alias = bytes;
    alias[20] = 0xd9;
    alias[21] = 0x83; // Arabic kaf, not the canonical Urdu kaf.
    updateChecksum(alias);
    UrduLexiconPack aliasPack;
    expect(!aliasPack.load(alias));
    expect(aliasPack.lexicon().empty());
    bytes.back() ^= 1;
    UrduLexiconPack corrupted;
    expect(!corrupted.load(bytes));
    expect(corrupted.lexicon().empty());
    bytes.back() ^= 1;
    bytes.pop_back();
    UrduLexiconPack invalid;
    expect(!invalid.load(bytes));
    expect(invalid.lexicon().empty());
    out.clear();
    expect(transliterateUrduWithPack("کتاب", "iast", None, bytes, out));
    expect(out == transliterate("کتاب", "urdu", "iast", None));
  };
};
