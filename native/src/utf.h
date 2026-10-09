#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

/// Represents a decoded Unicode codepoint alongside its UTF-8 encoded byte length.
struct Char32 {
  char32_t ch; ///< Decoded 32-bit Unicode codepoint
  uint32_t len; ///< UTF-8 sequence length in bytes (1 to 4)

  constexpr operator char32_t() const noexcept { return ch; }
};

/// High-performance utilities for UTF-8 and UTF-32 inspection, encoding, and decoding.
/// Navigation assumes complete UTF-8 characters in accessible storage; these
/// helpers infer lengths from bytes and do not validate encoding or buffer bounds.
struct UtfUtils {
  /// Returns the byte length (1 to 4) of the UTF-8 sequence starting at @p bytes
  /// by examining the leading byte mask.
  static inline constexpr size_t nextCharLen(const char* bytes) noexcept {
    const auto byte1 = static_cast<uint8_t>(*bytes);
    if ((byte1 & 0xF8) == 0xF0) { // byte1 > 0xF0: 4-byte sequence
      return 4;
    } else if ((byte1 & 0xF0) == 0xE0) { // byte1 > 0xE0: 3-byte sequence
      return 3;
    } else if ((byte1 & 0xE0) == 0xC0) { // byte1 > 0x80: 2-byte sequence
      return 2;
    }
    return 1; // 1-byte ASCII sequence
  }

  /// Scans backwards from @p bytes (pointing just past a character) to determine
  /// the byte length of the preceding UTF-8 sequence by inspecting continuation bytes (0x80..0xBF).
  static inline constexpr size_t prevCharLen(const char* bytes) noexcept {
    if (static_cast<uint8_t>(*--bytes) < 0x80) { // Single-byte ASCII
      return 1;
    }
    if (static_cast<uint8_t>(*--bytes) > 0xBF) { // 2-byte sequence lead byte
      return 2;
    }
    if (static_cast<uint8_t>(*--bytes) > 0xBF) { // 3-byte sequence lead byte
      return 3;
    }
    return 4; // 4-byte sequence lead byte
  }

  /// Returns a std::string_view containing the next complete UTF-8 codepoint starting at @p bytes.
  static inline constexpr std::string_view nextUtf8Char(const char* bytes) noexcept {
    const auto byte1 = static_cast<uint8_t>(*bytes);
    if ((byte1 & 0xF8) == 0xF0) {
      return { bytes, 4 };
    } else if ((byte1 & 0xF0) == 0xE0) {
      return { bytes, 3 };
    } else if ((byte1 & 0xE0) == 0xC0) {
      return { bytes, 2 };
    }
    return { bytes, 1 };
  }

  /// Decodes the UTF-8 sequence at @p bytes into a Char32 codepoint and advances @p bytes past the character.
  static inline constexpr Char32 toChar32(const char*& bytes) noexcept {
    const auto char1 = *bytes++;
    if ((char1 & 0xF8) == 0xF0) { // char1 > 0xF0
      const auto char2 = *bytes++;
      const auto char3 = *bytes++;
      const auto char4 = *bytes++;
      char32_t ch = (((char1 & 0x07) << 18) | ((char2 & 0x3F) << 12) | ((char3 & 0x3F) << 6) | (char4 & 0x3F));
      return { ch, 4 };
    } else if ((char1 & 0xF0) == 0xE0) { // char1 > 0xE0
      const auto char2 = *bytes++;
      const auto char3 = *bytes++;
      char32_t ch = (((char1 & 0x0F) << 12) | ((char2 & 0x3F) << 6) | (char3 & 0x3F));
      return { ch, 3 };
    } else if ((char1 & 0xE0) == 0xC0) { // char1 > 0x80
      const auto char2 = *bytes++;
      char32_t ch = (((char1 & 0x1F) << 6) | (char2 & 0x3F));
      return { ch, 2 };
    } else {
      char32_t ch = (char1);
      return { ch, 1 };
    }
  }

