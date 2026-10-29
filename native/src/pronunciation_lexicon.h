#pragma once

#include "type_defs.h"
#include "packed_lexicon.h"
#include <span>
#include <string>
#include <string_view>

/// Immutable pronunciation data. Entries contain strictly sorted, canonical
/// UTF-8 native-script words and canonical token units, not target-script strings.
/// Both word and pronunciation spans must outlive the conversion call.
struct PronunciationLexiconEntry {
  std::string_view word;
  std::span<const TokenUnit> pronunciation;
};

using PronunciationLexicon = std::span<const PronunciationLexiconEntry>;

/// Internal lookup view: legacy caller-owned arrays or a packed dictionary.
class PronunciationLexiconLookup {
public:
  PronunciationLexiconLookup() = default;
  explicit PronunciationLexiconLookup(PronunciationLexicon entries) : data(entries.data()), count(entries.size()) { }
  explicit PronunciationLexiconLookup(const PackedLexicon* packed) : data(packed), count(Packed) { }
  const PackedLexicon* packed() const noexcept {
    return count == Packed ? static_cast<const PackedLexicon*>(data) : nullptr;
  }
  PronunciationLexicon entries() const noexcept {
    return count == Packed ? PronunciationLexicon {} : PronunciationLexicon(static_cast<const PronunciationLexiconEntry*>(data), count);
  }
  bool empty() const noexcept { return packed() ? packed()->empty() : count == 0; }
private:
  static constexpr size_t Packed = static_cast<size_t>(-1);
  const void* data = nullptr;
  size_t count = 0;
};

