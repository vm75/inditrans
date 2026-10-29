#pragma once

#include <algorithm>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

/// Script-independent immutable dictionary. Keys are UTF-8; values are opaque
/// bytes. Only block offsets are expanded. Scratch space belongs to each lookup.
/// PLX1 uses little-endian integers, a sorted Unicode alphabet (one-byte IDs),
/// and blocks of at most 32 prefix-compressed keys.
class PackedLexicon {
public:
  struct Record { std::string_view key; std::span<const uint8_t> value; };
  bool empty() const noexcept { return blocks.empty(); }
  std::span<const uint8_t> bytes() const noexcept { return storage; }
  void clear() { storage.clear(); alphabet.clear(); blocks.clear(); }

  bool build(std::span<const Record> records) {
    clear();
    std::vector<std::string> chars;
    for (const auto& record : records) {
      std::vector<std::string> key;
      if (!characters(record.key, key)) return false;
      chars.insert(chars.end(), key.begin(), key.end());
    }
    std::sort(chars.begin(), chars.end());
    chars.erase(std::unique(chars.begin(), chars.end()), chars.end());
    if (chars.size() > 256) return false;
    std::vector<uint8_t> out {'P','L','X','1'};
    put(out, chars.size(), 2); put(out, records.size(), 4);
    for (const auto& ch : chars) { out.push_back(ch.size()); out.insert(out.end(), ch.begin(), ch.end()); }
    std::string previous;
    for (size_t i = 0; i < records.size(); ++i) {
      std::vector<std::string> key;
      characters(records[i].key, key);
      std::string encoded;
      for (const auto& ch : key) encoded.push_back(static_cast<char>(std::lower_bound(chars.begin(), chars.end(), ch) - chars.begin()));
      size_t prefix = 0;
      if (i % 32) while (prefix < previous.size() && prefix < encoded.size() && previous[prefix] == encoded[prefix]) ++prefix;
      if (encoded.size() > 65535 || records[i].value.size() > UINT32_MAX) return false;
      put(out, prefix, 2); put(out, encoded.size() - prefix, 2); put(out, records[i].value.size(), 4);
      out.insert(out.end(), encoded.begin() + prefix, encoded.end());
      out.insert(out.end(), records[i].value.begin(), records[i].value.end());
      previous = std::move(encoded);
    }
    return load(out);
  }

  bool load(std::span<const uint8_t> data) {
    clear();
    if (data.size() < 10 || std::string_view(reinterpret_cast<const char*>(data.data()), 4) != "PLX1") return false;
    size_t pos = 4;
    const auto countChars = get(data, pos, 2), count = get(data, pos, 4);
    if (countChars > 256) return false;
    std::vector<std::string> chars;
    for (size_t i = 0; i < countChars; ++i) {
      if (pos == data.size()) return false;
      size_t n = data[pos++];
      if (!n || n > data.size() - pos) return false;
      std::string ch(reinterpret_cast<const char*>(data.data() + pos), n);
      std::vector<std::string> split;
      if (!characters(ch, split) || split.size() != 1 || (!chars.empty() && ch <= chars.back())) return false;
      chars.push_back(ch); pos += n;
    }
    std::vector<size_t> index;
    std::string word, previous;
    if (count > (data.size() - pos) / 8) return false;
    for (size_t i = 0; i < count; ++i) {
      if (i % 32 == 0) index.push_back(pos);
      if (data.size() - pos < 8) return false;
      auto prefix = get(data, pos, 2), suffix = get(data, pos, 2), size = get(data, pos, 4);
      if ((i % 32 == 0 && prefix) || prefix > word.size() || prefix + suffix > 65535
          || suffix > data.size() - pos) return false;
      word.resize(prefix);
      for (size_t j = 0; j < suffix; ++j) {
        auto id = data[pos++];
        if (id >= chars.size()) return false;
        word.push_back(static_cast<char>(id));
      }
      // Compare IDs as unsigned bytes, including alphabet IDs above 127.
      if (word.empty() || (i && compare(word, previous) <= 0) || !size || size > data.size() - pos) return false;
      previous = word; pos += size;
    }
    if (pos != data.size()) return false;
    storage.assign(data.begin(), data.end()); alphabet = std::move(chars); blocks = std::move(index);
    return true;
  }

