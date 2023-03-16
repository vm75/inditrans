#pragma once

#include <cstdint>
#include <optional>
#include <stddef.h>

#pragma pack(push, 1)

struct NodeArrayEntry {
  uint16_t key;
  uint16_t offset;
};

class Node {
public:
  // look for child and return offset
  // if not found, return 0
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

  const uint8_t isLeaf : 1;
  const uint8_t scriptType : 3;
  const uint8_t tokenType : 4;
  const uint8_t idx;
  const uint16_t count;
  const NodeArrayEntry data[0];
};

#pragma pack(pop)

struct Result {
  uint8_t scriptType;
  uint8_t tokenType;
  uint8_t idx;
};

class CharTrie {
public:
  struct LookupState {
    const Node* node { nullptr };
    std::optional<Result> value { std::nullopt };
    size_t matchLen { 0 };

    void reset() noexcept {
      node = nullptr;
      value = std::nullopt;
      matchLen = 0;
    }
  };

  CharTrie(const uint8_t* data)
      : data(data) {
    root = reinterpret_cast<const Node*>(data);
  }

  ~CharTrie() = default;

  bool lookup(const uint16_t& key, LookupState& state) const noexcept {
    if (state.node == nullptr) {
      state.node = root;
    }

    auto mapEntry = state.node->find(key);
    if (mapEntry == 0) {
      return false;
    }

    auto lookup = reinterpret_cast<const Node*>(data + mapEntry);

    if (lookup->isLeaf) {
      state.value = Result { lookup->scriptType, lookup->tokenType, lookup->idx };
      state.matchLen++;
    }

    if (lookup->count == 0) {
      return false;
    }
    state.node = lookup;
    return true;
  }

private:
  const uint8_t* data;
  const Node* root { nullptr };
};