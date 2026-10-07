#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <span>
#include <string_view>
#include <type_traits>

namespace inditrans::static_data {

inline constexpr std::array<uint8_t, 1024> emptyTriplePages {};

template <size_t Capacity> using SmallestIndex = std::conditional_t<(Capacity <= 65535), uint16_t, uint32_t>;

template <typename Key> struct TrieEntry {
  using key_type = Key;
  using KeyView = std::conditional_t<std::is_same_v<Key, uint8_t>, std::string_view, std::span<const Key>>;
  KeyView key;
  uint16_t value;
};

template <typename Index> struct FlatNode {
  Index edges { };
  uint16_t value { }; // Zero is the absent-value sentinel.
  uint16_t count { };
  uint16_t dense { 65535 };
};

template <typename Key, typename Index> struct FlatEdge {
  Key key { };
  Index child { };
};

template <typename Index> struct FlatPath {
  uint32_t bytes { };
  Index child { };
};

// All dictionaries of the same key/index type share this lookup implementation.
template <typename Key, typename Index> struct TrieView {
  static constexpr Index leafBit = Index(uint64_t { 1 } << (std::numeric_limits<Index>::digits - 1));
  const FlatNode<Index>* nodes;
  const FlatEdge<Key, Index>* edges;
  const Index* dense;
  const uint8_t* prefixPages;
  const Index* prefixes;
  const uint8_t* triplePages;
  const Index* triples;
  const FlatPath<Index>* paths;

  constexpr Index next(Index state, Key key) const noexcept {
    const auto& node = nodes[state];
    if constexpr (std::is_same_v<Key, uint8_t>) {
      if (node.dense != 65535 && !(node.count >> 14)) {
        if (node.dense & 0x8000)
          return (key & 0xc0) == 0x80 ? Index(dense[(node.dense & 0x7fff) + (key & 0x3f)] & ~leafBit) : 0;
        return Index(dense[node.dense + key] & ~leafBit);
      }
    }
    const auto* first = edges + node.edges;
    const auto count = std::is_same_v<Key, uint8_t> ? node.count & 0x3fff : node.count;
    if (count == 1)
      return first->key == key ? first->child : 0;
    if (count <= 8) {
      for (size_t i = 0; i < count; ++i)
        if (first[i].key == key)
          return first[i].child;
      return 0;
    }
    const auto* last = first + count;
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

  struct Match {
    uint16_t value { };
    size_t length { };
  };

  template <bool FoldAscii = false, typename Select>
  [[gnu::always_inline]] constexpr Match match(const char* begin, const char* end, Select select) const noexcept
    requires std::is_same_v<Key, uint8_t>
  {
    Match best { };
    if (begin == end)
      return best;
    auto ptr = begin;
    auto key = static_cast<uint8_t>(*ptr++);
    if constexpr (FoldAscii)
      if (key >= 'A' && key <= 'Z')
        key += 'a' - 'A';
    Index state;
    // Fuse the two leading UTF-8 byte transitions. The small ragged pages
    // contain node IDs, not decoded scalars or a second script dictionary.
    if (key >= 0xe0 && key <= 0xef && end - ptr >= 2
        && ((uint16_t(static_cast<uint8_t>(ptr[0])) | (uint16_t(static_cast<uint8_t>(ptr[1])) << 8)) & 0xc0c0) == 0x8080
        && triplePages[size_t(key - 0xe0) * 64 + (static_cast<uint8_t>(ptr[0]) & 0x3f)]) {
      const auto page = triplePages[size_t(key - 0xe0) * 64 + (static_cast<uint8_t>(ptr[0]) & 0x3f)] - 1;
      if constexpr (requires { select.acceptsPrefix(key, uint8_t { }); })
        if (!select.acceptsPrefix(key, static_cast<uint8_t>(ptr[0])))
          return best;
      state = triples[size_t(page) * 64 + (static_cast<uint8_t>(ptr[1]) & 0x3f)];
      ptr += 2;
    } else if (key >= 0xc0 && prefixPages[key - 0xc0] && ptr < end && (static_cast<uint8_t>(*ptr) & 0xc0) == 0x80) {
      const auto page = prefixPages[key - 0xc0] - 1;
      state = prefixes[size_t(page) * 64 + (static_cast<uint8_t>(*ptr++) & 0x3f)];
    } else {
      state = dense[key]; // The root always uses dense table zero.
    }
    while (state) {
      // Dispatch and compressed paths mark terminal leaves, so selection can
      // avoid loading node/edge metadata while retaining an earlier match.
      if (state & leafBit) {
        const auto leaf = Index(state & ~leafBit);
        uint16_t value;
        if constexpr (requires { select(uint16_t { }, leaf); })
          value = select(0, leaf);
        else
          value = select(nodes[leaf].value);
        if (value)
          best = { value, size_t(ptr - begin) };
        break;
      }
      const auto& node = nodes[state];
      // State-indexed selectors carry their own absent-value sentinel; other
      // selectors use the terminal ID. Both retain accepted shorter prefixes.
      uint16_t value = 0;
      if constexpr (requires { select(node.value, state); })
        value = select(node.value, state);
      else if (node.value)
        value = select(node.value);
      if (value)
        best = { value, size_t(ptr - begin) };
      if (!node.count || ptr == end)
        break;
      // Up to three unique edges without intervening terminals are compared
      // together. Ordinary edges provide bounded one-byte fallbacks.
      const auto pathLength = node.count >> 14;
      if (pathLength && size_t(end - ptr) >= pathLength) {
        auto first = static_cast<uint8_t>(ptr[0]);
        auto second = static_cast<uint8_t>(ptr[1]);
        auto third = pathLength == 3 ? static_cast<uint8_t>(ptr[2]) : uint8_t(0);
        if constexpr (FoldAscii) {
          if (first >= 'A' && first <= 'Z')
            first += 'a' - 'A';
          if (second >= 'A' && second <= 'Z')
            second += 'a' - 'A';
          if (third >= 'A' && third <= 'Z')
            third += 'a' - 'A';
        }
        const auto path = paths[node.dense];
        if ((uint32_t(first) | (uint32_t(second) << 8) | (uint32_t(third) << 16)) != path.bytes)
          break;
        state = path.child;
        ptr += pathLength;
        continue;
      }
      key = static_cast<uint8_t>(*ptr++);
      if constexpr (FoldAscii)
        if (key >= 'A' && key <= 'Z')
          key += 'a' - 'A';
      state = next(state, key);
    }
    return best;
  }
};

template <typename Key, typename Index, size_t Nodes, size_t DenseSlots, size_t PrefixTables, size_t TripleTables, size_t PathSlots>
struct FlatTrie {
  std::array<FlatNode<Index>, Nodes> nodes { };
  std::array<FlatEdge<Key, Index>, Nodes - 1> edges { };
  std::array<Index, DenseSlots> dense { };
  std::array<uint8_t, std::is_same_v<Key, uint8_t> ? 64 : 0> prefixPages { };
  std::array<Index, PrefixTables * 64> prefixes { };
  std::array<uint8_t, TripleTables ? 1024 : 0> triplePages { };
  std::array<Index, TripleTables * 64> triples { };
  std::array<FlatPath<Index>, PathSlots> paths { };

  constexpr TrieView<Key, Index> view() const noexcept {
    return { nodes.data(), edges.data(), dense.data(), prefixPages.data(), prefixes.data(), TripleTables ? triplePages.data() : emptyTriplePages.data(),
      triples.data(), paths.data() };
  }
};

struct TrieShape {
  size_t nodes { 1 };
  size_t depth { };
};

// Entries are sorted, unique and nonempty. Adjacent common prefixes count the
// exact topology in O(total key length), without a heap-backed scratch tree.
template <auto const& Entries> consteval TrieShape trieShape() {
  using Key = typename std::remove_cvref_t<decltype(Entries[0])>::key_type;
  TrieShape shape { };
  for (size_t i = 0; i < Entries.size(); ++i) {
    const auto key = Entries[i].key;
    if (key.empty() || Entries[i].value == 0)
      std::abort();
    size_t common = 0;
    if (i) {
      const auto prev = Entries[i - 1].key;
      while (common < prev.size() && common < key.size()
          && static_cast<Key>(prev[common]) == static_cast<Key>(key[common]))
        ++common;
      if (common == key.size()
          || (common < prev.size() && static_cast<Key>(prev[common]) >= static_cast<Key>(key[common])))
        std::abort();
    }
    shape.nodes += key.size() - common;
    if (key.size() > shape.depth)
      shape.depth = key.size();
  }
  return shape;
}

template <auto const& Entries> consteval auto trieScratch() {
  using Key = typename std::remove_cvref_t<decltype(Entries[0])>::key_type;
  constexpr auto shape = trieShape<Entries>();
  struct Node {
    size_t parent { };
    Key key { };
    uint16_t value { };
    size_t count { };
    bool continuationOnly { true };
  };
  std::array<Node, shape.nodes> nodes { };
  std::array<size_t, shape.depth + 1> path { };
  size_t used = 1;
  for (size_t i = 0; i < Entries.size(); ++i) {
    const auto key = Entries[i].key;
    size_t common = 0;
    if (i) {
      const auto prev = Entries[i - 1].key;
      while (common < prev.size() && common < key.size()
          && static_cast<Key>(prev[common]) == static_cast<Key>(key[common]))
        ++common;
    }
    for (size_t j = common; j < key.size(); ++j) {
      nodes[used] = { path[j], static_cast<Key>(key[j]), 0, 0, true };
      auto& parent = nodes[path[j]];
      if constexpr (std::is_same_v<Key, uint8_t>) {
        const bool continuation = (static_cast<uint8_t>(key[j]) & 0xc0) == 0x80;
        if (parent.count == 0)
          parent.continuationOnly = continuation;
        else
          parent.continuationOnly &= continuation;
      }
      ++parent.count;
      path[j + 1] = used++;
    }
    nodes[path[key.size()]].value = Entries[i].value;
  }
  return nodes;
}

// Separate constant evaluations keep full dictionaries within normal compiler
// step limits. These scratch objects are never addressed by runtime data.
template <auto const& Entries> inline constexpr auto trieBuildNodes = trieScratch<Entries>();

template <auto const& Entries, size_t DenseThreshold> consteval size_t denseSlotCount() {
  using Key = typename std::remove_cvref_t<decltype(Entries[0])>::key_type;
  if constexpr (!std::is_same_v<Key, uint8_t>)
    return 0;
  else {
    const auto& scratch = trieBuildNodes<Entries>;
    size_t count = 256; // The byte root always gets direct dispatch.
    for (size_t i = 1; i < scratch.size(); ++i)
      if (scratch[i].count >= DenseThreshold)
        count += scratch[i].continuationOnly ? 64 : 256;
    return count;
  }
}

template <auto const& Entries> consteval size_t prefixTableCount() {
  using Key = typename std::remove_cvref_t<decltype(Entries[0])>::key_type;
  if constexpr (!std::is_same_v<Key, uint8_t>)
    return 0;
  else {
    const auto& scratch = trieBuildNodes<Entries>;
    size_t count = 0;
    for (size_t i = 1; i < scratch.size(); ++i)
      if (scratch[i].parent == 0 && scratch[i].key >= 0xc0 && !scratch[i].value && scratch[i].count)
        ++count;
    return count;
  }
}

template <auto const& Entries> consteval size_t trieIndexCapacity() {
  using Key = typename std::remove_cvref_t<decltype(Entries[0])>::key_type;
  return trieShape<Entries>().nodes * (std::is_same_v<Key, uint8_t> ? 2 : 1);
}

template <auto const& Entries, typename Index = SmallestIndex<trieIndexCapacity<Entries>()>, size_t DenseThreshold = 16>
consteval auto makeStaticTrie() {
  using Key = typename std::remove_cvref_t<decltype(Entries[0])>::key_type;
  constexpr auto shape = trieShape<Entries>();
  constexpr auto denseCount = denseSlotCount<Entries, DenseThreshold>();
  constexpr auto prefixCount = prefixTableCount<Entries>();
  constexpr auto tripleCount = []() consteval {
    if constexpr (!std::is_same_v<Key, uint8_t>)
      return size_t(0);
    else {
      const auto& scratch = trieBuildNodes<Entries>;
      size_t count = 0;
      for (size_t i = 1; i < scratch.size(); ++i) {
        const auto& parent = scratch[scratch[i].parent];
        if (scratch[i].parent && parent.parent == 0 && parent.key >= 0xe0 && parent.key <= 0xef && !parent.value
            && (scratch[i].key & 0xc0) == 0x80 && !scratch[i].value && scratch[i].count)
          ++count;
      }
      return count;
    }
  }();
  static_assert(shape.nodes - 1 <= std::numeric_limits<Index>::max());
  static_assert(!std::is_same_v<Key, uint8_t> || shape.nodes <= TrieView<Key, Index>::leafBit);
  static_assert(denseCount < 32768);
  static_assert(tripleCount < 255);
  const auto& scratch = trieBuildNodes<Entries>;
  constexpr auto pathCount = []() consteval {
    if constexpr (!std::is_same_v<Key, uint8_t>)
      return size_t(0);
    else {
      const auto& scratch = trieBuildNodes<Entries>;
      size_t count = 0;
      for (size_t i = 1; i < scratch.size(); ++i)
        if (scratch[i].value == 0 && scratch[i].count == 1 && scratch[scratch[i].parent].count == 1)
          ++count;
      return count;
    }
  }();
  static_assert(pathCount < 65535);
  FlatTrie<Key, Index, shape.nodes, denseCount, prefixCount, tripleCount, pathCount> result { };
  std::array<size_t, shape.nodes> cursor { };
  size_t offset = 0;
  size_t dense = 0;
  for (size_t i = 0; i < shape.nodes; ++i) {
    if (scratch[i].count > 65535)
      std::abort();
    auto& node = result.nodes[i];
    node.edges = static_cast<Index>(offset);
    node.count = static_cast<uint16_t>(scratch[i].count);
    node.value = scratch[i].value;
    if constexpr (std::is_same_v<Key, uint8_t>)
      if (i == 0 || scratch[i].count >= DenseThreshold) {
        const auto compact = i != 0 && scratch[i].continuationOnly;
        node.dense = static_cast<uint16_t>(dense | (compact ? 0x8000 : 0));
        dense += compact ? 64 : 256;
      }
    offset += scratch[i].count;
  }
  for (size_t i = 1; i < shape.nodes; ++i) {
    const auto parent = scratch[i].parent;
    const auto& node = result.nodes[parent];
    result.edges[node.edges + cursor[parent]++] = { scratch[i].key, static_cast<Index>(i) };
    if constexpr (std::is_same_v<Key, uint8_t>)
      if (node.dense != 65535) {
        const auto key = node.dense & 0x8000 ? scratch[i].key & 0x3f : scratch[i].key;
        result.dense[(node.dense & 0x7fff) + key] = static_cast<Index>(i);
      }
  }
  if constexpr (std::is_same_v<Key, uint8_t>) {
    const auto dispatch = [&](Index child) consteval {
      return Index(child | (result.nodes[child].count == 0 ? TrieView<Key, Index>::leafBit : 0));
    };
    for (auto& child : result.dense)
      if (child)
        child = dispatch(child);
    size_t page = 0;
    for (size_t i = 1; i < shape.nodes; ++i) {
      if (scratch[i].parent != 0 || scratch[i].key < 0xc0 || scratch[i].value || !scratch[i].count)
        continue;
      result.prefixPages[scratch[i].key - 0xc0] = static_cast<uint8_t>(++page);
      const auto& node = result.nodes[i];
      for (size_t j = 0; j < node.count; ++j) {
        const auto& edge = result.edges[node.edges + j];
        if ((edge.key & 0xc0) == 0x80)
          result.prefixes[(page - 1) * 64 + (edge.key & 0x3f)] = dispatch(edge.child);
      }
    }
    page = 0;
    for (size_t i = 1; i < shape.nodes; ++i) {
      const auto& parent = scratch[scratch[i].parent];
      if (!scratch[i].parent || parent.parent != 0 || parent.key < 0xe0 || parent.key > 0xef || parent.value
          || (scratch[i].key & 0xc0) != 0x80 || scratch[i].value || !scratch[i].count)
        continue;
      result.triplePages[size_t(parent.key - 0xe0) * 64 + (scratch[i].key & 0x3f)] = static_cast<uint8_t>(++page);
      const auto& node = result.nodes[i];
      for (size_t j = 0; j < node.count; ++j) {
        const auto& edge = result.edges[node.edges + j];
        if ((edge.key & 0xc0) == 0x80)
          result.triples[(page - 1) * 64 + (edge.key & 0x3f)] = dispatch(edge.child);
      }
    }
    size_t pathIndex = 0;
    for (size_t i = 0; i < shape.nodes; ++i) {
      auto& node = result.nodes[i];
      if (node.count != 1)
        continue;
      const auto first = result.edges[node.edges];
      const auto& child = result.nodes[first.child];
      if (child.value || child.count != 1)
        continue;
      const auto second = result.edges[child.edges];
      auto target = second.child;
      uint32_t bytes = uint32_t(first.key) | (uint32_t(second.key) << 8);
      uint16_t length = 2;
      const auto& grandchild = result.nodes[target];
      if (!grandchild.value && grandchild.count == 1) {
        const auto third = result.edges[grandchild.edges];
        bytes |= uint32_t(third.key) << 16;
        target = third.child;
        length = 3;
      }
      node.dense = static_cast<uint16_t>(pathIndex);
      result.paths[pathIndex++] = { bytes, dispatch(target) };
      node.count = uint16_t((length << 14) | 1);
    }
    if (pathIndex != pathCount)
      std::abort();
  }
  return result;
}

} // namespace inditrans::static_data
