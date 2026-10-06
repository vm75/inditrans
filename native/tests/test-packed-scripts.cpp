#include "static_scripts.h"
#include "ut.hpp"

using namespace boost::ut;
namespace data = inditrans::static_data;

namespace {
inline constexpr std::array<data::TrieEntry<uint8_t>, 3> wideEntries { {
    { "a", 32768 },
    { "ab", 40000 },
    { "z", 65535 },
} };
inline constexpr auto wideTrie = data::makeStaticTrie<wideEntries, uint32_t>();
inline constexpr auto packedWideTrie = data::packTrie<wideTrie>();
static_assert(std::is_same_v<decltype(packedWideTrie.nodes[0].edges), uint32_t>);
template <bool Fold = false, class Entries, class Original, class Packed, class Select>
size_t compareMatches(const Entries& entries, const Original& original, const Packed& packed, Select select) {
  size_t checked = 0;
  for (const auto& entry : entries) {
    std::string input { std::string_view(entry.key) };
    if constexpr (Fold)
      for (auto& byte : input)
        if (byte >= 'a' && byte <= 'z')
          byte -= 'a' - 'A';
    input += "!";
    // Include incomplete UTF-8 and compressed paths, and a nonmatching suffix.
    for (size_t length = 0; length <= input.size(); ++length) {
      const auto end = input.data() + length;
      const auto expected = original.template match<Fold>(input.data(), end, select);
      const auto actual = packed.template match<Fold>(input.data(), end, select);
      expect(actual.value == expected.value && actual.length == expected.length);
      ++checked;
    }
  }
  return checked;
}
}

