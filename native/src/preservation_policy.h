#pragma once

#include "inditrans.h"
#include "script_constants.h"
#include "static_scripts.h"
#include "token_stream.h"
#include "utf.h"
#include <string_view>

namespace inditrans::handlers {

/// Manages extraction of non-transliterated text spans from the input stream:
/// 1. XML/HTML tags (e.g. `<tag>`) unless NoXMLTagHandling is set
/// 2. Protected skip spans delimited by skipStart and skipEnd (e.g. `##protected##`)
/// 3. Unrecognized character sequences that do not match any trie prefix
class PreservedSpanHandler {
public:
  template <ReaderPolicy Policy, typename TokenOutType>
  [[gnu::noinline]] static const char* pullUnrecognized(const char* ptr, const char* end,
      const ScriptReaderMap& map, const std::string_view& skipStart, const std::string_view& skipEnd,
      TranslitOptions options, TokenOutType* outBuffer, size_t& outSize) noexcept {
    const bool skipXml = !(options * TranslitOptions::NoXMLTagHandling);
    const auto* start = ptr;
    if (skipXml && *ptr == '<') {
      const auto close = std::string_view(ptr, end - ptr).find('>');
      ptr = close == std::string_view::npos ? end : ptr + close + 1;
      outBuffer[outSize++] = TokenOutType(std::string_view(start, ptr - start));
    } else if (!skipStart.empty() && *ptr == skipStart.front()
        && skipStart.size() <= static_cast<size_t>(end - ptr)
        && std::string_view(ptr, skipStart.size()) == skipStart) {
      ptr += skipStart.length();
      start = ptr;
      const auto close = skipEnd.empty() ? std::string_view::npos : std::string_view(ptr, end - ptr).find(skipEnd);
      if (close == std::string_view::npos) {
        // An unmatched opening delimiter discards the remaining input.
        ptr = end;
      } else {
        outBuffer[outSize++] = TokenOutType(std::string_view(start, close));
        ptr += close + skipEnd.length();
      }
    } else {
      // The trie consumes bytes, so raw runs can be scanned without decoding;
      // keep their original bytes intact until a token or delimiter starts.
      ptr++;
      while (ptr < end && (skipStart.empty() || *ptr != skipStart.front())
          && *ptr != '<' && map.lookupToken<Policy>(ptr, end).sequence == 0) {
        ptr++;
      }
      outBuffer[outSize++] = TokenOutType(std::string_view(start, ptr - start));
    }
    return ptr;
  }
};

/// Filters internal non-ASCII disambiguation markers (e.g. 'ʽ', 'ʼ', 'ˮ', 'ˇ')
/// from the emitted text runs unless RetainSpecialMarkers is set.
class SpecialMarkerFilter {
public:
  template <typename Sink>
  static void push(const std::string_view& text, Sink& buffer, TranslitOptions options) noexcept {
    if (options / TranslitOptions::RetainSpecialMarkers) {
      stripSpecialMarkers(text, buffer);
    } else {
      buffer += text;
    }
  }

  template <typename BufType>
  static void stripSpecialMarkers(const std::string_view& in, BufType& out) noexcept {
    if (in.length() == 1 && static_cast<uint8_t>(in.front()) < 0x80) {
      out += in;
      return;
    }
    // Every marker byte is non-ASCII; preserve the substring predicate for
    // non-ASCII input, including the existing treatment of stray UTF-8 bytes.
    static_assert([] {
      for (const auto byte : SpecialMarkers)
        if (static_cast<uint8_t>(byte) < 0x80)
          return false;
      return true;
    }());
    const char* ptr = in.data() ? in.data() : "";
    const char* end = ptr + in.length();
    const char* run = ptr;
    while (ptr < end) {
      const auto remaining = static_cast<size_t>(end - ptr);
      auto charLength = UtfUtils::nextCharLen(ptr);
      if (charLength > remaining) {
        charLength = 1;
      } else {
        for (size_t offset = 1; offset < charLength; ++offset) {
          if ((static_cast<uint8_t>(ptr[offset]) & 0xc0) != 0x80) {
            charLength = 1;
            break;
          }
        }
      }
      const auto ch = std::string_view(ptr, charLength);
      if (static_cast<uint8_t>(ch.front()) >= 0x80 && SpecialMarkers.find(ch) != SpecialMarkers.npos) {
        if (ptr != run)
          out += std::string_view(run, ptr - run);
        ptr += ch.length();
        run = ptr;
      } else {
        // Incomplete/invalid sequences are preserved a byte at a time. Marker
        // strings are complete UTF-8 sequences, so no truncated prefix can match one.
        ptr += ch.length();
      }
    }
    if (ptr != run)
      out += std::string_view(run, ptr - run);
  }
};

} // namespace inditrans::handlers
