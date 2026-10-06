#pragma once

#include "packed_trie.h"
#include "type_defs.h"
#include <algorithm>
#include <array>
#include <span>

namespace inditrans::static_data {

struct Range {
  uint16_t begin;
  uint16_t count;
};

// Empty alternative ranges have no meaningful offset. Keep them canonical zero
// so neither runtime storage nor Wasm segments retain dead cumulative offsets.
struct VariantRange {
  uint16_t bits { };

  constexpr VariantRange() = default;
  consteval VariantRange(Range range) {
    if (range.count > 15 || (range.count && range.begin > 4095))
      std::abort();
    bits = range.count ? uint16_t(range.begin | (range.count << 12)) : 0;
  }

  constexpr uint16_t begin() const noexcept { return bits & 4095; }
  constexpr uint16_t count() const noexcept { return bits >> 12; }
};
static_assert(sizeof(VariantRange) == 2);

template <typename... Args>
constexpr uint32_t scriptMask(Args... ids) noexcept {
  return ((1u << static_cast<uint32_t>(ids)) | ...);
}

struct SourceVariant {
  uint32_t sources { };
  uint16_t sequence { };

  constexpr SourceVariant() = default;
  constexpr SourceVariant(uint32_t src, uint16_t seq) noexcept : sources(src), sequence(seq) {}
};

struct SourceTerminal {
  uint32_t sources { };
  uint16_t sequence { };
  uint16_t indicSequence { };
  Range alternatives { };

  constexpr SourceTerminal() = default;
  constexpr SourceTerminal(uint32_t src, uint16_t seq, uint16_t indicSeq = 0, Range alt = {}) noexcept
      : sources(src), sequence(seq), indicSequence(indicSeq), alternatives(alt) {}
};

inline constexpr uint16_t virtualRejected = 0x8000;

// Current virtual results are either the primary sequence or absent. Encode
// that distinction beside the sequence, avoiding a second array or lookup.
consteval uint16_t sequencePayload(const SourceTerminal& terminal) {
  if (terminal.sequence >= virtualRejected
      || (terminal.indicSequence && terminal.indicSequence != terminal.sequence))
    std::abort();
  return terminal.sequence | (terminal.sequence && !terminal.indicSequence ? virtualRejected : 0);
}

constexpr uint16_t explicitSequence(uint16_t payload) noexcept { return payload & ~virtualRejected; }
constexpr uint16_t virtualSequence(uint16_t payload) noexcept { return payload & virtualRejected ? 0 : payload; }

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
  uint8_t offsetLow { };
  uint8_t offsetHigh { };
  uint8_t length { };

  constexpr WriterChar() = default;
  constexpr WriterChar(uint16_t off, uint8_t len) noexcept
      : offsetLow(static_cast<uint8_t>(off)), offsetHigh(static_cast<uint8_t>(off >> 8)), length(len) {}

  constexpr uint16_t offset() const noexcept { return uint16_t(offsetLow) | (uint16_t(offsetHigh) << 8); }
};
static_assert(sizeof(WriterChar) == 3 && alignof(WriterChar) == 1);

template <size_t N>
struct PackedUtf8 {
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

template <size_t N>
consteval auto packUtf8(const char8_t (&str)[N]) {
  return PackedUtf8<N>(str);
}

struct Utf8Key {
  char bytes[16] { };
  uint8_t len { };

  consteval Utf8Key() = default;

