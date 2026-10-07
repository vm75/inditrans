#pragma once

#include "script_constants.h"
#include "script_data.h"
#include <optional>

constexpr std::string_view ScriptWriterMap::lookupChar(TokenType type, size_t index) const noexcept {
  const auto category = static_cast<size_t>(type);
  assert(category < charMaps.size());
  if (index >= charMaps[category].size())
    return { };
  const auto entry = charMaps[category][index];
  return { inditrans::static_data::writerText.data() + static_cast<size_t>(entry.offset), entry.length };
}

enum class ReaderPolicy { Roman, FoldedRoman, Indic, Explicit };

namespace inditrans::static_data {
// Build scratch payloads solely to derive prefix acceptance at compile time.
inline constexpr auto sourceNodes = []() consteval {
  std::array<SourceTerminal, readerTrie0.nodes.size()> result { };
  for (size_t i = 0; i < result.size(); ++i)
    if (const auto terminal = readerTrie0.nodes[i].value)
      result[i] = sourceTerminals[terminal - 1];
  return result;
}();

template <auto Member> consteval auto sourceField() {
  using Value = std::remove_cvref_t<decltype(sourceNodes[0].*Member)>;
  std::array<Value, sourceTerminals.size() + 1> result { };
  for (size_t i = 1; i < result.size(); ++i)
    if constexpr (std::is_same_v<Value, Range>)
      result[i] = canonicalRange(sourceTerminals[i - 1].*Member);
    else
      result[i] = sourceTerminals[i - 1].*Member;
  return result;
}

inline constexpr auto sourceMasks = sourceField<&SourceTerminal::sources>();
inline constexpr auto primarySequences = sourceField<&SourceTerminal::sequence>();
inline constexpr auto indicSequences = sourceField<&SourceTerminal::indicSequence>();
inline constexpr auto variantRanges = sourceField<&SourceTerminal::alternatives>();

// Branching states retain parallel payload arrays: following node.value to a
// terminal record would add a dependent load on common Indic characters.
template <auto Member> consteval auto branchField() {
  using Value = std::remove_cvref_t<decltype(sourceTerminals[0].*Member)>;
  std::array<Value, packedReaderTrie0.nodes.size()> result {};
  constexpr auto& ids = packedStateIds<readerTrie0>;
  for (size_t i = 1; i < ids.size(); ++i)
    if (ids[i])
      if (const auto terminal = readerTrie0.nodes[i].value) {
        if constexpr (std::is_same_v<Value, Range>)
          result[ids[i]] = canonicalRange(sourceTerminals[terminal - 1].*Member);
        else
          result[ids[i]] = sourceTerminals[terminal - 1].*Member;
      }
  return result;
}

inline constexpr auto branchMasks = branchField<&SourceTerminal::sources>();
inline constexpr auto branchSequences = branchField<&SourceTerminal::sequence>();
inline constexpr auto branchIndicSequences = branchField<&SourceTerminal::indicSequence>();
inline constexpr auto branchVariants = branchField<&SourceTerminal::alternatives>();

struct IndicSelector {
  constexpr uint16_t operator()(uint16_t terminal) const noexcept { return indicSequences[terminal]; }
  constexpr uint16_t operator()(uint16_t, ReaderIndex state) const noexcept { return branchIndicSequences[state]; }
};

inline constexpr auto sourcePrefixes = []() consteval {
  constexpr auto& scratch = trieBuildNodes<readerEntries0>;
  std::array<uint32_t, readerTrie0.nodes.size()> masks { };
  for (size_t i = 0; i < masks.size(); ++i) {
    masks[i] = sourceNodes[i].sources;
    const auto begin = sourceNodes[i].alternatives.begin;
    const auto count = sourceNodes[i].alternatives.count;
    for (size_t j = 0; j < count; ++j)
      masks[i] |= sourceAlternatives[begin + j].sources;
  }
  for (size_t i = masks.size() - 1; i > 0; --i)
    masks[scratch[i].parent] |= masks[i];
  std::array<uint32_t, 1024> result { };
  for (size_t i = 1; i < masks.size(); ++i) {
    const auto& parent = scratch[scratch[i].parent];
    if (scratch[i].parent && parent.parent == 0 && parent.key >= 0xe0 && parent.key <= 0xef
        && (scratch[i].key & 0xc0) == 0x80)
      result[size_t(parent.key - 0xe0) * 64 + (scratch[i].key & 0x3f)] = masks[i];
  }
  return result;
}();

struct SourceSelector {
  uint32_t mask;
  bool singleSource { };

