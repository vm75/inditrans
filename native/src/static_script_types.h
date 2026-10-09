#pragma once

#include "packed_trie.h"
#include "type_defs.h"
#include <algorithm>
#include <array>
#include <span>
#include <type_traits>

namespace inditrans::static_data {

/// Represents a bounded 16-bit half-open range [begin, begin + count).
struct Range {
  uint16_t begin; ///< Starting index
  uint16_t count; ///< Number of elements
};

/// Normalizes empty ranges to begin=0 at compile time while preserving layout.
consteval Range canonicalRange(Range range) { return { range.count ? range.begin : uint16_t(0), range.count }; }

/// Generates a compile-time bitmask combining multiple script IDs.
template <typename... Args> constexpr uint32_t scriptMask(Args... ids) noexcept {
  return ((1u << static_cast<uint32_t>(ids)) | ...);
}

/// Represents an alternative token sequence emitted when matching from specific source scripts.
struct SourceVariant {
  uint32_t sources { }; ///< Bitmask of source scripts for which this variant applies
  uint16_t sequence { }; ///< Token sequence ID emitted for matching sources

  constexpr SourceVariant() = default;
  constexpr SourceVariant(uint32_t src, uint16_t seq) noexcept
      : sources(src)
      , sequence(seq) { }
};

/// Terminal trie record linking an input spelling to its primary and alternative token expansions.
struct SourceTerminal {
  uint32_t sources { }; ///< Source scripts using the primary sequence; alternatives have their own masks
  uint16_t sequence { }; ///< Primary emitted token sequence ID
  uint16_t indicSequence { }; ///< Emitted sequence ID for the virtual Indic reader
  Range alternatives { }; ///< Range of script-specific SourceVariant alternatives

  constexpr SourceTerminal() = default;
  constexpr SourceTerminal(uint32_t src, uint16_t seq, uint16_t indicSeq = 0, Range alt = { }) noexcept
      : sources(src)
      , sequence(seq)
      , indicSequence(indicSeq)
      , alternatives(alt) { }
};

/// Descriptor mapping a script to its trie indices and source bitmask.
struct ReaderInfo {
  uint16_t graph; ///< Index of the standard reader trie
  uint16_t foldedGraph; ///< Index of the case-folded reader trie
  uint32_t source; ///< Bitmask identifying this script in shared multi-script tries
};

/// Associative mapping between a script name string and its internal script index.
struct ScriptName {
  std::string_view name; ///< Canonical name or alias
  uint16_t script; ///< Internal script identifier
};

/// Compact representation of a target grapheme within the pooled UTF-8 writer text.
struct WriterChar {
  uint16_t offset { }; ///< Byte offset into the shared writerText pool
  uint8_t length { }; ///< Byte length of the target grapheme

  constexpr WriterChar() = default;
  constexpr WriterChar(uint16_t off, uint8_t len) noexcept
      : offset(off)
      , length(len) { }
};

/// Fixed-size compile-time UTF-8 string buffer for pooled text storage.
template <size_t N> struct PackedUtf8 {
  char bytes[N] { };

  consteval PackedUtf8() = default;

  consteval PackedUtf8(const char8_t (&str)[N]) {
    for (size_t i = 0; i < N; ++i)
      bytes[i] = static_cast<char>(str[i]);
  }

  constexpr const char* operator+(size_t offset) const noexcept { return bytes + offset; }
  constexpr const char* data() const noexcept { return bytes; }
  constexpr operator const char*() const noexcept { return bytes; }
  constexpr size_t size() const noexcept { return N > 0 ? N - 1 : 0; }
  constexpr char operator[](size_t i) const noexcept { return bytes[i]; }
};

template <size_t N> consteval auto packUtf8(const char8_t (&str)[N]) { return PackedUtf8<N>(str); }

struct Utf8Key {
  char bytes[16] { };
  uint8_t len { };

  consteval Utf8Key() = default;

