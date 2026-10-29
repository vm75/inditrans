#pragma once

#include "inditrans.h"
#include "pronunciation_lexicon.h"
#include "token_stream.h"
#include "utf.h"

namespace inditrans::handlers {

/// Generic keys match exactly. Compiler inputs are normalized to NFC; source
/// adapters can supply aliases and eligibility restrictions as a static policy.
struct ExactWordPolicy {
  static bool eligible(std::string_view word) noexcept { return !word.empty(); }
  static bool canonical(std::string_view word) noexcept { return !word.empty(); }
  static int compare(std::string_view a, std::string_view b) noexcept { return a.compare(b); }
  // Treat common Unicode punctuation and spaces as word boundaries.
  // Keep ZWNJ/ZWJ and bidi controls conservative: words containing them fall
  // through to the existing orthographic reader.
  static size_t boundaryLength(const char* p, const char* end) noexcept {
    if (p == end) return 0;
    const auto a = static_cast<uint8_t>(p[0]);
    if (a < 128)
      return a <= 32 || (a >= 33 && a <= 47) || (a >= 58 && a <= 64)
          || (a >= 91 && a <= 96) || (a >= 123 && a <= 126) ? 1 : 0;
    if (end - p >= 2) {
      const auto b = static_cast<uint8_t>(p[1]);
      if (a == 0xc2 && (b == 0xa0 || b == 0xab || b == 0xbb)) return 2;
      if (a == 0xd8 && (b == 0x8c || b == 0x9b || b == 0x9f)) return 2;
      if (a == 0xd9 && b >= 0xa0 && b <= 0xa9) return 2;
      if (a == 0xdb && (b == 0x94 || (b >= 0xb0 && b <= 0xb9))) return 2;
    }
    if (end - p >= 3) {
      const auto b = static_cast<uint8_t>(p[1]), c = static_cast<uint8_t>(p[2]);
      if (a == 0xe2 && b == 0x80
          && ((c >= 0x80 && c <= 0x8a) || (c >= 0x90 && c <= 0x95)
              || (c >= 0x98 && c <= 0x9f) || (c >= 0xa8 && c <= 0xa9)
              || c == 0xa6 || c == 0xaf)) return 3;
      if (a == 0xe2 && b == 0x81 && c == 0x9f) return 3;
      if (a == 0xe3 && b == 0x80 && c == 0x80) return 3;
      if (a == 0xe0 && b == 0xa5 && (c == 0xa4 || c == 0xa5)) return 3;
    }
    return 0;
  }

};

/// Per-call exact-word pronunciation reader. No dictionary data is owned or initialized here.
template <typename Policy = ExactWordPolicy> class PronunciationReader {
public:
  PronunciationReader(std::string_view input, PronunciationLexicon entries) noexcept
      : begin(input.data()), lexicon {entries} { }

  PronunciationReader(std::string_view input, PronunciationLexiconLookup lookup) noexcept
      : begin(input.data()), lexicon(lookup) { }

  bool hasPending() const noexcept { return pendingSize != 0; }

  bool pull(const char*& ptr, const char* end, std::string_view skipStart, std::string_view skipEnd,
      TranslitOptions options, TokenOrString* buffer, size_t& bufferSize) noexcept {
    if (!hasPending() && (ptr == end || !selectPronunciation(ptr, end, skipStart, skipEnd, options)))
      return false;
    pullPronunciation(buffer, bufferSize);
    return true;
  }

private:
  const char* begin;
  PronunciationLexiconLookup lexicon;
  const void* pending = nullptr;
  size_t pendingSize = 0;
  size_t pronunciationIndex = 0;

  bool selectPronunciation(const char*& ptr, const char* end, std::string_view skipStart,
      std::string_view skipEnd, TranslitOptions options) noexcept {
    if (lexicon.empty() || options * TranslitOptions::DisableInference) return false;
    if (ptr != begin) {
      const char* prev = ptr - 1;
      while (prev > begin && (static_cast<unsigned char>(*prev) & 0xc0) == 0x80) --prev;
      if (Policy::boundaryLength(prev, ptr) != static_cast<size_t>(ptr - prev)) return false;
    }
    if (std::string_view(ptr, end - ptr).starts_with(skipStart)) return false;
    const char* wordEnd = ptr;
    while (wordEnd < end && !Policy::boundaryLength(wordEnd, end)) ++wordEnd;
    const std::string_view word(ptr, wordEnd - ptr);
    if (!Policy::eligible(word) || word.find(skipStart) != word.npos || word.find(skipEnd) != word.npos) return false;
    if (lexicon.packed()) {
#if defined(__cpp_exceptions)
      try {
#endif
        auto value = lexicon.packed()->find(word, Policy::compare);
        if (value.empty()) return false;
        // The Urdu payload starts with a reading count and first-unit count.
        const size_t count = value[2] | (size_t(value[3]) << 8);
        pending = value.data() + 4;
        pendingSize = count * 2;
        ptr = wordEnd;
        return true;
#if defined(__cpp_exceptions)
      } catch (...) { return false; }
#endif
    }
    const auto entries = lexicon.entries();
    // Canonical keys are strictly sorted; lookup is O(log vocabulary).
    size_t low = 0, high = entries.size();
    while (low < high) {
      const auto middle = low + (high - low) / 2;
      if (Policy::compare(entries[middle].word, word) < 0)
        low = middle + 1;
      else
        high = middle;
    }
    if (low < entries.size() && Policy::compare(entries[low].word, word) == 0) {
      pending = entries[low].pronunciation.data();
      pendingSize = entries[low].pronunciation.size();
      pronunciationIndex = 0;
      ptr = wordEnd;
      return true;
    }
    return false;
  }

  void pullPronunciation(TokenOrString* buffer, size_t& bufferSize) noexcept {
    TokenUnit decoded({TokenType::Vowel, 0, ScriptType::Indic});
    const bool packed = lexicon.packed() != nullptr;
    if (packed) {
      const auto* packedPronunciation = static_cast<const uint8_t*>(pending);
      const unsigned bits = packedPronunciation[0] | (unsigned(packedPronunciation[1]) << 8);
      const unsigned lead = bits & 127, mark = (bits >> 7) & 31, other = bits >> 12;
      decoded.leadToken = {lead < 22 ? TokenType::Vowel : TokenType::Consonant,
          static_cast<uint8_t>(lead < 22 ? lead : lead - 22), ScriptType::Indic};
      if (mark != 22) decoded.vowelMark = {TokenType::VowelMark, static_cast<uint8_t>(mark)};
      if (other != 8) decoded.otherDiacritic = {TokenType::OtherDiacritic, static_cast<uint8_t>(other)};
      pending = packedPronunciation + 2;
      pendingSize -= 2;
    }
    const auto& unit = packed ? decoded : static_cast<const TokenUnit*>(pending)[pronunciationIndex++];
    buffer[bufferSize++] = TokenOrString(unit.leadToken.clone(ScriptType::Indic));
    if (unit.vowelMark.idx != InvalidToken)
      buffer[bufferSize++] = TokenOrString(ScriptToken(unit.vowelMark.tokenType, unit.vowelMark.idx, ScriptType::Indic));
    if (unit.otherDiacritic.idx != InvalidToken)
      buffer[bufferSize++] = TokenOrString(ScriptToken(unit.otherDiacritic.tokenType, unit.otherDiacritic.idx, ScriptType::Indic));
    if (!packed && pronunciationIndex == pendingSize) pendingSize = 0;
  }
};

/// Reject invalid canonical units as a whole, so callers get legacy fallback.
template <typename Policy> inline bool validPronunciationLexicon(PronunciationLexicon lexicon) noexcept {
  std::string_view previous;
  for (const auto& entry : lexicon) {
    if (!Policy::canonical(entry.word) || entry.pronunciation.empty()
        || (!previous.empty() && entry.word <= previous))
      return false;
    previous = entry.word;
    for (const auto& unit : entry.pronunciation) {
      const bool consonant = unit.leadToken.tokenType == TokenType::Consonant;
      if ((!consonant && unit.leadToken.tokenType != TokenType::Vowel)
          || unit.leadToken.idx >= (consonant ? 64 : 22)
          || (unit.vowelMark.idx != InvalidToken && (!consonant || unit.vowelMark.tokenType != TokenType::VowelMark || unit.vowelMark.idx >= 22))
          || (unit.otherDiacritic.idx != InvalidToken && (unit.otherDiacritic.tokenType != TokenType::OtherDiacritic || unit.otherDiacritic.idx >= 8))
          || unit.accent.idx != InvalidToken) return false;
    }
  }
  return true;
}

} // namespace inditrans::handlers
