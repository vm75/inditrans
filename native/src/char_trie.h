#pragma once

#include "type_defs.h"
#include <cstdint>
#include <optional>
#include <stddef.h>

#pragma pack(push, 1)

struct TrieNode {
private:
  struct NodeArrayEntry {
    uint16_t key;
    uint16_t offset;
  };

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

  ScriptToken getScriptToken() const noexcept {
    return ScriptToken { static_cast<TokenType>(scriptType), tokenType, static_cast<ScriptType>(idx) };
  }

  const uint8_t isLeaf : 1;
  const uint8_t scriptType : 3;
  const uint8_t tokenType : 4;
  const uint8_t idx;
  const uint16_t count;
  const NodeArrayEntry data[0];
};

#pragma pack(pop)

class CharTrie {
public:
  struct LookupState {
    const TrieNode* node { nullptr };
    std::optional<ScriptToken> value { std::nullopt };
    size_t matchLen { 0 };

    void reset() noexcept {
      node = nullptr;
      value = std::nullopt;
      matchLen = 0;
    }
  };

  constexpr CharTrie(const uint8_t* data) noexcept
      : data(reinterpret_cast<const uint16_t*>(data)) { }

  ~CharTrie() = default;

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