  template <size_t M>
  consteval Utf8Key(const char8_t (&str)[M])
      : len(static_cast<uint8_t>(M - 1)) {
    static_assert(M <= 16, "Key exceeds capacity");
    for (size_t i = 0; i < M - 1; ++i)
      bytes[i] = static_cast<char>(str[i]);
  }

  consteval Utf8Key(const char* str) {
    size_t i = 0;
    while (str[i] != '\0') {
      bytes[i] = str[i];
      ++i;
    }
    len = static_cast<uint8_t>(i);
  }

  constexpr bool empty() const noexcept { return len == 0; }
  constexpr size_t size() const noexcept { return len; }
  constexpr const char* data() const noexcept { return bytes; }
  constexpr char operator[](size_t i) const noexcept { return bytes[i]; }
  constexpr operator std::string_view() const noexcept { return { bytes, len }; }
  constexpr std::string_view view() const noexcept { return { bytes, len }; }
};

constexpr ScriptToken vowel(uint8_t idx, ScriptType script = ScriptType::Indic) noexcept {
  return { TokenType::Vowel, idx, script };
}
constexpr ScriptToken vowelMark(uint8_t idx, ScriptType script = ScriptType::Indic) noexcept {
  return { TokenType::VowelMark, idx, script };
}
constexpr ScriptToken consonant(uint8_t idx, ScriptType script = ScriptType::Indic) noexcept {
  return { TokenType::Consonant, idx, script };
}
constexpr ScriptToken otherDiacritic(uint8_t idx, ScriptType script = ScriptType::Indic) noexcept {
  return { TokenType::OtherDiacritic, idx, script };
}
constexpr ScriptToken accent(uint8_t idx, ScriptType script = ScriptType::Indic) noexcept {
  return { TokenType::Accent, idx, script };
}
constexpr ScriptToken symbol(uint8_t idx, ScriptType script = ScriptType::Indic) noexcept {
  return { TokenType::Symbol, idx, script };
}
constexpr ScriptToken vedicSymbol(uint8_t idx, ScriptType script = ScriptType::Indic) noexcept {
  return { TokenType::VedicSymbol, idx, script };
}
constexpr ScriptToken exclusiveSymbol(uint8_t idx, ScriptType script = ScriptType::Indic) noexcept {
  return { TokenType::ExclusiveSymbol, idx, script };
}

template <size_t N> struct TokenSequence {
  std::array<ScriptToken, N> tokens { };

  constexpr size_t size() const noexcept { return N; }
  constexpr const ScriptToken& operator[](size_t i) const noexcept { return tokens[i]; }

  constexpr bool operator==(const TokenSequence& other) const noexcept {
    for (size_t i = 0; i < N; ++i)
      if (tokens[i] != other.tokens[i])
        return false;
    return true;
  }
};

template <typename... Tokens> constexpr auto seq(Tokens... ts) noexcept {
  return TokenSequence<sizeof...(Tokens)> { { ts... } };
}

template <size_t TotalTokens, size_t TotalSequences> struct SequencePool {
  std::array<ScriptToken, TotalTokens> tokens { };
  std::array<Range, TotalSequences> ranges { };
  constexpr auto data() const noexcept { return tokens.data(); }
  constexpr size_t size() const noexcept { return TotalTokens; }
  constexpr const ScriptToken& operator[](size_t i) const noexcept { return tokens[i]; }
};

template <typename T> struct SequenceLength;

template <size_t N> struct SequenceLength<TokenSequence<N>> : std::integral_constant<size_t, N> { };

template <typename... Seqs> consteval size_t sequencePoolTokenCount() {
  const std::array<size_t, sizeof...(Seqs)> sizes { SequenceLength<std::remove_cvref_t<Seqs>>::value... };
  size_t total = 1;
  for (const auto size : sizes)
    total += size;
  return total;
}

template <typename... Seqs> consteval bool sequenceLengthsFit() {
  for (const auto size : std::array<size_t, sizeof...(Seqs)> { SequenceLength<std::remove_cvref_t<Seqs>>::value... })
    if (size >= (1u << (16 - 12)))
      return false;
  return true;
}

template <typename F> constexpr void forEachSequence(F&) { }

template <typename F, typename First, typename... Rest>
constexpr void forEachSequence(F& f, const First& first, const Rest&... rest) {
  f(first);
  if constexpr (sizeof...(Rest) > 0)
    forEachSequence(f, rest...);
}

template <typename... Seqs> consteval auto makeSequencePool(const Seqs&... seqs) {
  constexpr size_t total = sequencePoolTokenCount<Seqs...>();
  static_assert(total <= (1u << 12), "Total sequence tokens exceeds offset capacity");
  static_assert(sequenceLengthsFit<Seqs...>(), "Sequence length exceeds length capacity");
  SequencePool<total, sizeof...(Seqs)> pool;
  // Offset zero is reserved so an encoded sequence ID of zero means no match.
  pool.tokens[0] = ScriptToken(TokenType::Ignore, 255, ScriptType::Others);
  size_t offset = 1;
  size_t sequence = 0;
  const auto add = [&]<size_t N>(const TokenSequence<N>& s) {
    pool.ranges[sequence++] = { static_cast<uint16_t>(offset), static_cast<uint16_t>(N) };
    for (size_t i = 0; i < N; ++i)
      pool.tokens[offset++] = s[i];
  };
  forEachSequence(add, seqs...);
  return pool;
}

struct SemanticMapping {
  Utf8Key key;
  ScriptToken tokens[4] { };
  uint8_t count { };

