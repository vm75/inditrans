#pragma once

#include "type_defs.h"
#include <cstdint>
#include <optional>
#include <stddef.h>

#pragma pack(push, 1)

/**
 * @brief Tightly packed binary trie node for zero-copy static table serialization.
 *
 * Designed with 1-byte packing and a flexible array member (`data[0]`) so that
 * precomputed tries can be embedded directly in read-only memory or binary assets.
 * Child transitions are stored in sorted order by key and searched using binary search.
 */
struct TrieNode {
private:
  /**
   * @brief Sorted key-to-child offset transition entry.
   */
  struct NodeArrayEntry {
    uint16_t key; ///< Transition character or token code.
    uint16_t offset; ///< Offset (in 16-bit word units) from trie base to target node.
  };

public:
  /**
   * @brief Binary searches child transitions for a matching key.
   *
   * @param key Transition key to locate.
   * @return Offset to child node if found; 0 if no matching child exists.
   */
  uint16_t find(uint16_t key) const noexcept {
    const NodeArrayEntry* entry = data;
    const NodeArrayEntry* end = entry + count;
    // binary search
    while (entry < end) {
      const NodeArrayEntry* mid = entry + (end - entry) / 2;
      if (mid->key == key) {
        return mid->offset;
      } else if (mid->key < key) {
        entry = mid + 1;
      } else {
        end = mid;
      }
    }

    return 0;
  }

  /**
   * @brief Reconstructs the ScriptToken stored at this terminal leaf node.
   * @return ScriptToken containing decoded tokenType, scriptType, and idx.
   */
  ScriptToken getScriptToken() const noexcept {
    return ScriptToken { static_cast<TokenType>(scriptType), tokenType, static_cast<ScriptType>(idx) };
  }

  const uint8_t isLeaf : 1; ///< 1 if this node represents a valid terminal token.
  const uint8_t scriptType : 3; ///< 3-bit encoded script type discriminator.
  const uint8_t tokenType : 4; ///< 4-bit encoded TokenType.
  const uint8_t idx; ///< Token character index.
  const uint16_t count; ///< Number of outgoing transitions stored in `data`.
  const NodeArrayEntry data[0]; ///< Flexible array member containing sorted child entries.
};

#pragma pack(pop)

/**
 * @brief Zero-copy traverser over a serialized packed binary trie buffer.
 *
 * Operates directly over raw memory without copying or allocating node objects.
 * Supports incremental stateful navigation via `LookupState`.
 */
class CharTrie {
public:
  /**
   * @brief Incremental search state for walking the packed binary trie.
   */
  struct LookupState {
    const TrieNode* node { nullptr }; ///< Pointer to current packed node in memory.
    std::optional<ScriptToken> value { std::nullopt }; ///< Most recently matched ScriptToken.
    size_t matchLen { 0 }; ///< Number of transition keys matched.

    /**
     * @brief Resets lookup cursor to uninitialized state.
     */
    void reset() noexcept {
      node = nullptr;
      value = std::nullopt;
      matchLen = 0;
    }
  };

  /**
   * @brief Constructs a CharTrie view over raw binary data.
   * @param data Pointer to packed binary trie memory buffer.
   */
  constexpr CharTrie(const uint8_t* data) noexcept
      : data(reinterpret_cast<const uint16_t*>(data)) { }

  ~CharTrie() = default;

  /**
   * @brief Steps down the trie using the given key and updates `state`.
   *
   * @param key Transition key to match.
   * @param state Traversal state tracking the active node and matched token.
   * @return true if the transition succeeded and child branches exist;
   *         false if matching failed or the target node has no outgoing edges.
   */
  bool lookup(const uint16_t& key, LookupState& state) const noexcept {
    if (state.node == nullptr) {
      state.node = reinterpret_cast<const TrieNode*>(data);
    }

    auto mapEntry = state.node->find(key);
    if (mapEntry == 0) {
      return false;
    }

    auto lookup = reinterpret_cast<const TrieNode*>(data + mapEntry);

    if (lookup->isLeaf) {
      state.value = lookup->getScriptToken();
      state.matchLen++;
    }

    if (lookup->count == 0) {
      return false;
    }
    state.node = lookup;
    return true;
  }

private:
  const uint16_t* data;
};