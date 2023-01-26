#pragma once

#include "utf.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <memory>
#include <optional>
#include <unordered_map>

template <typename Key, typename Value, std::size_t Size> struct ConstexprMap {
  std::array<std::pair<Key, Value>, Size> data;

  [[nodiscard]] constexpr const Value* get(const Key& key) const noexcept {
    const auto itr = std::find_if(data.begin(), data.end(), [&key](const auto& v) { return v.first == key; });
    if (itr != data.end()) {
      return &itr->second;
    } else {
      return nullptr;
    }
  }

  [[nodiscard]] constexpr const typename std::array<std::pair<Key, Value>, Size>::const_iterator begin() const noexcept { return data.begin(); }
  [[nodiscard]] constexpr const typename std::array<std::pair<Key, Value>, Size>::const_iterator end() const noexcept { return data.end(); }
};

template <typename ValueType> struct TrieNode {
  using NodeMap = std::unordered_map<char32_t, std::unique_ptr<TrieNode<ValueType>>>;
  std::optional<ValueType> value { std::nullopt };
  std::unique_ptr<NodeMap> nodes { nullptr };
};

template <typename ValueType> class Char32Trie {
public:
  struct LookupResult {
    std::optional<ValueType> value { std::nullopt };
    size_t matchLen { 0 };
  };

  std::optional<ValueType> addLookup(std::string_view str, const ValueType& value) noexcept {
    auto u32Str = UtfUtils::toUtf32Str(str);
    return addLookup(u32Str, value);
  }

  std::optional<ValueType> addLookup(std::u32string_view u32Str, const ValueType& value) noexcept {
    const char32_t* text = u32Str.data();
    const char32_t* end = text + u32Str.length();
    auto* lookupNode = &root;

    while (text < end) {
      auto nextChar32 = *text;
      text++;
      auto nextChar = nextChar32;
      auto mapEntry = lookupNode->find(nextChar);
      if (mapEntry == lookupNode->end()) {
        mapEntry = lookupNode->emplace(nextChar, std::make_unique<TrieNode<ValueType>>()).first;
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
          lookup->nodes = std::make_unique<typename TrieNode<ValueType>::NodeMap>();
        }
        lookupNode = lookup->nodes.get();
      }
    }

    return std::nullopt;
  }

  LookupResult lookup(const char* text) const noexcept {
    LookupResult result {};
    auto* lookupNode = &root;
    const char* start = text;
    text = start;
    while (true) {
      auto ch = UtfUtils::toChar32(text);
      auto nextChar32 = ch.ch;
      if (nextChar32 == 0) {
        break;
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
  typename TrieNode<ValueType>::NodeMap root {};
};

template <typename CharType, typename ValueType> class StringMap {
public:
  struct LookupResult {
    std::optional<ValueType> value { std::nullopt };
    size_t matchLen { 0 };
  };

  std::optional<ValueType> addLookup(std::basic_string_view<CharType> strview, ValueType value) noexcept {
    std::basic_string<CharType> str { strview };
    longestMatch = std::max(longestMatch, str.length());

    auto result = map.insert({ str, value });
    if (!result.second) {
      return result.first->second;
    }
    return std::nullopt;
  }

  LookupResult lookup(const CharType* text, size_t offset) const noexcept {
    std::basic_string<CharType> str { text + offset, longestMatch };
    while (str.length() > 0) {
      auto entry = map.find(str);
      if (entry != map.end()) {
        return { entry->second, str.length() };
      }
      str.pop_back();
    }
    return {};
  }

private:
  size_t longestMatch { 0 };
  std::unordered_map<std::basic_string<CharType>, ValueType> map {};
};

template <class TimeT = std::chrono::milliseconds, class ClockT = std::chrono::steady_clock> class Timer {
  using timep_t = typename ClockT::time_point;

public:
  void tick() {
    _end = timep_t {};
    _start = ClockT::now();
  }

  void tock() { _end = ClockT::now(); }

  template <class TT = TimeT> TT duration() const {
    gsl_Expects(_end != timep_t {} && "toc before reporting");
    return std::chrono::duration_cast<TT>(_end - _start);
  }

private:
  timep_t _start = ClockT::now(), _end = {};
};

template <class TimeT = std::chrono::milliseconds, class ClockT = std::chrono::steady_clock> struct measure {
  template <class F, class... Args> static auto duration(F&& func, Args&&... args) {
    auto start = ClockT::now();
    std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
    return std::chrono::duration_cast<TimeT>(ClockT::now() - start);
  }
};

template <typename StringType> std::vector<StringType> split(const StringType& str, StringType delims) noexcept {
  std::vector<StringType> strs {};
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

template <typename CharType> std::size_t replaceAll(std::basic_string<CharType>& text, const std::basic_string_view<CharType>& what, const std::basic_string_view<CharType>& with) noexcept {
  std::size_t count {};
  for (size_t pos {}; text.npos != (pos = text.find(what.data(), pos, what.length())); pos += with.length(), ++count) {
    text.replace(pos, what.length(), with.data(), with.length());
  }
  return count;
}

template <typename CharType> std::size_t removeAll(std::basic_string<CharType>& inout, const std::basic_string_view<CharType>& what) {
  return replaceAll(inout, what, std::basic_string_view<CharType>());
}

class SwapConvert {
public:
  SwapConvert() { }

  SwapConvert(std::u32string_view p1, std::u32string_view p2)
      : pattern1(p1)
      , pattern2(p2)
      , swap(true) { }

  std::u32string toUtf32Str(std::string_view str) noexcept {
    std::u32string out;
    out.reserve(str.length());
    auto pos = str.data();
    auto end = pos + str.length();
    char32_t prev {};
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
  bool swap {};
  std::u32string_view pattern1;
  std::u32string_view pattern2;
};
