#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

/**
 * @brief Dynamic heap-allocated node for generic trie structures.
 *
 * Used by `StatefulTrie` and `Char32Trie`. Child nodes are stored in a dynamically
 * allocated hash map that is only instantiated when outgoing edges exist, reducing
 * memory overhead for leaf nodes.
 *
 * @tparam KeyType Transition edge key type (e.g., char32_t, uint16_t).
 * @tparam ValueType Associated value type for terminal nodes.
 */
template <typename KeyType, typename ValueType> struct TrieNode {
  using NodeMap = std::unordered_map<KeyType, std::unique_ptr<TrieNode<KeyType, ValueType>>>;

  std::optional<ValueType> value {
    std::nullopt
  }; ///< Terminal value if this node represents a valid key, else nullopt.
  std::unique_ptr<NodeMap> nodes { nullptr }; ///< Lazily-allocated map of outgoing transitions to child nodes.
};

/**
 * @brief Incremental, step-by-step stateful trie.
 *
 * Allows incremental character-by-character (or token-by-token) traversal of a trie
 * without restarting from the root at each step. Maintains state across successive
 * `lookup` invocations using `LookupState`.
 *
 * @tparam KeyType Type of elements in the key sequence.
 * @tparam ValueType Type of mapped terminal values.
 */
template <typename KeyType, typename ValueType> class StatefulTrie {
public:
  /**
   * @brief Holds incremental traversal state during sequential lookups.
   */
  struct LookupState {
    const typename TrieNode<KeyType, ValueType>::NodeMap* node { nullptr }; ///< Current active node map level.
    std::optional<ValueType> value { std::nullopt }; ///< Deepest matched terminal value seen.
    size_t matchLen { 0 }; ///< Number of consumed keys in current match.

    /**
     * @brief Resets lookup state to the root.
     */
    void reset() noexcept {
      node = nullptr;
      value = std::nullopt;
      matchLen = 0;
    }
  };

  /**
   * @brief Inserts a sequence of keys and associates it with a value.
   *
   * @param keys Vector of key components forming the path.
   * @param value Terminal value to store at the end of the path.
   * @return Previous value if the path already had a conflicting value; nullopt otherwise.
   */
  std::optional<ValueType> addLookup(const std::vector<KeyType>& keys, const ValueType& value) noexcept {
    auto* lookupNode = &root;

    auto iter = keys.begin();
    while (iter != keys.end()) {
      auto nextKey = *iter++;
      auto mapEntry = lookupNode->find(nextKey);
      if (mapEntry == lookupNode->end()) {
        mapEntry = lookupNode->emplace(nextKey, std::make_unique<TrieNode<KeyType, ValueType>>()).first;
      }

      auto& lookup = mapEntry->second;

      if (iter == keys.end()) {
        // last lookupNode is the end
        if (lookup->value != std::nullopt) {
          if (lookup->value != value) {
            return lookup->value;
          }
          continue;
        }
        lookup->value = value;
      } else {
        if (lookup->nodes == nullptr) {
          lookup->nodes = std::make_unique<typename TrieNode<KeyType, ValueType>::NodeMap>();
        }
        lookupNode = lookup->nodes.get();
      }
    }

    return std::nullopt;
  }

  /**
   * @brief Advances incremental lookup state by one key transition.
   *
   * If a transition matching `key` exists, updates `state.value` (if the node is terminal),
   * increments `state.matchLen`, and advances `state.node` to child transitions.
   *
   * @param key The next key element to consume.
   * @param state Traversal state tracking the current node and matches.
   * @return true if the transition succeeded and more child transitions exist;
   *         false if no transition was found or the node has no children.
   */
  bool lookup(const KeyType& key, LookupState& state) const noexcept {
    if (state.node == nullptr) {
      state.node = &root;
    }

    auto mapEntry = state.node->find(key);
    if (mapEntry == state.node->end()) {
      return false;
    }
    state.matchLen++;

    auto& lookup = mapEntry->second;

    if (lookup->value != std::nullopt) {
      state.value = lookup->value;
    }

    if (!lookup->nodes) {
      return false;
    }
    state.node = lookup->nodes.get();
    return true;
  }

  /**
   * @brief Checks if the trie contains any entries.
   * @return true if the root node map is empty; false otherwise.
   */
  bool isEmpty() const noexcept { return root.empty(); }

private:
  typename TrieNode<KeyType, ValueType>::NodeMap root { };
};