  /// Comparator receives a reconstructed UTF-8 key and the query. It may apply
  /// script-specific aliases, provided it preserves the stored ordering.
  template <typename Compare>
  std::span<const uint8_t> find(std::string_view query, Compare compareKey) const {
    if (empty()) return {};
    std::string encoded, key;
    auto record = [&](size_t& pos) {
      auto prefix = get(storage, pos, 2), suffix = get(storage, pos, 2), size = get(storage, pos, 4);
      encoded.resize(prefix);
      encoded.append(reinterpret_cast<const char*>(storage.data() + pos), suffix); pos += suffix;
      key.clear();
      for (unsigned char id : encoded) key += alphabet[id];
      auto value = std::span<const uint8_t>(storage).subspan(pos, size); pos += size;
      return value;
    };
    size_t low = 0, high = blocks.size();
    while (low < high) {
      size_t mid = low + (high - low) / 2, pos = blocks[mid];
      encoded.clear(); record(pos);
      if (compareKey(key, query) <= 0) low = mid + 1; else high = mid;
    }
    if (!low) return {};
    size_t block = low - 1, pos = blocks[block];
    const size_t end = block + 1 < blocks.size() ? blocks[block + 1] : storage.size();
    encoded.clear();
    while (pos < end) {
      auto value = record(pos);
      int comparison = compareKey(key, query);
      if (!comparison) return value;
      if (comparison > 0) break;
    }
    return {};
  }

  template <typename Visit> bool visit(Visit visitor) const {
    for (size_t block = 0; block < blocks.size(); ++block) {
      size_t pos = blocks[block], end = block + 1 < blocks.size() ? blocks[block + 1] : storage.size();
      std::string word, key;
      while (pos < end) {
        auto prefix = get(storage, pos, 2), suffix = get(storage, pos, 2), size = get(storage, pos, 4);
        word.resize(prefix); word.append(reinterpret_cast<const char*>(storage.data() + pos), suffix); pos += suffix;
        key.clear(); for (unsigned char id : word) key += alphabet[id];
        if (!visitor(key, std::span<const uint8_t>(storage).subspan(pos, size))) return false;
        pos += size;
      }
    }
    return true;
  }
private:
  std::vector<uint8_t> storage;
  std::vector<std::string> alphabet;
  std::vector<size_t> blocks;
  static int compare(std::string_view a, std::string_view b) { return a.compare(b); }
  static void put(std::vector<uint8_t>& out, uint64_t value, unsigned n) {
    for (unsigned i = 0; i < n; ++i) out.push_back(value >> (8 * i));
  }
  static uint32_t get(std::span<const uint8_t> data, size_t& pos, unsigned n) {
    uint32_t value = 0;
    for (unsigned i = 0; i < n; ++i) value |= uint32_t(data[pos++]) << (8 * i);
    return value;
  }
  static bool characters(std::string_view text, std::vector<std::string>& result) {
    while (!text.empty()) {
      const auto a = static_cast<uint8_t>(text[0]);
      unsigned n = a < 128 ? 1 : a >= 0xc2 && a <= 0xdf ? 2 : a >= 0xe0 && a <= 0xef ? 3 : a >= 0xf0 && a <= 0xf4 ? 4 : 0;
      if (!n || n > text.size()) return false;
      uint32_t cp = a & (n == 1 ? 127 : (1u << (7 - n)) - 1);
      for (unsigned i = 1; i < n; ++i) {
        auto b = static_cast<uint8_t>(text[i]);
        if ((b & 0xc0) != 0x80) return false;
        cp = (cp << 6) | (b & 63);
      }
      if ((n == 2 && cp < 128) || (n == 3 && cp < 2048) || (n == 4 && cp < 65536)
          || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) return false;
      result.emplace_back(text.substr(0, n)); text.remove_prefix(n);
    }
    return true;
  }
};
