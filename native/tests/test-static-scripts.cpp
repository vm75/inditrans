#include "static_scripts.h"
#include "ut.hpp"

using namespace boost::ut;
namespace data = inditrans::static_data;

namespace {
inline constexpr std::array<data::TrieEntry<uint8_t>, 7> testEntries { {
    { "a", 1 },
    { "ab", 2 },
    { "abcdx", 7 },
    { "ac", 3 },
    { "z", 6 },
    { "க", 4 },
    { "க²", 5 },
} };
inline constexpr auto testTrie = data::makeStaticTrie<testEntries>();
static_assert(testTrie.nodes.size() == 13);
static_assert(std::is_same_v<data::SmallestIndex<65535>, uint16_t>);
static_assert(std::is_same_v<data::SmallestIndex<65536>, uint32_t>);

auto lookup(std::string_view source, std::string_view input) {
  return getScriptReaderMap(source)->lookupToken(input.data(), input.data() + input.size());
}
} // namespace

suite<"Static script data"> staticScriptsTests = [] {
  "longest accepted prefix and bounded bytes"_test = [] {
    constexpr std::string_view input = "ab!";
    const auto all = [](uint16_t id) { return id; };
    auto match = testTrie.view().match(input.data(), input.data() + input.size(), all);
    expect(match.value == 2_u && match.length == 2_u);
    match = testTrie.view().match(
        input.data(), input.data() + input.size(), [](uint16_t id) -> uint16_t { return id == 2 ? 0 : id; });
    expect(match.value == 1_u && match.length == 1_u);
    match = testTrie.view().match(input.data(), input.data() + 1, all);
    expect(match.value == 1_u && match.length == 1_u);
    const std::array<char, 2> truncated { char(0xe0), char(0xae) };
    match = testTrie.view().match(truncated.data(), truncated.data() + truncated.size(), all);
    expect(match.value == 0_u && match.length == 0_u);
    constexpr std::string_view path = "ABCDX";
    match = testTrie.view().match<true>(path.data(), path.data() + path.size(), all);
    expect(match.value == 7_u && match.length == 5_u);
    match = testTrie.view().match<true>(path.data(), path.data() + 4, all);
    expect(match.value == 2_u && match.length == 2_u);
    match = testTrie.view().match<true>(
        path.data(), path.data() + path.size(), [](uint16_t id) -> uint16_t { return id == 7 ? 0 : id; });
    expect(match.value == 2_u && match.length == 2_u);
    expect(testTrie.view().match(input.data(), input.data(), all).value == 0_u);
    constexpr std::string_view compound = "க²";
    match = testTrie.view().match(compound.data(), compound.data() + compound.size(), all);
    expect(match.value == 5_u && match.length == 5_u);
    match = testTrie.view().match(compound.data(), compound.data() + 4, all);
    expect(match.value == 4_u && match.length == 3_u);
    match = testTrie.view().match(
        compound.data(), compound.data() + compound.size(), [](uint16_t id) -> uint16_t { return id == 5 ? 0 : id; });
    expect(match.value == 4_u && match.length == 3_u);
    constexpr std::string_view uppercase = "AC";
    match = testTrie.view().match<true>(uppercase.data(), uppercase.data() + uppercase.size(), all);
    expect(match.value == 3_u && match.length == 2_u);
    // Public transitions return ordinary node IDs even for accelerated leaves.
    const auto leaf = testTrie.view().next(0, uint8_t('z'));
    expect(leaf < testTrie.nodes.size() && testTrie.nodes[leaf].value == 6_u);
    constexpr std::string_view rejected = "z";
    match = testTrie.view().match(
        rejected.data(), rejected.data() + rejected.size(), [](uint16_t) -> uint16_t { return 0; });
    expect(match.value == 0_u && match.length == 0_u);
  };

  "one graph, source restrictions, virtual precedence"_test = [] {
    const auto* shared = getScriptReaderMap("indic")->trie;
    for (size_t id = 0; id < data::writers.size(); ++id) {
      if (data::writers[id].scriptType != ScriptType::Latin) {
        expect(data::readers[id * 2].trie == shared);
      }
    }
    expect(lookup("devanagari", "க").tokens().empty());
    const auto tamil = lookup("indic", "க");
    expect(tamil.matchLen == 3_u && tamil.tokens().size() == 1_u);
    expect(tamil.tokens().front() == ScriptToken(TokenType::Consonant, 0, ScriptType::Tamil));
    // Malayalam chillu expansions are accepted explicitly but not by indic.
    const auto chillu = lookup("malayalam", "ൻ");
    expect(chillu.tokens().size() == 2_u && chillu.matchLen == 3_u);
    expect(chillu.tokens().front() == ScriptToken(TokenType::Consonant, 19, ScriptType::Indic));
    expect(chillu.tokens().back() == ScriptToken(TokenType::VowelMark, 0, ScriptType::Indic));
    expect(lookup("indic", "ൻ").tokens().empty());
    // Devanagari equivalents are imported into the virtual reader.
    const auto nukta = lookup("indic", "क़");
    expect(nukta.matchLen == 6_u && nukta.tokens().size() == 1_u);
    expect(nukta.tokens().front().idx == 37_u);
    // The virtual graph's shared joiner payload keeps Devanagari's precedence.
    expect(lookup("indic", "‍").tokens().front().scriptType == ScriptType::Indic);
    expect(lookup("tamil", "‍").tokens().front().scriptType == ScriptType::Tamil);
  };

  "aliases, casing, static writers"_test = [] {
    expect(getScriptWriterMap("HINDI") == getScriptWriterMap("devanagari"));
    expect(getScriptReaderMap("bangla") == getScriptReaderMap("bengali"));
    expect(getScriptReaderMap("unknown") == nullptr);
    expect(getScriptWriterMap("indic") == nullptr);
    expect(getScriptReaderMap("INDIC") == nullptr);
    expect(getScriptReaderMap("readablelatin") == nullptr);
    // Preserve exact source-name sensitivity of Roman ASCII folding.
    const auto lower = lookup("iso", "Kh");
    const auto upper = lookup("ISO", "Kh");
    expect(lower.matchLen == 2_u && lower.tokens().front().idx == 1_u);
    expect(upper.matchLen == 1_u && upper.tokens().front().idx == 0_u);
    const auto* writer = getScriptWriterMap("devanagari");
    expect(writer->lookupChar(TokenType::Consonant, 0) == "क");
    expect(writer->lookupChar(TokenType::Consonant, 255).empty());
    expect(writer->lookupChar(TokenType::Ignore, 0).empty());
    expect(getScriptWriterMap("iso")->lookupChar(TokenType::VowelMark, 0).empty());
  };

  "Tamil prefix state and normalization"_test = [] {
    const auto prefix = TamilPrefixLookup { };
    TamilPrefixLookup::LookupState state { };
    TokenUnit initial { { TokenType::Vowel, 0, ScriptType::Indic } };
    expect(prefix.lookup(initial, state));
    TokenUnit dental { { TokenType::Consonant, 17, ScriptType::Indic } };
    dental.vowelMark = { TokenType::VowelMark, 4 };
    expect(!prefix.lookup(dental, state));
    expect(state.value == true && state.matchLen == 2_u);
    state.reset();
    expect(state.node == 0_u && !state.value && state.matchLen == 0_u);
  };
};
