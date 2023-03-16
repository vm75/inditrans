#include "doctest.h"

#include "trie.h"

TEST_CASE("Testing StatefulTrie") {
  StatefulTrie<char32_t, std::string> trie;

  auto result = trie.addLookup({ U'न', U'म', U'स', U'्', U'त', U'े' }, "नमस्ते");
  CHECK(result == std::nullopt);

  result = trie.addLookup({ U'न', U'म', U'स', U'्', U'त', U'े' }, "नमस्ते");
  CHECK(result == std::nullopt);

  StatefulTrie<char32_t, std::string>::LookupState state {};
  CHECK(trie.lookup(U'न', state));
  CHECK(trie.lookup(U'म', state));
  CHECK(trie.lookup(U'स', state));
  CHECK(trie.lookup(U'्', state));
  CHECK(trie.lookup(U'त', state));
  CHECK(!trie.lookup(U'े', state));
  CHECK(state.value == "नमस्ते");
  CHECK(state.matchLen == 6);
}
