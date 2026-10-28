#pragma once

#include "inditrans.h"
#include "urdu_lexicon.h"
#include "token_stream.h"
#include "utf.h"

namespace inditrans::handlers {

/// Per-call Urdu-only resolver. No dictionary data is owned or initialized here.
class UrduPronunciationReader {
public:
  UrduPronunciationReader(std::string_view input, UrduLexicon entries) noexcept
      : begin(input.data()), lexicon(entries) { }

  bool hasPending() const noexcept { return !pronunciation.empty(); }

  bool pull(const char*& ptr, const char* end, std::string_view skipStart, std::string_view skipEnd,
      TranslitOptions options, TokenOrString* buffer, size_t& bufferSize) noexcept {
    if (!hasPending() && (ptr == end || !selectPronunciation(ptr, end, skipStart, skipEnd, options)))
      return false;
    pullPronunciation(buffer, bufferSize);
    return true;
  }

private:
  const char* begin;
  UrduLexicon lexicon;
  std::span<const TokenUnit> pronunciation;
  size_t pronunciationIndex = 0;

  // Be conservative around non-ASCII text: only ordinary ASCII punctuation,
  // whitespace and explicit Arabic punctuation are known word boundaries.
  static size_t boundaryLength(const char* p, const char* end) noexcept {
    if (p == end) return 0;
    const auto c = static_cast<unsigned char>(*p);
    if (c < 128) {
      if (c <= 32 || (c >= 33 && c <= 47) || (c >= 58 && c <= 64)
          || (c >= 91 && c <= 96) || (c >= 123 && c <= 126)) return 1;
      return 0;
    }
    if (end - p >= 2 && static_cast<unsigned char>(p[0]) == 0xd8
        && (static_cast<unsigned char>(p[1]) == 0x8c || static_cast<unsigned char>(p[1]) == 0x9b
            || static_cast<unsigned char>(p[1]) == 0x9f)) return 2;
    return 0;
  }

  static bool equalUrduWord(std::string_view source, std::string_view key) noexcept {
    auto canonical = [](std::string_view& s) -> std::string_view {
      if (s.starts_with("ك")) { s.remove_prefix(2); return "ک"; }
      if (s.starts_with("ي") || s.starts_with("ى")) { s.remove_prefix(2); return "ی"; }
      if (s.starts_with("ه")) { s.remove_prefix(2); return "ہ"; }
      auto n = UtfUtils::nextCharLen(s.data());
      if (n > s.size()) n = 1;
      auto result = s.substr(0, n); s.remove_prefix(n); return result;
    };
    while (!source.empty() && !key.empty())
      if (canonical(source) != canonical(key)) return false;
    return source.empty() && key.empty();
  }

  static bool unmarkedUrduWord(std::string_view word) noexcept {
    if (word.empty()) return false;
    while (!word.empty()) {
      if (word.size() < 2) return false;
      auto a = static_cast<unsigned char>(word[0]);
      auto b = static_cast<unsigned char>(word[1]);
      if (a < 0xd8 || a > 0xdd || (b & 0xc0) != 0x80) return false;
      unsigned cp = ((a & 0x1f) << 6) | (b & 0x3f);
      // Precomposed madd and hamza are meaningful written information too.
      if ((cp >= 0x621 && cp <= 0x626) || (cp >= 0x674 && cp <= 0x677)
          || cp == 0x6c0 || cp == 0x6c2 || cp == 0x6d3)
        return false;
      if (!((cp >= 0x621 && cp <= 0x63a) || (cp >= 0x641 && cp <= 0x64a)
          || (cp >= 0x66e && cp <= 0x66f) || (cp >= 0x671 && cp <= 0x6d3)
          || cp == 0x6d5 || (cp >= 0x6ee && cp <= 0x6ef)
          || (cp >= 0x6fa && cp <= 0x6fc) || (cp >= 0x750 && cp <= 0x77f))) return false;
      word.remove_prefix(2);
    }
    return true;
  }

  bool selectPronunciation(const char*& ptr, const char* end, std::string_view skipStart,
      std::string_view skipEnd, TranslitOptions options) noexcept {
    if (lexicon.empty() || options * TranslitOptions::DisableInference) return false;
    if (ptr != begin) {
      const char* prev = ptr - 1;
      while (prev > begin && (static_cast<unsigned char>(*prev) & 0xc0) == 0x80) --prev;
      if (boundaryLength(prev, ptr) != static_cast<size_t>(ptr - prev)) return false;
    }
    if (std::string_view(ptr, end - ptr).starts_with(skipStart)) return false;
    const char* wordEnd = ptr;
    while (wordEnd < end && !boundaryLength(wordEnd, end)) ++wordEnd;
    const std::string_view word(ptr, wordEnd - ptr);
    if (!unmarkedUrduWord(word) || word.find(skipStart) != word.npos || word.find(skipEnd) != word.npos) return false;
    for (const auto& entry : lexicon) {
      if (!entry.pronunciation.empty() && equalUrduWord(word, entry.word)) {
        pronunciation = entry.pronunciation;
        pronunciationIndex = 0;
        ptr = wordEnd;
        return true;
      }
    }
    return false;
  }

  void pullPronunciation(TokenOrString* buffer, size_t& bufferSize) noexcept {
    const auto& unit = pronunciation[pronunciationIndex++];
    buffer[bufferSize++] = TokenOrString(unit.leadToken.clone(ScriptType::Indic));
    if (unit.vowelMark.idx != InvalidToken)
      buffer[bufferSize++] = TokenOrString(ScriptToken(unit.vowelMark.tokenType, unit.vowelMark.idx, ScriptType::Indic));
    if (unit.otherDiacritic.idx != InvalidToken)
      buffer[bufferSize++] = TokenOrString(ScriptToken(unit.otherDiacritic.tokenType, unit.otherDiacritic.idx, ScriptType::Indic));
    if (pronunciationIndex == pronunciation.size()) pronunciation = {};
  }
};

/// Reject invalid canonical units as a whole, so callers get legacy fallback.
inline bool validUrduLexicon(UrduLexicon lexicon) noexcept {
  for (const auto& entry : lexicon) {
    if (entry.word.empty() || entry.pronunciation.empty()) return false;
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
