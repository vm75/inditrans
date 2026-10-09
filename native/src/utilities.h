#pragma once

#include "trie.h"
#include "utf.h"
#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <unordered_map>

#ifdef _MSC_VER
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

namespace detail {

/**
 * @brief Functor providing case-insensitive ordering for string views.
 *
 * Uses POSIX `strcasecmp` (or Windows `_stricmp`); both views must point to
 * null-terminated strings because the comparison does not use their lengths.
 * Primarily used in associative containers requiring case-insensitive string lookups.
 */
struct CaseInsensitiveComparator {
  bool operator()(const std::string_view& a, const std::string_view& b) const noexcept {
    return ::strcasecmp(a.data(), b.data()) < 0;
  }
};

} // namespace detail

/**
 * @brief Fixed-capacity compile-time key-value dictionary.
 *
 * Implements a `constexpr` lookup table backed by a contiguous `std::array` of pairs.
 * Lookup is performed via linear search, making it optimal for small datasets where
 * zero dynamic allocation, compile-time evaluation, and cache locality are paramount.
 *
 * @tparam Key Key type.
 * @tparam Value Associated value type.
 * @tparam Size Total number of key-value pairs stored in the table.
 */
template <typename Key, typename Value, std::size_t Size> struct ConstexprMap {
  std::array<std::pair<Key, Value>, Size> data;

  /**
   * @brief Finds the value associated with the given key.
   * @param key The key to look up.
   * @return Pointer to the stored value if found; nullptr otherwise.
   */
  [[nodiscard]] constexpr const Value* get(const Key& key) const noexcept {
    const auto itr = std::find_if(data.begin(), data.end(), [&key](const auto& v) { return v.first == key; });
    if (itr != data.end()) {
      return &itr->second;
    } else {
      return nullptr;
    }
  }

  [[nodiscard]] constexpr const typename std::array<std::pair<Key, Value>, Size>::const_iterator
  begin() const noexcept {
    return data.begin();
  }
  [[nodiscard]] constexpr const typename std::array<std::pair<Key, Value>, Size>::const_iterator end() const noexcept {
    return data.end();
  }
};

/**
 * @brief Dynamic heap-allocated prefix tree (trie) keyed by UTF-32 codepoints.
 *
 * Used for dynamic dictionary lookups and multi-codepoint prefix matching.
 * Converts input UTF-8 sequences to UTF-32 on the fly, allowing longest-prefix
 * matching with optional ASCII case-insensitivity.
 *
 * @tparam ValueType Associated value type mapped to matched prefixes.
 */