  consteval SemanticMapping() = default;

  template <size_t N>
  consteval SemanticMapping(Utf8Key k, const TokenSequence<N>& s)
      : key(k)
      , count(static_cast<uint8_t>(N)) {
    static_assert(N <= 4, "Sequence exceeds maximum mapping length");
    for (size_t i = 0; i < N; ++i)
      tokens[i] = s[i];
  }

  consteval SemanticMapping(Utf8Key k, ScriptToken t)
      : key(k)
      , count(1) {
    tokens[0] = t;
  }
};

struct ReaderEntry {
  using key_type = uint8_t;
  Utf8Key key;
  uint16_t value;
};

struct TokenLookupTable {
  uint16_t single[4][9][64] { };

  consteval TokenLookupTable() = default;

  consteval TokenLookupTable(const auto& tokens, unsigned seqBits) {
    for (size_t i = 1; i < tokens.size(); ++i) {
      const auto& t = tokens[i];
      auto s = static_cast<size_t>(t.scriptType);
      auto type = static_cast<size_t>(t.tokenType);
      auto idx = static_cast<size_t>(t.idx);
      if (s < 4 && type < 9 && idx < 64 && single[s][type][idx] == 0) {
        single[s][type][idx] = static_cast<uint16_t>(i | (1u << seqBits));
      }
    }
  }

