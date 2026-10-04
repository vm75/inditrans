#pragma once

#include "static_trie.h"
#include "type_defs.h"
#include <array>
#include <span>

namespace inditrans::static_data {

struct Range {
  uint16_t begin;
  uint16_t count;
};

struct SourceVariant {
  uint32_t sources;
  uint16_t sequence;
};

struct SourceTerminal {
  uint32_t sources;
  uint16_t sequence;
  uint16_t indicSequence;
  Range alternatives;
};

struct ReaderInfo {
  uint16_t graph;
  uint16_t foldedGraph;
  uint32_t source;
};

struct ScriptName {
  std::string_view name;
  uint16_t script;
};

struct WriterChar {
  uint16_t offset;
  uint8_t length;
};

} // namespace inditrans::static_data

// All character-class views are populated by the generator, not bound on use.
struct ScriptWriterMap {
  ScriptType scriptType;
  bool vedic;
  // The ninth, empty slot handles Ignore without a branch on every write.
  std::array<std::span<const inditrans::static_data::WriterChar>, 9> charMaps;

  constexpr ScriptType getType() const noexcept { return scriptType; }
  constexpr bool isVedic() const noexcept { return vedic; }
  constexpr std::string_view lookupChar(TokenType type, size_t index) const noexcept;
  constexpr std::string_view lookupChar(const Token& token) const noexcept {
    return lookupChar(token.tokenType, token.idx);
  }
};