  constexpr bool acceptsPrefix(uint8_t first, uint8_t second) noexcept {
    const auto accepted = sourcePrefixes[size_t(first - 0xe0) * 64 + (second & 0x3f)];
    singleSource = accepted == mask;
    return accepted & mask;
  }

  template <bool Branch> constexpr uint16_t select(ReaderIndex state) const noexcept {
    const auto* masks = Branch ? branchMasks.data() : sourceMasks.data();
    const auto* sequences = Branch ? branchSequences.data() : primarySequences.data();
    const auto* variants = Branch ? branchVariants.data() : variantRanges.data();
    if (singleSource || (masks[state] & mask))
      return sequences[state];
    const auto range = variants[state];
    for (size_t i = 0; i < range.count; ++i) {
      const auto& variant = sourceAlternatives[range.begin + i];
      if (variant.sources & mask)
        return variant.sequence;
    }
    return 0;
  }

  constexpr uint16_t operator()(uint16_t terminal) const noexcept { return select<false>(terminal); }
  constexpr uint16_t operator()(uint16_t, ReaderIndex state) const noexcept { return select<true>(state); }
};
}

struct ScriptReaderMap {
  using Trie = inditrans::static_data::PackedTrieView<inditrans::static_data::ReaderIndex>;
  const Trie* trie { };
  uint32_t source { };
  bool folded { };
  bool nonRoman { };

  struct LookupResult {
    uint16_t sequence { };
    size_t matchLen { };

    constexpr std::span<const ScriptToken> tokens() const noexcept {
      namespace data = inditrans::static_data;
      constexpr auto mask = (1u << data::sequenceOffsetBits) - 1;
      return { data::sequenceTokens.data() + (sequence & mask), size_t(sequence >> data::sequenceOffsetBits) };
    }
  };

  template <ReaderPolicy Policy>
  [[gnu::always_inline]] constexpr LookupResult lookupToken(const char* begin, const char* end) const noexcept {
    namespace data = inditrans::static_data;
    if constexpr (Policy == ReaderPolicy::Roman || Policy == ReaderPolicy::FoldedRoman) {
      const auto identity = [](uint16_t value) constexpr { return value; };
      const auto match = trie->match<Policy == ReaderPolicy::FoldedRoman>(begin, end, identity);
      return { match.value, match.length };
    } else if constexpr (Policy == ReaderPolicy::Indic) {
      const auto match
          = trie->match(begin, end, data::IndicSelector {});
      return { match.value, match.length };
    } else {
      const auto select = data::SourceSelector { source };
      const auto match = trie->match(begin, end, select);
      return { match.value, match.length };
    }
  }

  constexpr LookupResult lookupToken(const char* begin, const char* end) const noexcept {
    if (nonRoman)
      return source ? lookupToken<ReaderPolicy::Explicit>(begin, end) : lookupToken<ReaderPolicy::Indic>(begin, end);
    return folded ? lookupToken<ReaderPolicy::FoldedRoman>(begin, end) : lookupToken<ReaderPolicy::Roman>(begin, end);
  }
};

