#pragma once

#include "pronunciation_reader.h"
#include "urdu_lexicon.h"

namespace inditrans::handlers {

struct UrduWordPolicy : ExactWordPolicy {
  // Compare alias-normalized UTF-8 words directly, without allocating.
  static std::string_view canonicalChar(std::string_view& s) noexcept {
    if (s.starts_with("ك")) { s.remove_prefix(2); return "ک"; }
    if (s.starts_with("ي") || s.starts_with("ى")) { s.remove_prefix(2); return "ی"; }
    if (s.starts_with("ه")) { s.remove_prefix(2); return "ہ"; }
    auto n = UtfUtils::nextCharLen(s.data());
    if (n > s.size()) n = 1;
    auto result = s.substr(0, n);
    s.remove_prefix(n);
    return result;
  }

  static int compare(std::string_view a, std::string_view b) noexcept {
    while (!a.empty() && !b.empty()) {
      const int comparison = canonicalChar(a).compare(canonicalChar(b));
      if (comparison) return comparison;
    }
    if (a.empty()) return b.empty() ? 0 : -1;
    return 1;
  }

  static bool eligible(std::string_view word) noexcept {
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

  static bool canonical(std::string_view word) noexcept {
    return !word.empty() && word.find("ك") == word.npos && word.find("ي") == word.npos
        && word.find("ى") == word.npos && word.find("ه") == word.npos;
  }
};

using UrduPronunciationReader = PronunciationReader<UrduWordPolicy>;
inline bool validUrduLexicon(UrduLexicon lexicon) noexcept {
  return validPronunciationLexicon<UrduWordPolicy>(lexicon);
}

} // namespace inditrans::handlers