suite<"Packed static script data"> packedScriptsTests = [] {
  "pooled leaf masks preserve every generated source mask"_test = [] {
    expect(data::sourceMaskPool[data::sourceMaskIndices[0]] == 0_u);
    for (size_t i = 0; i < data::sourceTerminals.size(); ++i) {
      expect(data::sourceMaskIndices[i + 1] < data::sourceMaskPool.size());
      expect(data::sourceMaskPool[data::sourceMaskIndices[i + 1]] == data::sourceTerminals[i].sources);
    }
  };
  "three-byte writer entries preserve every offset and length"_test = [] {
    constexpr data::WriterChar boundary { 65535, 255 };
    static_assert(boundary.offset() == 65535 && boundary.length == 255);
    // Every generated literal is also checked by writerChar at compile time.
    for (size_t i = 0; i < data::writerChars.size(); ++i) {
      const auto entry = data::writerChars[i];
      expect(data::writerOffsets[i] == entry.offset());
      expect(data::writerLengths[i] == entry.length);
      expect(size_t(entry.offset()) + entry.length <= data::writerText.size());
      const data::WriterChar roundTrip { entry.offset(), entry.length };
      expect(roundTrip.offsetLow == entry.offsetLow && roundTrip.offsetHigh == entry.offsetHigh
          && roundTrip.length == entry.length);
    }
  };
  "sequence flags preserve all explicit and virtual payloads"_test = [] {
    constexpr auto boundary = data::sequencePayload(data::SourceTerminal { 1, 32767, 0 });
    static_assert(data::explicitSequence(boundary) == 32767 && data::virtualSequence(boundary) == 0);
    for (size_t i = 0; i < data::sourceTerminals.size(); ++i) {
      const auto& expected = data::sourceTerminals[i];
      const auto actual = data::primarySequences[i + 1];
      expect(data::explicitSequence(actual) == expected.sequence);
      expect(data::virtualSequence(actual) == expected.indicSequence);
    }
    for (size_t i = 1; i < data::packedStateIds<data::readerTrie0>.size(); ++i) {
      if (const auto state = data::packedStateIds<data::readerTrie0>[i]) {
        const auto terminal = data::readerTrie0.nodes[i].value;
        const auto expected = terminal ? data::sourceTerminals[terminal - 1] : data::SourceTerminal { };
        const auto actual = data::branchSequences[state];
        expect(data::explicitSequence(actual) == expected.sequence);
        expect(data::virtualSequence(actual) == expected.indicSequence);
      }
    }
  };
  "alternative ranges preserve every generated payload and checked boundaries"_test = [] {
    constexpr data::VariantRange largest { data::Range { 4095, 15 } };
    constexpr data::VariantRange empty { data::Range { 65535, 0 } };
    static_assert(largest.begin() == 4095 && largest.count() == 15);
    static_assert(empty.bits == 0);
    for (size_t i = 0; i < data::sourceTerminals.size(); ++i) {
      const auto expected = data::sourceTerminals[i].alternatives;
      const auto actual = data::variantRanges[i + 1];
      expect(actual.count() == expected.count);
      expect(expected.count ? actual.begin() == expected.begin : actual.bits == 0);
    }
    for (size_t i = 1; i < data::packedStateIds<data::readerTrie0>.size(); ++i) {
      if (const auto state = data::packedStateIds<data::readerTrie0>[i]) {
        const auto terminal = data::readerTrie0.nodes[i].value;
        const auto expected = terminal ? data::sourceTerminals[terminal - 1].alternatives : data::Range { };
        const auto actual = data::branchVariants[state];
        expect(actual.count() == expected.count);
        expect(expected.count ? actual.begin() == expected.begin : actual.bits == 0);
      }
    }
  };
  "every non-Roman source and bounded prefix retains acceptance"_test = [] {
    size_t checked = 0;
    for (size_t source = 0; source < data::readerInfo.size(); ++source) {
      const auto select = [mask = uint32_t(1) << source](uint16_t id) -> uint16_t {
        if (!id)
          return 0;
        const auto& terminal = data::sourceTerminals[id - 1];
        if (terminal.sources & mask)
          return terminal.sequence;
        for (size_t i = 0; i < terminal.alternatives.count; ++i) {
          const auto& alternative = data::sourceAlternatives[terminal.alternatives.begin + i];
          if (alternative.sources & mask)
            return alternative.sequence;
        }
        return 0;
      };
      checked += compareMatches(data::readerEntries0, data::readerTrie0.view(), data::packedReaderTrie0.view(), select);
      if (data::readerInfo[source].graph == 0) {
        const auto& reader = data::readers[source * 2];
        for (const auto& entry : data::readerEntries0) {
          const auto input = entry.key.view();
          for (size_t length = 0; length <= input.size(); ++length) {
            const auto expected = data::readerTrie0.view().match(input.data(), input.data() + length, select);
            const auto actual = reader.lookupToken(input.data(), input.data() + length);
            expect(actual.sequence == expected.value && actual.matchLen == expected.length);
          }
        }
      }
    }
    const auto virtualSelect
        = [](uint16_t id) -> uint16_t { return id ? data::sourceTerminals[id - 1].indicSequence : 0; };
    checked += compareMatches(
        data::readerEntries0, data::readerTrie0.view(), data::packedReaderTrie0.view(), virtualSelect);
    for (const auto& entry : data::readerEntries0) {
      const auto input = entry.key.view();
      for (size_t length = 0; length <= input.size(); ++length) {
        const auto expected = data::readerTrie0.view().match(input.data(), input.data() + length, virtualSelect);
        const auto actual = data::readers.back().lookupToken(input.data(), input.data() + length);
        expect(actual.sequence == expected.value && actual.matchLen == expected.length);
      }
    }
    expect(checked > 200000_u);
  };

  "wide indices keep the leaf marker separate from terminal payloads"_test = [] {
    const auto identity = [](uint16_t id) { return id; };
    expect(compareMatches(wideEntries, wideTrie.view(), packedWideTrie.view(), identity) == 10_u);
  };

  "every Roman graph retains terminals and ASCII folding"_test = [] {
    const auto identity = [](uint16_t id) { return id; };
#define CHECK_GRAPH(N)                                                                                                 \
  compareMatches(data::readerEntries##N, data::readerTrie##N.view(), data::packedReaderTrie##N.view(), identity);      \
  compareMatches<true>(data::readerEntries##N, data::readerTrie##N.view(), data::packedReaderTrie##N.view(), identity)
    CHECK_GRAPH(1);
    CHECK_GRAPH(2);
    CHECK_GRAPH(3);
    CHECK_GRAPH(4);
    CHECK_GRAPH(5);
    CHECK_GRAPH(6);
    CHECK_GRAPH(7);
    CHECK_GRAPH(8);
    CHECK_GRAPH(9);
    CHECK_GRAPH(10);
#undef CHECK_GRAPH
  };
};