  /// Returns the number of UTF-8 bytes required to encode the given Unicode codepoint @p ch.
  static inline constexpr size_t charLen(char32_t ch) noexcept {
    if (ch < 0x80) {
      return 1;
    } else if (ch < 0x800) {
      return 2;
    } else if (ch < 0x10000) {
      return 3;
    } else {
      return 4;
    }
  }

  /// Encodes a Unicode codepoint @p ch into UTF-8 and appends the resulting bytes to sink @p out.
  template <typename T> static inline void appendChar(char32_t ch, T& out) noexcept {
    if (ch < 0x80) {
      out += static_cast<char>(ch);
    } else if (ch < 0x800) {
      out += static_cast<char>((ch >> 6) | 0xC0);
      out += static_cast<char>((ch & 0x3F) | 0x80);
    } else if (ch < 0x10000) {
      out += static_cast<char>(((ch >> 12) & 0xF) | 0xE0);
      out += static_cast<char>(((ch >> 6) & 0x3F) | 0x80);
      out += static_cast<char>((ch & 0x3F) | 0x80);
    } else {
      out += static_cast<char>(((ch >> 18) & 0x7) | 0xF0);
      out += static_cast<char>(((ch >> 12) & 0x3F) | 0x80);
      out += static_cast<char>(((ch >> 6) & 0x3F) | 0x80);
      out += static_cast<char>((ch & 0x3F) | 0x80);
    }
  }

  /// Converts a Unicode codepoint @p ch into a std::string containing its UTF-8 encoding.
  static std::string toString(char32_t ch) noexcept {
    std::string out { };
    appendChar(ch, out);
    return out;
  }

  /// Decodes a UTF-8 byte buffer of length @p len into a vector of UTF-32 codepoints.
  static void toUtf32Str(const char* str, size_t len, std::vector<char32_t>& out) noexcept {
    auto end = str + len;
    while (str < end) {
      out.emplace_back(toChar32(str));
    }
  }

  /// Decodes a UTF-8 string view into a std::u32string.
  static std::u32string toUtf32Str(std::string_view str) noexcept {
    std::u32string out;
    out.reserve(str.length());
    auto pos = str.data();
    auto end = pos + str.length();
    while (pos < end) {
      out += toChar32(pos);
    }

    return out;
  }

  /// Encodes a UTF-32 string view into a UTF-8 container of type @p T (e.g. std::string).
  template <typename T> static T toUtf8Str(std::u32string_view str) noexcept {
    T out;
    out.reserve(str.length());
    for (auto ch : str) {
      appendChar(ch, out);
    }

    return out;
  }
};

/// Lightweight, non-owning view of a single UTF-8 encoded character in memory.
class Utf8Char {
public:
  constexpr Utf8Char(const char* str) noexcept
      : bytes(str) { }

  /// Returns the byte length (1 to 4) of this UTF-8 character.
  inline constexpr size_t len() const noexcept { return UtfUtils::nextCharLen(bytes); }

  /// Decodes and returns the character as a Char32 codepoint.
  inline constexpr Char32 char32() const noexcept {
    auto pos = bytes;
    return UtfUtils::toChar32(pos);
  }

  /// Returns a std::string_view over the UTF-8 bytes of this character.
  inline constexpr std::string_view view() const noexcept { return std::string_view(bytes, len()); }

  /// Returns a new std::string containing the UTF-8 bytes of this character.
  inline std::string string() const noexcept { return std::string(bytes, len()); }

  /// Returns true if this character points to a null terminator.
  inline constexpr bool isZero() const noexcept { return *bytes == 0; }

  constexpr operator std::string_view() const noexcept { return view(); }
  constexpr operator char32_t() const noexcept { return char32().ch; }

private:
  const char* const bytes;
};
constexpr Utf8Char Utf8ChZero { "" };
constexpr Utf8Char operator""_uc8(const char* str, size_t) noexcept { return str; }

/// Lightweight wrapper around a single 32-bit Unicode codepoint (char32_t).
class Utf32Char {
public:
  constexpr Utf32Char(char32_t ch)
      : ch(ch) { }

  /// Returns the byte length needed to encode this character in UTF-8.
  inline constexpr size_t len() const noexcept { return UtfUtils::charLen(ch); }

