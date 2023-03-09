#include <cassert>
#include <iostream>
#include <trie.h>

void testStatefulTrie() {
  StatefulTrie<char32_t, std::string> trie;

  auto result = trie.addLookup({ U'न', U'म', U'स', U'्', U'त', U'े' }, "नमस्ते");
  assert(result == std::nullopt);

  result = trie.addLookup({ U'न', U'म', U'स', U'्', U'त', U'े' }, "नमस्ते");
  assert(result != std::nullopt);

  StatefulTrie<char32_t, std::string>::LookupState state {};
  assert(trie.lookup(U'न', state));
  assert(trie.lookup(U'म', state));
  assert(trie.lookup(U'स', state));
  assert(trie.lookup(U'्', state));
  assert(trie.lookup(U'त', state));
  assert(trie.lookup(U'े', state));
  assert(state.value == "नमस्ते");
  assert(state.matchLen == 6);
}

int main() {
  testStatefulTrie();
  return 0;
}