  constexpr uint16_t lookup(ScriptToken t) const noexcept {
    auto s = static_cast<size_t>(t.scriptType);
    auto type = static_cast<size_t>(t.tokenType);
    auto idx = static_cast<size_t>(t.idx);
    if (s < 4 && type < 9 && idx < 64)
      return single[s][type][idx];
    return 0;
  }
};

template <size_t M>
consteval std::array<ReaderEntry, M> deriveReaderEntries(
    const auto& seqTokens, unsigned seqBits, const std::array<SemanticMapping, M>& mappings) {
  TokenLookupTable table(seqTokens, seqBits);
  std::array<ReaderEntry, M> entries { };
  for (size_t m = 0; m < M; ++m) {
    const auto& map = mappings[m];
    const size_t N = map.count;
    uint16_t id = 0;
    if (N == 1) {
      id = table.lookup(map.tokens[0]);
    }
    if (id == 0) {
      // A mapping may reuse any contiguous token span, including part of an
      // expansion; only canonical standalone IDs require sequence boundaries.
      for (size_t i = 1; i + N <= seqTokens.size(); ++i) {
        bool match = true;
        for (size_t j = 0; j < N; ++j) {
          if (seqTokens[i + j] != map.tokens[j]) {
            match = false;
            break;
          }
        }
        if (match) {
          id = static_cast<uint16_t>(i | (N << seqBits));
          break;
        }
      }
    }
    if (id == 0)
      std::abort();
    entries[m] = { map.key, id };
  }
  return entries;
}

// These source records and pool checks are evaluated only during compilation.
struct SourceEntry {
  Utf8Key key;
  SourceTerminal terminal;
};

template <size_t N> consteval auto sourcePayloads(const std::array<SourceEntry, N>& mappings) {
  std::array<SourceTerminal, N> result { };
  for (size_t i = 0; i < N; ++i)
    result[i] = mappings[i].terminal;
  return result;
}

template <size_t N> consteval auto sourceEntries(const std::array<SourceEntry, N>& mappings) {
  static_assert(N < 65536, "Source terminal IDs exceed capacity");
  std::array<ReaderEntry, N> result { };
  for (size_t i = 0; i < N; ++i)
    result[i] = { mappings[i].key, static_cast<uint16_t>(i + 1) };
  return result;
}

template <auto const& Text, size_t N> consteval WriterChar writerChar(const char8_t (&text)[N], uint16_t offset) {
  static_assert(N - 1 <= 255, "Writer length exceeds capacity");
  if (size_t(offset) + N - 1 > Text.size())
    std::abort();
  for (size_t i = 0; i < N - 1; ++i)
    if (static_cast<uint8_t>(Text[offset + i]) != static_cast<uint8_t>(text[i]))
      std::abort();
  const WriterChar result { offset, static_cast<uint8_t>(N - 1) };
  if (result.offset != offset || result.length != N - 1)
    std::abort();
  return result;
}

consteval auto sequenceLookup(const auto& pool, unsigned bits) {
  TokenLookupTable result;
  // Search sequence boundaries, so a token within an expansion cannot change
  // the canonical ID of a standalone token when source syntax changes.
  for (const auto range : pool.ranges) {
    if (range.count != 1)
      continue;
    const auto token = pool.tokens[range.begin];
    const auto script = size_t(token.scriptType), type = size_t(token.tokenType), index = size_t(token.idx);
    if (script < 4 && type < 9 && index < 64 && result.single[script][type][index] == 0)
      result.single[script][type][index] = uint16_t(range.begin | (1u << bits));
  }
  return result;
}

/// Packs the phonological components of a TokenUnit into a 64-bit integer key for trie lookup.
///
/// Bit layout:
/// - Bits [0..3]:   leadToken.tokenType
/// - Bits [4..11]:  leadToken.idx
/// - Bits [12..13]: leadToken.scriptType
/// - Bits [14..25]: vowelMark (tokenType + idx)
/// - Bits [26..37]: otherDiacritic (tokenType + idx)
/// - Bits [38..49]: accent (tokenType + idx)
constexpr uint64_t prefixKey(const TokenUnit& unit) noexcept {
  const auto tokenKey
      = [](const Token& token) constexpr -> uint64_t { return uint64_t(token.tokenType) | (uint64_t(token.idx) << 4); };
  return tokenKey(unit.leadToken) | (uint64_t(unit.leadToken.scriptType) << 12) | (tokenKey(unit.vowelMark) << 14)
      | (tokenKey(unit.otherDiacritic) << 26) | (tokenKey(unit.accent) << 38);
}

/// Sequence of packed 64-bit TokenUnit keys representing a Tamil prefix.
struct TamilPrefixKey {
  using key_type = uint64_t;
  uint64_t keys[4] { }; ///< Array of up to 4 packed TokenUnit keys
  uint8_t count { }; ///< Number of valid keys in the sequence

  constexpr bool empty() const noexcept { return count == 0; }
  constexpr size_t size() const noexcept { return count; }
  constexpr uint64_t operator[](size_t i) const noexcept { return keys[i]; }

