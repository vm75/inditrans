#include "ut.hpp"

#include "trie.h"

using namespace boost::ut;

suite<"Trie tests"> trieTests = [] {
  "Testing StatefulTrie"_test = [] {
    StatefulTrie<char32_t, std::string> trie;

    auto result = trie.addLookup({ U'न', U'म', U'स', U'्', U'त', U'े' }, "नमस्ते");
    expect(result == std::nullopt);

    result = trie.addLookup({ U'न', U'म', U'स', U'्', U'त', U'े' }, "नमस्ते");
    expect(result == std::nullopt);

    StatefulTrie<char32_t, std::string>::LookupState state {};
    expect(trie.lookup(U'न', state));
    expect(trie.lookup(U'म', state));
    expect(trie.lookup(U'स', state));
    expect(trie.lookup(U'्', state));
    expect(trie.lookup(U'त', state));
    expect(!trie.lookup(U'े', state));
    expect(state.value == "नमस्ते");
    expect(state.matchLen == 6);
  };
};