#include <memory>
#include <optional>
#include <unordered_map>

template <typename KeyType, typename ValueType> struct TrieNode {
  using NodeMap = std::unordered_map<KeyType, std::unique_ptr<TrieNode<KeyType, ValueType>>>;
  std::optional<ValueType> value { std::nullopt };
  std::unique_ptr<NodeMap> nodes { nullptr };
};

template <typename KeyType, typename ValueType> class StatefulTrie {
public:
  struct LookupState {
    const TrieNode<KeyType, ValueType>* node { nullptr };
    std::optional<ValueType> value { std::nullopt };
    size_t matchLen { 0 };

    void reset() noexcept {
      node = nullptr;
      value = std::nullopt;
      matchLen = 0;
    }
  };

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

  bool lookup(const KeyType& key, LookupState& state) const noexcept {
    if (state.node == nullptr) {
      state.node = &root;
    }

    auto mapEntry = state.node->find(key);
    if (mapEntry == state.node->end()) {
      return false;
    }

    auto& lookup = mapEntry->second;

    if (lookup->value != std::nullopt) {
      state.value = lookup->value;
      state.matchLen++;
    }

    if (!lookup->nodes) {
      return false;
    }
    state.node = lookup->nodes.get();
    return true;
  }

  bool isEmpty() const noexcept {
    return root.empty();
  }

private:
  typename TrieNode<KeyType, ValueType>::NodeMap root {};
};
