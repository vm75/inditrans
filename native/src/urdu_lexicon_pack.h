#pragma once

#include "urdu_pronunciation.h"
#include <cstdint>
#include <span>
#include <string>
#include <vector>

/// Owns compact, validated IUL1 pronunciation data. Word keys and token units
/// live in contiguous storage, without one heap allocation per word. Input
/// bytes may be released immediately after load(); the spans are pack-owned.
class UrduLexiconPack {
public:
  UrduLexiconPack() = default;
  UrduLexiconPack(const UrduLexiconPack&) = delete;
  UrduLexiconPack& operator=(const UrduLexiconPack&) = delete;
  UrduLexiconPack(UrduLexiconPack&&) = delete;
  UrduLexiconPack& operator=(UrduLexiconPack&&) = delete;

  bool load(std::span<const uint8_t> data) {
    clear();
    if (parse(data)) return true;
    clear();
    return false;
  }

  UrduLexicon lexicon() const noexcept { return entries; }

private:
  struct EntryOffsets {
    size_t wordOffset, wordSize, readingOffset, readingSize;
  };

  void clear() noexcept {
    entries.clear();
    readings.clear();
    words.clear();
  }

  bool parse(std::span<const uint8_t> data) {
    if (data.size() < 16 || std::string_view(reinterpret_cast<const char*>(data.data()), 4) != "IUL1")
      return false;
    size_t pos = 4;
    uint32_t checksum, metadataSize, count;
    if (!read32(data, pos, checksum) || checksum != crc32(data.subspan(pos))) return false;
    if (!read32(data, pos, metadataSize) || metadataSize > data.size() - pos) return false;
    pos += metadataSize;
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
      if ((i && word <= previousWord) || word.find("ك") != word.npos || word.find("ي") != word.npos
          || word.find("ى") != word.npos || word.find("ه") != word.npos) return false;
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
    return inditrans::handlers::validUrduLexicon(entries);
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
  std::vector<UrduLexiconEntry> entries;
};

/// The caller supplies the asset bytes, owned only during this call. Invalid
/// assets fall back to the existing Urdu reader; no global state is created.
inline bool transliterateUrduWithPack(std::string_view text, std::string_view to, TranslitOptions options,
    std::span<const uint8_t> data, std::string& output, std::string_view skipStart = "##",
    std::string_view skipEnd = "##") {
  UrduLexiconPack pack;
  pack.load(data);
  return transliterateUrduWithLexicon(text, to, options, pack.lexicon(), output, skipStart, skipEnd);
}