namespace inditrans::static_data {

consteval auto makeReaders() {
  std::array<ScriptReaderMap, readerInfo.size() * 2 + 1> result { };
  for (size_t i = 0; i < readerInfo.size(); ++i) {
    const auto info = readerInfo[i];
    result[2 * i] = { &readerTries[info.graph], info.source, false, info.graph == 0 };
    result[2 * i + 1] = { &readerTries[info.foldedGraph], info.source, true, info.foldedGraph == 0 };
  }
  result.back() = { &readerTries[0], 0, false, true };
  return result;
}

inline constexpr auto readers = makeReaders();

constexpr int compareScriptName(std::string_view input, std::string_view name) noexcept {
  const auto size = input.size() < name.size() ? input.size() : name.size();
  for (size_t i = 0; i < size; ++i) {
    auto ch = static_cast<uint8_t>(input[i]);
    if (ch >= 'A' && ch <= 'Z')
      ch += 'a' - 'A';
    const auto other = static_cast<uint8_t>(name[i]);
    if (ch != other)
      return int(ch) - int(other);
  }
  return input.size() < name.size() ? -1 : input.size() > name.size() ? 1 : 0;
}

constexpr uint32_t scriptNameHash(std::string_view name) noexcept {
  if (name.empty())
    return 0;
  const auto lower = [](char byte) constexpr -> uint32_t {
    const auto ch = static_cast<uint8_t>(byte);
    return ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch;
  };
  // Only select a slot here; full name equality below resolves collisions.
  return uint32_t(name.size()) * 131 + lower(name.front()) * 17 + lower(name.back()) * 7
      + lower(name[name.size() / 2]) * 31;
}

inline constexpr auto nameSlots = []() consteval {
  std::array<uint16_t, 128> slots { };
  static_assert(names.size() < slots.size() / 2);
  for (size_t i = 0; i < names.size(); ++i) {
    auto slot = scriptNameHash(names[i].name) % slots.size();
    while (slots[slot])
      slot = (slot + 1) % slots.size();
    slots[slot] = static_cast<uint16_t>(i + 1);
  }
  return slots;
}();

constexpr int findScript(std::string_view name) noexcept {
  auto slot = scriptNameHash(name) % nameSlots.size();
  while (auto id = nameSlots[slot]) {
    const auto& entry = names[id - 1];
    if (name == entry.name || compareScriptName(name, entry.name) == 0)
      return entry.script;
    slot = (slot + 1) % nameSlots.size();
  }
  return -1;
}

} // namespace inditrans::static_data

inline const ScriptReaderMap* getScriptReaderMap(std::string_view script) noexcept {
  namespace data = inditrans::static_data;
  if (isWriteOnlyScript(script))
    return nullptr;
  if (script == "indic")
    return &data::readers.back();
  const auto id = data::findScript(script);
  if (id < 0)
    return nullptr;
#ifdef FORCE_INDIC
  if (isIndicScript(data::writers[id].getType()))
    return &data::readers.back();
#endif
  // Preserve the old name-sensitive folding policy independently of alias
  // resolution: "iso" folds ASCII input, while "ISO" does not.
  return &data::readers[size_t(id) * 2 + size_t(isCaseInsensitiveScripts(script))];
}

inline const ScriptWriterMap* getScriptWriterMap(std::string_view script) noexcept {
  namespace data = inditrans::static_data;
  const auto id = data::findScript(script);
  return id < 0 ? nullptr : &data::writers[id];
}

class TamilPrefixLookup {
public:
  struct LookupState {
    uint16_t node { };
    std::optional<bool> value { };
    size_t matchLen { };
    constexpr void reset() noexcept { *this = { }; }
  };

  constexpr bool lookup(const TokenUnit& token, LookupState& state) const noexcept {
    namespace data = inditrans::static_data;
    auto normalized = token;
    if (token.leadToken.scriptType != ScriptType::Tamil) {
      normalized.leadToken.scriptType = ScriptType::Tamil;
      if (normalized.leadToken.tokenType == TokenType::Consonant && normalized.leadToken.idx <= 24
          && normalized.leadToken.idx % 5 != 4) {
        normalized.leadToken.idx -= normalized.leadToken.idx % 5;
      }
    }
    const auto child = data::tamilTrie.view().next(state.node, data::prefixKey(normalized));
    if (!child)
      return false;
    ++state.matchLen;
    const auto& node = data::tamilTrie.nodes[child];
    if (node.value)
      state.value = true;
    // StatefulTrie retained the prior parent on a terminal leaf or a miss.
    // Keep that behavior for the existing Tamil pronunciation rules.
    if (!node.count)
      return false;
    state.node = child;
    return true;
  }
};