template <typename ValueType> class Char32Trie {
public:
  /**
   * @brief Result of a prefix lookup in the trie.
   */
  struct LookupResult {
    std::optional<ValueType> value { std::nullopt }; ///< Matched value, or nullopt if no match.
    size_t matchLen { 0 }; ///< Byte length of the matched prefix in the UTF-8 input.
  };

  /**
   * @brief Inserts a UTF-8 string key into the trie.
   *
   * Converts `str` to UTF-32 and delegates to the UTF-32 overload.
   *
   * @param str UTF-8 string key.
   * @param value Value to map to the key.
   * @return Previous value if the key already existed with a different value; nullopt otherwise.
   */
  std::optional<ValueType> addLookup(std::string_view str, const ValueType& value) noexcept {
    auto u32Str = UtfUtils::toUtf32Str(str);
    return addLookup(u32Str, value);
  }

  /**
   * @brief Inserts a UTF-32 sequence key into the trie.
   *
   * Traverses or allocates nodes for each codepoint in `u32Str`.
   *
   * @param u32Str UTF-32 key view.
   * @param value Value to associate with the key.
   * @return Previous value if the key already existed with a different value; nullopt otherwise.
   */
  std::optional<ValueType> addLookup(std::u32string_view u32Str, const ValueType& value) noexcept {
    const char32_t* text = u32Str.data();
    const char32_t* end = text + u32Str.length();
    auto* lookupNode = &root;

    while (text < end) {
      auto nextChar = *text++;
      auto mapEntry = lookupNode->find(nextChar);
      if (mapEntry == lookupNode->end()) {
        mapEntry = lookupNode->emplace(nextChar, std::make_unique<TrieNode<char32_t, ValueType>>()).first;
      }

      auto& lookup = mapEntry->second;

      if (text >= end) {
        // last lookupNode is the end of a word
        if (lookup->value != std::nullopt) {
          if (lookup->value != value) {
            return lookup->value;
          }
          continue;
        }
        lookup->value = value;
      } else {
        if (lookup->nodes == nullptr) {
          lookup->nodes = std::make_unique<typename TrieNode<char32_t, ValueType>::NodeMap>();
        }
        lookupNode = lookup->nodes.get();
      }
    }

    return std::nullopt;
  }

  /**
   * @brief Finds the longest matching prefix for a UTF-8 text pointer.
   *
   * Iteratively decodes UTF-8 codepoints using UtfUtils::toChar32 and advances down
   * the trie nodes. Tracks the deepest node that holds an associated value.
   *
   * @param text Pointer to null-terminated or valid UTF-8 character stream.
   * @param caseInsensitive If true, folds ASCII uppercase letters ('A'-'Z') to lowercase.
   * @return LookupResult containing the matched value and the matching UTF-8 byte length.
   */
  LookupResult lookup(const char* text, bool caseInsensitive = false) const noexcept {
    LookupResult result { };
    auto* lookupNode = &root;
    const char* start = text;
    text = start;
    while (true) {
      auto ch = UtfUtils::toChar32(text);
      auto nextChar32 = ch.ch;
      if (nextChar32 == 0) {
        break;
      }
      if (caseInsensitive && nextChar32 >= 'A' && nextChar32 <= 'Z') {
        nextChar32 += 'a' - 'A';
      }
      auto mapEntry = lookupNode->find(nextChar32);
      if (mapEntry == lookupNode->end()) {
        break;
      }

      auto& lookup = mapEntry->second;

      if (lookup->value != std::nullopt) {
        result.value = lookup->value;
        result.matchLen = text - start;
      }

      if (!lookup->nodes) {
        break;
      }
      lookupNode = lookup->nodes.get();
    }
    return result;
  }

private:
  typename TrieNode<char32_t, ValueType>::NodeMap root { };
};

/**
 * @brief Hash-map based longest-prefix lookup table.
 *
 * Maintains a hash map of string keys to values, tracking the maximum key length (`longestMatch`).
 * During lookup, checks candidate prefixes of decreasing lengths from `longestMatch` down to 1.
 *
 * @tparam CharType Character type (e.g. char, char32_t).
 * @tparam ValueType Value type mapped to each string key.
 */
template <typename CharType, typename ValueType> class StringMap {
public:
  /**
   * @brief Result of a prefix lookup in the StringMap.
   */
  struct LookupResult {
    std::optional<ValueType> value { std::nullopt }; ///< Matched value, or nullopt if no match found.
    size_t matchLen { 0 }; ///< Number of characters matched.
  };

  /**
   * @brief Inserts a string view and associated value into the map.
   *
   * Updates `longestMatch` if the inserted string length exceeds the current maximum.
   *
   * @param strview The string key.
   * @param value The value to associate.
   * @return Previous value if the key already existed; nullopt otherwise.
   */
  std::optional<ValueType> addLookup(std::basic_string_view<CharType> strview, ValueType value) noexcept {
    std::basic_string<CharType> str { strview };
    longestMatch = std::max(longestMatch, str.length());

    auto result = map.insert({ str, value });
    if (!result.second) {
      return result.first->second;
    }
    return std::nullopt;
  }

  /**
   * @brief Looks up the longest matching prefix at `text + offset`.
   *
   * Constructs candidate strings of length up to `longestMatch` and truncates
   * backwards until a match is found in the hash map.
   *
   * @param text Pointer to characters.
   * @param offset Character offset from `text` to start matching.
   * @return LookupResult with matched value and character length.
   */
  LookupResult lookup(const CharType* text, size_t offset) const noexcept {
    std::basic_string<CharType> str { text + offset, longestMatch };
    while (str.length() > 0) {
      auto entry = map.find(str);
      if (entry != map.end()) {
        return { entry->second, str.length() };
      }
      str.pop_back();
    }
    return { };
  }

private:
  size_t longestMatch { 0 };
  std::unordered_map<std::basic_string<CharType>, ValueType> map { };
};