  template <size_t M>
  consteval Utf8Key(const char8_t (&str)[M]) : len(static_cast<uint8_t>(M - 1)) {
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

template <size_t N>
struct TokenSequence {
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

template <typename... Tokens>
constexpr auto seq(Tokens... ts) noexcept {
  return TokenSequence<sizeof...(Tokens)> { { ts... } };
}

template <size_t TotalTokens, size_t TotalSequences>
struct SequencePool {
  std::array<ScriptToken, TotalTokens> tokens { };
  std::array<Range, TotalSequences> ranges { };
  constexpr auto data() const noexcept { return tokens.data(); }
  constexpr size_t size() const noexcept { return TotalTokens; }
  constexpr const ScriptToken& operator[](size_t i) const noexcept { return tokens[i]; }
};

template <typename... Seqs>
consteval auto makeSequencePool(const Seqs&... seqs) {
  constexpr size_t total = 1 + (seqs.size() + ... + 0);
  static_assert(total <= (1u << 12), "Total sequence tokens exceeds offset capacity");
  static_assert(((seqs.size() < (1u << (16 - 12))) && ...), "Sequence length exceeds length capacity");
  SequencePool<total, sizeof...(Seqs)> pool;
  pool.tokens[0] = ScriptToken(TokenType::Ignore, 255, ScriptType::Others);
  size_t offset = 1;
  size_t sequence = 0;
  const auto add = [&]<size_t N>(const TokenSequence<N>& s) {
    pool.ranges[sequence++] = {static_cast<uint16_t>(offset), static_cast<uint16_t>(N)};
    for (size_t i = 0; i < N; ++i)
      pool.tokens[offset++] = s[i];
  };
  (add(seqs), ...);
  return pool;
}

struct SemanticMapping {
  Utf8Key key;
  ScriptToken tokens[4] { };
  uint8_t count { };

  consteval SemanticMapping() = default;

  template <size_t N>
  consteval SemanticMapping(Utf8Key k, const TokenSequence<N>& s) : key(k), count(static_cast<uint8_t>(N)) {
    static_assert(N <= 4, "Sequence exceeds maximum mapping length");
    for (size_t i = 0; i < N; ++i)
      tokens[i] = s[i];
  }

  consteval SemanticMapping(Utf8Key k, ScriptToken t) : key(k), count(1) {
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
consteval std::array<ReaderEntry, M> deriveReaderEntries(const auto& seqTokens, unsigned seqBits, const std::array<SemanticMapping, M>& mappings) {
  TokenLookupTable table(seqTokens, seqBits);
  std::array<ReaderEntry, M> entries;
  for (size_t m = 0; m < M; ++m) {
    const auto& map = mappings[m];
    const size_t N = map.count;
    uint16_t id = 0;
    if (N == 1) {
      id = table.lookup(map.tokens[0]);
    }
    if (id == 0) {
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
  std::array<SourceTerminal, N> result {};
  for (size_t i = 0; i < N; ++i) result[i] = mappings[i].terminal;
  return result;
}

template <size_t N> consteval auto sourceEntries(const std::array<SourceEntry, N>& mappings) {
  static_assert(N < 65536, "Source terminal IDs exceed capacity");
  std::array<ReaderEntry, N> result {};
  for (size_t i = 0; i < N; ++i) result[i] = {mappings[i].key, static_cast<uint16_t>(i + 1)};
  return result;
}

template <auto const& Text, size_t N>
consteval WriterChar writerChar(const char8_t (&text)[N], uint16_t offset) {
  static_assert(N - 1 <= 255, "Writer length exceeds capacity");
  if (size_t(offset) + N - 1 > Text.size()) std::abort();
  for (size_t i = 0; i < N - 1; ++i)
    if (static_cast<uint8_t>(Text[offset + i]) != static_cast<uint8_t>(text[i])) std::abort();
  const WriterChar result {offset, static_cast<uint8_t>(N - 1)};
  if (result.offset() != offset || result.length != N - 1)
    std::abort();
  return result;
}

consteval auto sequenceLookup(const auto& pool, unsigned bits) {
  TokenLookupTable result;
  // Search sequence boundaries, so a token within an expansion cannot change
  // the canonical ID of a standalone token when source syntax changes.
  for (const auto range : pool.ranges) {
    if (range.count != 1) continue;
    const auto token = pool.tokens[range.begin];
    const auto script = size_t(token.scriptType), type = size_t(token.tokenType), index = size_t(token.idx);
    if (script < 4 && type < 9 && index < 64 && result.single[script][type][index] == 0)
      result.single[script][type][index] = uint16_t(range.begin | (1u << bits));
  }
  return result;
}

constexpr uint64_t prefixKey(const TokenUnit& unit) noexcept {
  const auto tokenKey
      = [](const Token& token) constexpr -> uint64_t { return uint64_t(token.tokenType) | (uint64_t(token.idx) << 4); };
  return tokenKey(unit.leadToken) | (uint64_t(unit.leadToken.scriptType) << 12) | (tokenKey(unit.vowelMark) << 14)
      | (tokenKey(unit.otherDiacritic) << 26) | (tokenKey(unit.accent) << 38);
}

struct TamilPrefixKey {
  using key_type = uint64_t;
  uint64_t keys[4] { };
  uint8_t count { };

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

struct TamilEntry {
  using key_type = uint64_t;
  TamilPrefixKey key;
  uint16_t value { 1 };

  constexpr bool operator<(const TamilEntry& other) const noexcept {
    return key < other.key;
  }
};

template <typename Trie, typename Terminals, typename Alternatives, typename Tokens, size_t N>
consteval auto deriveTamilEntries(
    const Trie& trie,
    const Terminals& terminals,
    const Alternatives& alternatives,
    const Tokens& tokens,
    unsigned seqBits,
    uint32_t tamilSource,
    const std::array<Utf8Key, N>& prefixes) {

  const auto selector = [&](uint16_t, auto state) consteval -> uint16_t {
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

  std::array<TamilEntry, N> entries;

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