  inline constexpr char32_t operator()() const noexcept { return ch; }

  /// Encodes this codepoint into a UTF-8 std::string.
  inline std::string string() const noexcept { return UtfUtils::toString(ch); }

  constexpr operator char32_t() const noexcept { return ch; }
  operator std::string() const noexcept { return string(); }

private:
  char32_t ch;
};
constexpr Utf32Char operator""_uc32(char32_t ch) noexcept { return ch; };

/// Non-owning view over a UTF-8 string with codepoint-aware navigation and manipulation methods.
class Utf8String {
public:
  static constexpr Char32 leadingChar32(const char* str) noexcept { return UtfUtils::toChar32(str); }

  static constexpr Char32 trailingChar32(const char* end) noexcept {
    auto pos = end - UtfUtils::prevCharLen(end);
    return { UtfUtils::toChar32(pos) };
  }

  static constexpr Char32 leadingChar32(std::string_view str) noexcept { return leadingChar32(str.data()); }
  static constexpr Char32 trailingChar32(std::string_view str) noexcept {
    return trailingChar32(str.data() + str.length());
  }

  static constexpr Utf8Char trailingChar(const char* end) noexcept {
    auto len = UtfUtils::prevCharLen(end);
    return Utf8Char(end - len);
  }
  static constexpr Utf8Char trailingChar(std::string_view str) noexcept {
    return trailingChar(str.data() + str.length());
  }

  constexpr Utf8String(const char* str, size_t len) noexcept
      : start(str)
      , end(str + len) { }

  constexpr Utf8String(const std::string_view& str) noexcept
      : start(str.data())
      , end(start + str.length()) { }

  constexpr Utf8String(const char* begin, const char* end) noexcept
      : start(begin)
      , end(end) { }

  constexpr inline bool empty() const noexcept { return start >= end; }
  constexpr inline bool emptyAt(size_t offset) const noexcept { return start + offset >= end; }

  /// Returns a Utf8Char view of the first character, or Utf8ChZero if empty.
  constexpr Utf8Char front() const noexcept {
    if (empty()) {
      return Utf8ChZero;
    }

    return Utf8Char(start);
  }

  /// Returns a Utf8Char view at the given byte @p offset.
  constexpr Utf8Char operator[](size_t offset) const noexcept {
    if (start + offset >= end) {
      return Utf8ChZero;
    }

    return Utf8Char(start + offset);
  }

  /// Extracts the Utf8Char at byte @p offset and advances @p offset by the character's byte length.
  constexpr Utf8Char operator()(size_t& offset) const noexcept {
    if (start + offset >= end) {
      return Utf8ChZero;
    }

    Utf8Char ch(start + offset);
    offset += ch.len();
    return ch;
  }

  constexpr const char* const operator&() const noexcept { return start; }

  /// Returns a Utf8Char view of the last character in the string.
  constexpr Utf8Char back() const noexcept {
    if (empty()) {
      return Utf8ChZero;
    }

    auto len = UtfUtils::prevCharLen(end);
    return Utf8Char(end - len);
  }

  /// Returns a sub-view starting at byte @p offset with length @p len.
  constexpr const Utf8String substr(size_t offset, size_t len) noexcept { return Utf8String(start + offset, len); }

  constexpr operator std::string_view() const noexcept { return { start, static_cast<size_t>(end - start) }; }

  /// Splits the string by any character present in @p delims.
  std::vector<std::string_view> split(std::string_view delims) const noexcept {
    std::vector<std::string_view> strs { };
    const char *off1 { start }, *off2 { start };
    while (off2 < end) {
      auto next = UtfUtils::nextUtf8Char(off2);
      if (delims.find(next) != delims.npos) {
        strs.emplace_back(off1, static_cast<size_t>(off2 - off1));
        off2 += next.length();
        off1 = off2;
      } else {
        off2 += next.length();
      }
    }
    if (off1 != off2) {
      strs.emplace_back(off1, static_cast<size_t>(off2 - off1));
    }

    return strs;
  }

private:
  const char* start;
  const char* end;
};