/**
 * @brief Splits a string into sub-tokens delimited by any character in `delims`.
 *
 * @tparam StringType String or string_view type supporting data(), length(), and find().
 * @param str Input string to partition.
 * @param delims Collection of delimiter characters.
 * @return Vector of substring slices.
 */
template <typename StringType> std::vector<StringType> split(const StringType& str, StringType delims) noexcept {
  std::vector<StringType> strs { };
  const auto *off1 { str.data() }, *off2 { off1 };
  const auto end = off1 + str.length();
  while (off2 < end) {
    auto next = *off2;
    if (delims.find(next) != delims.npos) {
      strs.emplace_back(off1, static_cast<size_t>(off2 - off1));
      off2++;
      off1 = off2;
    } else {
      off2++;
    }
  }
  if (off1 != off2) {
    strs.emplace_back(off1, static_cast<size_t>(off2 - off1));
  }

  return strs;
}

/**
 * @brief Replaces all occurrences of a target substring within `text` in place.
 *
 * @tparam CharType Character type of the string.
 * @param text String modified in place.
 * @param what Substring pattern to search for.
 * @param with Replacement substring.
 * @return Total number of substitutions performed.
 */
template <typename CharType>
std::size_t replaceAll(std::basic_string<CharType>& text, const std::basic_string_view<CharType>& what,
    const std::basic_string_view<CharType>& with) noexcept {
  std::size_t count { };
  for (size_t pos { }; text.npos != (pos = text.find(what.data(), pos, what.length())); pos += with.length(), ++count) {
    text.replace(pos, what.length(), with.data(), with.length());
  }
  return count;
}

/**
 * @brief Removes all occurrences of a target substring within `inout` in place.
 *
 * @tparam CharType Character type of the string.
 * @param inout String modified in place.
 * @param what Substring to strip.
 * @return Total number of occurrences removed.
 */
template <typename CharType>
std::size_t removeAll(std::basic_string<CharType>& inout, const std::basic_string_view<CharType>& what) {
  return replaceAll(inout, what, std::basic_string_view<CharType>());
}

/**
 * @brief UTF-8 to UTF-32 decoder with adjacent character inversion/swapping.
 *
 * In certain Indic scripts or Romanization systems, combining marks or diacritics
 * may occur out of logical order (e.g. pre-base matras or virama-consonant sequences).
 * If the previous character matches `pattern1` and the incoming character matches
 * `pattern2`, SwapConvert swaps their emitted order in the resulting UTF-32 string.
 */
class SwapConvert {
public:
  SwapConvert() { }

  /**
   * @brief Constructs a SwapConvert rule pair.
   * @param p1 Characters that trigger a swap when appearing immediately before `p2`.
   * @param p2 Characters that trigger a swap when appearing immediately after `p1`.
   */
  SwapConvert(std::u32string_view p1, std::u32string_view p2)
      : pattern1(p1)
      , pattern2(p2)
      , swap(true) { }

  /**
   * @brief Decodes a UTF-8 string to UTF-32, applying the swap rule to adjacent pairs.
   * @param str UTF-8 input string view.
   * @return Decoded UTF-32 string with matching pairs transposed.
   */
  std::u32string toUtf32Str(std::string_view str) noexcept {
    std::u32string out;
    out.reserve(str.length());
    auto pos = str.data();
    auto end = pos + str.length();
    char32_t prev { };
    while (pos < end) {
      char32_t ch = UtfUtils::toChar32(pos);
      if (swap && pattern2.find(ch) != pattern2.npos && pattern1.find(prev) != pattern1.npos) {
        out.pop_back();
        out += ch;
        out += prev;
      } else {
        out += ch;
      }
      prev = ch;
    }

    return out;
  }

private:
  bool swap { };
  std::u32string_view pattern1;
  std::u32string_view pattern2;
};