  constexpr bool operator<(const TamilPrefixKey& other) const noexcept {
    for (size_t i = 0; i < count && i < other.count; ++i) {
      if (keys[i] != other.keys[i])
        return keys[i] < other.keys[i];
    }
    return count < other.count;
  }
};

/// Entry in the static Tamil prefix lookup table.
struct TamilEntry {
  using key_type = uint64_t;
  TamilPrefixKey key; ///< Key sequence of packed TokenUnits
  uint16_t value { 1 }; ///< Non-zero terminal indicator

  constexpr bool operator<(const TamilEntry& other) const noexcept { return key < other.key; }
};

/// Consteval function that tokenizes the canonical Tamil prefix strings using the compiled reader trie,
/// constructs their packed TamilPrefixKey representations, and sorts them for static trie building.
template <typename Trie, typename Terminals, typename Alternatives, typename Tokens, size_t N>
consteval auto deriveTamilEntries(const Trie& trie, const Terminals& terminals, const Alternatives& alternatives,
    const Tokens& tokens, unsigned seqBits, uint32_t tamilSource, const std::array<Utf8Key, N>& prefixes) {

  const auto selector = [&](uint16_t, auto state) -> uint16_t {
    const auto termId = trie.nodes[state].value;
    if (termId == 0)
      return 0;
    const auto& term = terminals[termId - 1];
    if (term.sources & tamilSource)
      return term.sequence;
    const auto range = term.alternatives;
    for (size_t i = 0; i < range.count; ++i) {
      if (alternatives[range.begin + i].sources & tamilSource)
        return alternatives[range.begin + i].sequence;
    }
    return 0;
  };

  std::array<TamilEntry, N> entries { };

  for (size_t i = 0; i < N; ++i) {
    const char* ptr = prefixes[i].bytes;
    const char* end = ptr + prefixes[i].len;
    TokenUnit current(invalidScriptToken);
    bool hasUnit = false;

    while (ptr < end) {
      auto match = trie.view().match(ptr, end, selector);
      if (match.value == 0 || match.length == 0)
        std::abort();
      const auto mask = (1u << seqBits) - 1;
      const auto token = tokens[match.value & mask];
      const auto type = static_cast<uint8_t>(token.tokenType);
      if (type == 0 || type == 2 || type == 5 || type == 7) {
        if (hasUnit) {
          entries[i].key.keys[entries[i].key.count++] = prefixKey(current);
        }
        current = TokenUnit(token);
        hasUnit = true;
      } else if (type == 1) {
        current.vowelMark = token;
      } else if (type == 3) {
        current.otherDiacritic = token;
      } else if (type == 4) {
        current.accent = token;
      }
      ptr += match.length;
    }
    if (hasUnit) {
      entries[i].key.keys[entries[i].key.count++] = prefixKey(current);
    }
    entries[i].value = 1;
  }

  std::sort(entries.begin(), entries.end());
  return entries;
}

} // namespace inditrans::static_data

/// Target script descriptor defining output character mappings, script family, and Vedic capability.
///
/// Contains compile-time spans into the shared UTF-8 writer text pool for each TokenType category.
/// The 9th slot is empty, allowing TokenType::Ignore lookups to safely return empty views without branching.
struct ScriptWriterMap {
  ScriptType scriptType; ///< Script family (Indic, Tamil, Latin, Others)
  bool vedic; ///< True if this script natively supports Vedic accent marks
  std::array<std::span<const inditrans::static_data::WriterChar>, 9> charMaps; ///< Grapheme spans indexed by TokenType

  constexpr ScriptType getType() const noexcept { return scriptType; }
  constexpr bool isVedic() const noexcept { return vedic; }
  constexpr std::string_view lookupChar(TokenType type, size_t index) const noexcept;
  constexpr std::string_view lookupChar(const Token& token) const noexcept {
    return lookupChar(token.tokenType, token.idx);
  }
};