/**
 * @brief User-defined string literal operator constructing a `Utf8String` view.
 *
 * Allows compile-time creation of a `Utf8String` directly from string literal syntax:
 * @code
 * constexpr auto str = "श्री"_us8;
 * @endcode
 *
 * @param str Pointer to the literal character array.
 * @param len Length of the string literal in bytes.
 * @return `Utf8String` viewing the literal character sequence.
 */
constexpr Utf8String operator""_us8(const char* str, size_t len) { return Utf8String(str, len); };

/**
 * @brief User-defined literal calculating byte length of a string literal.
 */
constexpr size_t operator""_len(const char* str, size_t len) { return std::string_view(str, len).length(); };

/**
 * @brief User-defined literal calculating codepoint count of a UTF-32 string literal.
 */
constexpr size_t operator""_len(const char32_t* str, size_t len) { return std::u32string_view(str, len).length(); };

/// Growable UTF-8 string sink backed by raw heap memory via malloc/realloc.
///
/// Designed to minimize allocation overhead and directly release a null-terminated
/// C buffer across the C ABI boundary via release() without an intermediate copy.
class Utf8StringBuilder {
public:
  /// Pre-allocates buffer capacity to at least @p size bytes.
  void reserve(size_t size) noexcept { grow(std::max(size, bufferSize)); }

  Utf8StringBuilder() noexcept { }

  Utf8StringBuilder(Utf8StringBuilder&& other) noexcept {
    buffer = other.buffer;
    other.buffer = nullptr;
    bufferSize = other.bufferSize;
    other.bufferSize = 1;
    ptr = other.ptr;
    other.ptr = nullptr;
    end = other.end;
    other.end = nullptr;
  }

  ~Utf8StringBuilder() noexcept {
    if (buffer != nullptr) {
      free(buffer);
      buffer = nullptr;
    }
  }

  /// Appends a single byte / ASCII character, doubling capacity if needed.
  inline void operator+=(char ch) noexcept {
    if (ptr == end) {
      grow(bufferSize * 2);
    }
    *ptr++ = ch;
  }

  /// Appends a string view, expanding capacity if needed.
  inline void operator+=(std::string_view str) noexcept {
    if (ptr + str.length() >= end) {
      grow(bufferSize * 2 + str.length());
    }
    for (auto ch : str) {
      *ptr++ = ch;
    }
  }

  /// Appends a null terminator, detaches ownership of the heap buffer, and returns it.
  /// The caller takes ownership and must release it with std::free / releaseBuffer.
  char* release() noexcept {
    (*this) += 0;
    auto buf = buffer;
    buffer = nullptr;
    return buf;
  }

  inline size_t size() const noexcept { return ptr - buffer; }

  inline size_t length() const noexcept { return ptr - buffer; }

  /// Returns the last UTF-8 character in the buffer.
  inline Utf8Char back() noexcept {
    auto len = UtfUtils::prevCharLen(ptr);
    return Utf8Char(ptr - len);
  }

  /// Removes the last UTF-8 character from the buffer.
  inline void pop_back() noexcept {
    auto len = UtfUtils::prevCharLen(ptr);
    ptr -= len;
  }

  /// Replaces @p numChars bytes at byte offset @p pos with @p repl.
  inline void replace(size_t pos, size_t numChars, std::string_view repl) noexcept {
    const auto replLen = repl.length();
    if (ptr + replLen >= end) {
      grow(bufferSize * 2 + replLen);
    }
    auto s = buffer + pos;
    auto e = s + numChars;
    if (e >= ptr) {
      ptr = s;
      (*this) += repl;
    } else {
      std::memmove(s + replLen, e, ptr - e);
      std::memcpy(s, repl.data(), replLen);
      ptr += (repl.length() - numChars);
    }
  }

private:
  inline void grow(size_t newSize) {
    size_t used = (ptr - buffer);
    bufferSize = std::max(used, newSize);
    buffer = static_cast<char*>(std::realloc(buffer, bufferSize));
    ptr = buffer + used;
    end = buffer + bufferSize;
  }

  size_t bufferSize { 1 };
  char* buffer { };
  char* ptr { };
  char* end { };
};
