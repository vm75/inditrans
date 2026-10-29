#pragma once

#include "pronunciation_reader.h"
#include "reader_extension.h"
#include <cstdint>
#include <vector>

/// Validated immutable IUL2 storage. Legacy IUL1 data is compacted once on load;
/// caller bytes can be released immediately afterward. Lookup scratch is local
/// to each reader. Key policies do not change the script-independent payload.
template <typename Policy = inditrans::handlers::ExactWordPolicy> class BasicPronunciationLexiconPack {
public:
  BasicPronunciationLexiconPack() = default;
  BasicPronunciationLexiconPack(const BasicPronunciationLexiconPack&) = delete;
  BasicPronunciationLexiconPack& operator=(const BasicPronunciationLexiconPack&) = delete;
  BasicPronunciationLexiconPack(BasicPronunciationLexiconPack&&) = delete;
  BasicPronunciationLexiconPack& operator=(BasicPronunciationLexiconPack&&) = delete;

  bool load(std::span<const uint8_t> data) {
    clear();
    if (parse(data)) {
      if (packed.empty() && !entries.empty()) {
        std::vector<std::vector<uint8_t>> values;
        std::vector<PackedLexicon::Record> records;
        values.reserve(entries.size()); records.reserve(entries.size());
        for (const auto& entry : entries) {
          std::vector<uint8_t> value {1, 0, static_cast<uint8_t>(entry.pronunciation.size()),
              static_cast<uint8_t>(entry.pronunciation.size() >> 8)};
          for (const auto& unit : entry.pronunciation) {
            const unsigned lead = unit.leadToken.idx + (unit.leadToken.tokenType == TokenType::Consonant ? 22 : 0);
            const unsigned mark = unit.vowelMark.idx == InvalidToken ? 22 : unit.vowelMark.idx;
            const unsigned other = unit.otherDiacritic.idx == InvalidToken ? 8 : unit.otherDiacritic.idx;
            const unsigned bits = lead | (mark << 7) | (other << 12);
            value.push_back(bits); value.push_back(bits >> 8);
          }
          values.push_back(std::move(value));
          records.push_back({entry.word, values.back()});
        }
        if (!packed.build(records)) return true; // Large alphabets retain the compatible legacy representation.
      }
      // Release expanded legacy data after conversion. All steady-state
      // lookups use the same compact representation, including IUL1 imports.
      std::vector<PronunciationLexiconEntry>().swap(entries);
      std::vector<TokenUnit>().swap(readings);
      std::string().swap(words);
      return true;
    }
    clear();
    return false;
  }

  PronunciationLexiconLookup lexicon() const noexcept { return packed.empty() ? PronunciationLexiconLookup(PronunciationLexicon(entries)) : PronunciationLexiconLookup(&packed); }

private:
  struct EntryOffsets {
    size_t wordOffset, wordSize, readingOffset, readingSize;
  };

  PackedLexicon packed;

  void clear() noexcept {
    packed.clear();
    entries.clear();
    readings.clear();
    words.clear();
  }

  bool parse(std::span<const uint8_t> data) {
    if (data.size() < 16 || (std::string_view(reinterpret_cast<const char*>(data.data()), 4) != "IUL1"
        && std::string_view(reinterpret_cast<const char*>(data.data()), 4) != "IUL2"))
      return false;
    size_t pos = 4;
    uint32_t checksum, metadataSize, count;
    if (!read32(data, pos, checksum) || checksum != crc32(data.subspan(pos))) return false;
    if (!read32(data, pos, metadataSize) || metadataSize > data.size() - pos) return false;
    pos += metadataSize;
    if (data[3] == '2') {
      if (!packed.load(data.subspan(pos))) return false;
      return packed.visit([](std::string_view word, std::span<const uint8_t> value) {
        if (!Policy::canonical(word) || value.size() < 4) return false;
        size_t p = 0;
        auto read = [&]() { unsigned n = value[p] | (unsigned(value[p + 1]) << 8); p += 2; return n; };
        unsigned readings = read();
        if (!readings) return false;
        for (unsigned r = 0; r < readings; ++r) {
          if (value.size() - p < 2) return false;
          unsigned count = read();
          if (!count || count > (value.size() - p) / 2) return false;
          for (unsigned i = 0; i < count; ++i) {
            unsigned bits = read(), lead = bits & 127, mark = (bits >> 7) & 31, other = bits >> 12;
            if (lead >= 86 || mark > 22 || other > 8 || (lead < 22 && mark != 22)) return false;
          }
        }
        return p == value.size();
      });
    }
    if (!read32(data, pos, count) || count > (data.size() - pos) / 8) return false;
    std::vector<EntryOffsets> offsets;
    offsets.reserve(count);
    entries.reserve(count);
    std::string_view previousWord;
    for (uint32_t i = 0; i < count; ++i) {
      uint16_t wordSize, readingCount;
      if (!read16(data, pos, wordSize) || !read16(data, pos, readingCount)
          || wordSize == 0 || readingCount == 0 || wordSize > data.size() - pos) return false;
      std::string_view word(reinterpret_cast<const char*>(data.data() + pos), wordSize);
      pos += wordSize;
      if ((i && word <= previousWord) || !Policy::canonical(word)) return false;
      previousWord = word;

      const size_t wordOffset = words.size(), readingOffset = readings.size();
      words.append(word);
      for (uint16_t j = 0; j < readingCount; ++j) {
        uint16_t unitCount;
        if (!read16(data, pos, unitCount) || unitCount == 0 || unitCount > (data.size() - pos) / 4) return false;
        for (uint16_t k = 0; k < unitCount; ++k) {
          const auto lead = data[pos++], index = data[pos++], mark = data[pos++], other = data[pos++];
          if (lead > 1 || index >= (lead ? 64 : 22) || (mark != 255 && (!lead || mark >= 22))
              || (other != 255 && other >= 8)) return false;
          if (j == 0) {
            TokenUnit unit({lead ? TokenType::Consonant : TokenType::Vowel, index, ScriptType::Indic});
            if (mark != 255) unit.vowelMark = {TokenType::VowelMark, mark};
            if (other != 255) unit.otherDiacritic = {TokenType::OtherDiacritic, other};
            readings.push_back(unit);
          }
        }
      }
      offsets.push_back({wordOffset, wordSize, readingOffset, readings.size() - readingOffset});
    }
    if (pos != data.size()) return false;
    // Only construct views after both owning buffers are final and stable.
    for (const auto& entry : offsets) {
      entries.push_back({
          std::string_view(words.data() + entry.wordOffset, entry.wordSize),
          std::span<const TokenUnit>(readings.data() + entry.readingOffset, entry.readingSize)});
    }
    return inditrans::handlers::validPronunciationLexicon<Policy>(entries);
  }

  static uint32_t crc32(std::span<const uint8_t> data) noexcept {
    uint32_t crc = 0xffffffffu;
    for (const auto byte : data) {
      crc ^= byte;
      for (int bit = 0; bit < 8; ++bit)
        crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
  }

  static bool read16(std::span<const uint8_t> data, size_t& pos, uint16_t& value) noexcept {
    if (data.size() - pos < 2) return false;
    value = uint16_t(data[pos]) | (uint16_t(data[pos + 1]) << 8);
    pos += 2;
    return true;
  }
  static bool read32(std::span<const uint8_t> data, size_t& pos, uint32_t& value) noexcept {
    if (data.size() - pos < 4) return false;
    value = uint32_t(data[pos]) | (uint32_t(data[pos + 1]) << 8)
        | (uint32_t(data[pos + 2]) << 16) | (uint32_t(data[pos + 3]) << 24);
    pos += 4;
    return true;
  }

  std::string words;
  std::vector<TokenUnit> readings;
  std::vector<PronunciationLexiconEntry> entries;
};

using PronunciationLexiconPack = BasicPronunciationLexiconPack<>;

/// Explicit dictionary override for any source script. The pack is validated
/// once by load(); unknown words use that script's ordinary reader.
bool transliterateWithLexicon(std::string_view input, std::string_view from,
    std::string_view to, TranslitOptions options, const PronunciationLexiconPack& pack,
    std::string& output, std::string_view skipStart = "##", std::string_view skipEnd = "##") noexcept;
