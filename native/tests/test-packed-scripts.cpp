#include "static_scripts.h"
#include "ut.hpp"

using namespace boost::ut;
namespace data = inditrans::static_data;

namespace {
template <bool Fold = false, class Entries, class Original, class Packed, class Select>
size_t compareMatches(const Entries& entries, const Original& original, const Packed& packed, Select select) {
  size_t checked = 0;
  for (const auto& entry : entries) {
    std::string input(entry.key.view());
    if constexpr (Fold)
      for (auto& byte : input)
        if (byte >= 'a' && byte <= 'z') byte -= 'a' - 'A';
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
  "every non-Roman source and bounded prefix retains acceptance"_test = [] {
    size_t checked = 0;
    for (size_t source = 0; source < data::readerInfo.size(); ++source) {
      const auto select = [mask = uint32_t(1) << source](uint16_t id) -> uint16_t {
        if (!id) return 0;
        const auto& terminal = data::sourceTerminals[id - 1];
        if (terminal.sources & mask) return terminal.sequence;
        for (size_t i = 0; i < terminal.alternatives.count; ++i) {
          const auto& alternative = data::sourceAlternatives[terminal.alternatives.begin + i];
          if (alternative.sources & mask) return alternative.sequence;
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
    const auto virtualSelect = [](uint16_t id) -> uint16_t {
      return id ? data::sourceTerminals[id - 1].indicSequence : 0;
    };
    checked += compareMatches(data::readerEntries0, data::readerTrie0.view(), data::packedReaderTrie0.view(), virtualSelect);
    expect(checked > 200000_u);
  };

  "every Roman graph retains terminals and ASCII folding"_test = [] {
    const auto identity = [](uint16_t id) { return id; };
#define CHECK_GRAPH(N) \
    compareMatches(data::readerEntries##N, data::readerTrie##N.view(), data::packedReaderTrie##N.view(), identity); \
    compareMatches<true>(data::readerEntries##N, data::readerTrie##N.view(), data::packedReaderTrie##N.view(), identity)
    CHECK_GRAPH(1); CHECK_GRAPH(2); CHECK_GRAPH(3); CHECK_GRAPH(4); CHECK_GRAPH(5);
    CHECK_GRAPH(6); CHECK_GRAPH(7); CHECK_GRAPH(8); CHECK_GRAPH(9); CHECK_GRAPH(10);
#undef CHECK_GRAPH
  };
};
