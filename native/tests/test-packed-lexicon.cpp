#include "ut.hpp"
#include "packed_lexicon.h"
#include "urdu_lexicon_pack.h"
#include <cstdio>
#include <array>

using namespace boost::ut;

suite<"Packed lexicon"> packedLexiconTests = [] {
  "generic blocks preserve Unicode keys and opaque values"_test = [] {
    std::vector<std::string> keys;
    const std::vector<uint8_t> value {0, 255, 7};
    for (int i = 0; i < 100; ++i) {
      char suffix[8]; std::snprintf(suffix, sizeof suffix, "%03d", i);
      keys.push_back(std::string("अلف") + suffix);
    }
    std::vector<PackedLexicon::Record> records;
    for (const auto& key : keys) records.push_back({key, value});
    PackedLexicon pack;
    expect(pack.build(records));
    auto compare = [](std::string_view a, std::string_view b) { return a.compare(b); };
    for (const auto& key : keys) expect(std::ranges::equal(pack.find(key, compare), value));
    for (auto key : {"", "अلف", "अلف050a", "अلف999", "zzz"}) expect(pack.find(key, compare).empty());
    auto bytes = std::vector<uint8_t>(pack.bytes().begin(), pack.bytes().end());
    for (size_t n = 0; n < bytes.size(); ++n) {
      PackedLexicon invalid;
      expect(!invalid.load(std::span(bytes).first(n)));
      expect(invalid.empty());
    }
    // A malformed prefix must be rejected independently of checksums.
    size_t pos = 10;
    const size_t chars = bytes[4] | (size_t(bytes[5]) << 8);
    for (size_t i = 0; i < chars; ++i) pos += 1 + bytes[pos];
    bytes[pos] = 1; // A block head cannot reference a previous word.
    PackedLexicon invalid;
    expect(!invalid.load(bytes));
  };
  "alphabet IDs above 127 compare in Unicode order"_test = [] {
    std::vector<std::string> keys;
    const std::array<uint8_t, 1> value {42};
    for (unsigned cp = 0x400; cp < 0x490; ++cp) {
      std::string key; key.push_back(0xc0 | (cp >> 6)); key.push_back(0x80 | (cp & 63));
      keys.push_back(key);
    }
    std::vector<PackedLexicon::Record> records;
    for (const auto& key : keys) records.push_back({key, value});
    PackedLexicon pack; expect(pack.build(records));
    for (const auto& key : keys)
      expect(!pack.find(key, [](auto a, auto b) { return a.compare(b); }).empty());
  };
  "IUL2 uses compact pronunciations through reusable handles"_test = [] {
    // Two alternate readings; only the first is selected, as with IUL1.
    std::vector<uint8_t> value {2,0,3,0};
    for (unsigned bits : {22u | (2u << 7) | (8u << 12), 37u | (1u << 7) | (8u << 12), 44u | (8u << 12)}) {
      value.push_back(bits); value.push_back(bits >> 8);
    }
    value.insert(value.end(), {1,0,0,139}); // independent vowel with no marks
    const std::array records {PackedLexicon::Record {"کتاب", value}};
    PackedLexicon dictionary; expect(dictionary.build(records));
    std::vector<uint8_t> bytes {'I','U','L','2',0,0,0,0,0,0,0,0};
    bytes.insert(bytes.end(), dictionary.bytes().begin(), dictionary.bytes().end());
    auto checksum = [&] {
      uint32_t crc = 0xffffffffu;
      for (size_t i = 8; i < bytes.size(); ++i) {
        crc ^= bytes[i];
        for (int j = 0; j < 8; ++j) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
      }
      crc = ~crc; for (int i = 0; i < 4; ++i) bytes[4+i] = crc >> (8*i);
    };
    checksum();
    void* handle = createUrduLexiconHandle(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    expect(handle != nullptr);
    for (auto text : {"کتاب", "كتاب", "کتاب،کتاب"}) {
      auto* result = transliterateUrduWithLexiconHandle(text, "iast", None, handle, "##", "##");
      expect(result != nullptr);
      if (result) { expect(std::string_view(result) == (std::string_view(text).find("،") == std::string_view::npos ? "kitāb" : "kitāb,kitāb")); releaseBuffer(result); }
    }
    auto* disabled = transliterateUrduWithLexiconHandle("کتاب", "iast", DisableInference, handle, "##", "##");
    expect(disabled != nullptr);
    if (disabled) {
      expect(std::string_view(disabled) == transliterate("کتاب", "urdu", "iast", DisableInference));
      releaseBuffer(disabled);
    }
    auto* protectedText = transliterateUrduWithLexiconHandle("##کتاب## کتاب", "iast", None, handle, "##", "##");
    expect(protectedText != nullptr);
    if (protectedText) { expect(std::string_view(protectedText) == "کتاب kitāb"); releaseBuffer(protectedText); }
    releaseUrduLexiconHandle(handle);
    // Recompute CRC so payload validation, rather than checksum, rejects it.
    bytes.back() = 255; checksum();
    expect(createUrduLexiconHandle(reinterpret_cast<const char*>(bytes.data()), bytes.size()) == nullptr);
  };
};
