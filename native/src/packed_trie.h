#pragma once

#include "static_trie.h"

namespace inditrans::static_data {

// A leaf contains a terminal ID in the dispatch word. Only branching states
// and accepted prefixes with children need a node record.
template <typename Index> struct PackedTrieView {
  static constexpr Index leafBit = TrieView<uint8_t, Index>::leafBit;
  const FlatNode<Index>* nodes;
  const FlatEdge<uint8_t, Index>* edges;
  const Index* dense;
  const uint8_t* prefixPages;
  const Index* prefixes;
  const uint8_t* triplePages;
  const Index* triples;
  const FlatPath<Index>* paths;

  constexpr Index next(Index state, uint8_t key) const noexcept {
    const auto& node = nodes[state];
    if (node.dense != 65535) {
      if (node.dense & 0x8000)
        return (key & 0xc0) == 0x80 ? dense[(node.dense & 0x7fff) + (key & 0x3f)] : 0;
      return dense[node.dense + key];
    }
    const auto* first = edges + node.edges;
    if (node.count == 1)
      return first->key == key ? first->child : 0;
    if (node.count <= 8) {
      for (size_t i = 0; i < node.count; ++i)
        if (first[i].key == key)
          return first[i].child;
      return 0;
    }
    const auto* last = first + node.count;
    while (first < last) {
      const auto* mid = first + (last - first) / 2;
      if (mid->key == key)
        return mid->child;
      if (mid->key < key)
        first = mid + 1;
      else
        last = mid;
    }
    return 0;
  }

  using Match = typename TrieView<uint8_t, Index>::Match;

  template <bool FoldAscii = false, typename Select>
  [[gnu::always_inline]] constexpr Match match(const char* begin, const char* end, Select select) const noexcept {
    Match best {};
    if (begin == end)
      return best;
    auto ptr = begin;
    auto key = static_cast<uint8_t>(*ptr++);
    if constexpr (FoldAscii)
      if (key >= 'A' && key <= 'Z')
        key += 'a' - 'A';
    Index state;
    if (key >= 0xe0 && key <= 0xef && end - ptr >= 2
        && ((uint16_t(static_cast<uint8_t>(ptr[0])) | (uint16_t(static_cast<uint8_t>(ptr[1])) << 8)) & 0xc0c0) == 0x8080
        && triplePages[size_t(key - 0xe0) * 64 + (static_cast<uint8_t>(ptr[0]) & 0x3f)]) {
      const auto page = triplePages[size_t(key - 0xe0) * 64 + (static_cast<uint8_t>(ptr[0]) & 0x3f)] - 1;
      if constexpr (requires { select.acceptsPrefix(key, uint8_t {}); })
        if (!select.acceptsPrefix(key, static_cast<uint8_t>(ptr[0])))
          return best;
      state = triples[size_t(page) * 64 + (static_cast<uint8_t>(ptr[1]) & 0x3f)];
      ptr += 2;
    } else if (key >= 0xc0 && prefixPages[key - 0xc0] && ptr < end && (static_cast<uint8_t>(*ptr) & 0xc0) == 0x80) {
      const auto page = prefixPages[key - 0xc0] - 1;
      state = prefixes[size_t(page) * 64 + (static_cast<uint8_t>(*ptr++) & 0x3f)];
    } else {
      state = dense[key];
    }
    while (state) {
      if (state & leafBit) {
        if (const auto value = select(uint16_t(state & ~leafBit)))
          best = {value, size_t(ptr - begin)};
        break;
      }
      const auto& node = nodes[state];
      if (const auto value = select(node.value))
        best = {value, size_t(ptr - begin)};
      if (ptr == end)
        break;
      const auto pathLength = node.count >> 14;
      if (pathLength) {
        // Intermediate states have no terminals. A truncated or mismatched
        // path cannot add an accepted prefix, so no fallback edges are needed.
        if (size_t(end - ptr) < pathLength)
          break;
        auto first = static_cast<uint8_t>(ptr[0]);
        auto second = static_cast<uint8_t>(ptr[1]);
        auto third = pathLength == 3 ? static_cast<uint8_t>(ptr[2]) : uint8_t(0);
        if constexpr (FoldAscii) {
          if (first >= 'A' && first <= 'Z') first += 'a' - 'A';
          if (second >= 'A' && second <= 'Z') second += 'a' - 'A';
          if (third >= 'A' && third <= 'Z') third += 'a' - 'A';
        }
        const auto path = paths[node.dense];
        if ((uint32_t(first) | (uint32_t(second) << 8) | (uint32_t(third) << 16)) != path.bytes)
          break;
        state = path.child;
        ptr += pathLength;
      } else {
        key = static_cast<uint8_t>(*ptr++);
        if constexpr (FoldAscii)
          if (key >= 'A' && key <= 'Z') key += 'a' - 'A';
        state = next(state, key);
      }
    }
    return best;
  }
};

// Reachability includes every accelerated entry point and ordinary root
// fallback. Compressed paths bypass nonterminal intermediate states.
template <auto const& Trie> consteval auto packedStates() {
  using Index = decltype(Trie.nodes[0].edges);
  constexpr auto leafBit = PackedTrieView<Index>::leafBit;
  constexpr auto count = Trie.nodes.size();
  static_assert(count < leafBit);
  std::array<Index, count> ids {};
  std::array<size_t, count> queue {};
  size_t used = 0;
  const auto mark = [&](auto encoded) consteval {
    const auto state = size_t(encoded & ~leafBit);
    if (state && Trie.nodes[state].count && !ids[state]) {
      ids[state] = static_cast<Index>(++used);
      queue[used - 1] = state;
    }
  };
  for (auto state : Trie.dense) mark(state);
  for (auto state : Trie.prefixes) mark(state);
  for (auto state : Trie.triples) mark(state);
  // The root's direct table is the only runtime entry point to state zero.
  for (size_t cursor = 0; cursor < used; ++cursor) {
    const auto state = queue[cursor];
    const auto& node = Trie.nodes[state];
    if (node.count >> 14) {
      mark(Trie.paths[node.dense].child);
    } else {
      for (size_t i = 0; i < node.count; ++i)
        mark(Trie.edges[node.edges + i].child);
    }
  }
  return ids;
}

template <auto const& Trie> inline constexpr auto packedStateIds = packedStates<Trie>();

template <auto const& Trie> consteval auto packTrie() {
  using Index = decltype(Trie.nodes[0].edges);
  constexpr auto& ids = packedStateIds<Trie>;
  constexpr auto shape = []() consteval {
    std::array<size_t, 3> result {1, 0, 0}; // nodes, sparse edges, paths
    for (size_t i = 0; i < Trie.nodes.size(); ++i) {
      if (i && !packedStateIds<Trie>[i]) continue;
      const auto& n = Trie.nodes[i];
      result[0] += i != 0;
      if (n.count >> 14) ++result[2];
      else if (n.dense == 65535) result[1] += n.count;
    }
    return result;
  }();
  static_assert(shape[0] < PackedTrieView<Index>::leafBit);
  static_assert(shape[1] <= std::numeric_limits<Index>::max());
  static_assert(shape[2] < 65535);
  constexpr size_t nodeCount = shape[0], edgeCount = shape[1], pathCount = shape[2];
  struct Storage {
    std::array<FlatNode<Index>, nodeCount> nodes {};
    std::array<FlatEdge<uint8_t, Index>, edgeCount> edges {};
    std::array<Index, Trie.dense.size()> dense {};
    std::array<uint8_t, 64> prefixPages = Trie.prefixPages;
    std::array<Index, Trie.prefixes.size()> prefixes {};
    decltype(Trie.triplePages) triplePages = Trie.triplePages;
    std::array<Index, Trie.triples.size()> triples {};
    std::array<FlatPath<Index>, pathCount> paths {};

    constexpr PackedTrieView<Index> view() const noexcept {
      return {nodes.data(), edges.data(), dense.data(), prefixPages.data(), prefixes.data(),
          triples.size() ? triplePages.data() : emptyTriplePages.data(), triples.data(), paths.data()};
    }
  } result;
  const auto remap = [&](auto encoded) consteval -> Index {
    const auto state = size_t(encoded & ~PackedTrieView<Index>::leafBit);
    if (!state) return 0;
    if (!Trie.nodes[state].count) {
      if (!Trie.nodes[state].value || Trie.nodes[state].value >= PackedTrieView<Index>::leafBit)
        std::abort();
      return Index(Trie.nodes[state].value | PackedTrieView<Index>::leafBit);
    }
    if (!ids[state]) std::abort();
    return ids[state];
  };
  size_t edge = 0, path = 0;
  for (size_t i = 0; i < Trie.nodes.size(); ++i) {
    if (i && !ids[i]) continue;
    const auto& old = Trie.nodes[i];
    auto& node = result.nodes[ids[i]];
    node = old;
    if (old.count >> 14) {
      const auto& oldPath = Trie.paths[old.dense];
      node.edges = 0;
      node.dense = static_cast<uint16_t>(path);
      result.paths[path++] = {oldPath.bytes, remap(oldPath.child)};
    } else if (old.dense == 65535) {
      node.edges = static_cast<Index>(edge);
      for (size_t j = 0; j < old.count; ++j) {
        const auto& e = Trie.edges[old.edges + j];
        result.edges[edge++] = {e.key, remap(e.child)};
      }
    } else node.edges = 0;
  }
  for (size_t i = 0; i < result.dense.size(); ++i) result.dense[i] = remap(Trie.dense[i]);
  for (size_t i = 0; i < result.prefixes.size(); ++i) result.prefixes[i] = remap(Trie.prefixes[i]);
  for (size_t i = 0; i < result.triples.size(); ++i) result.triples[i] = remap(Trie.triples[i]);
  if (edge != shape[1] || path != shape[2]) std::abort();
  return result;
}

} // namespace inditrans::static_data
