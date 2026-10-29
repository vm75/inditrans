#pragma once

#include "pronunciation_pack.h"
#include "urdu_pronunciation.h"

using UrduLexiconPack = BasicPronunciationLexiconPack<inditrans::handlers::UrduWordPolicy>;

/// The caller supplies the asset bytes, owned only during this call. Invalid
/// assets fall back to the existing Urdu reader; no global state is created.
inline bool transliterateUrduWithPack(std::string_view text, std::string_view to, TranslitOptions options,
    std::span<const uint8_t> data, std::string& output, std::string_view skipStart = "##",
    std::string_view skipEnd = "##") {
  UrduLexiconPack pack;
  pack.load(data);
  auto lookup = pack.lexicon();
  if (lookup.empty() || options * TranslitOptions::DisableInference || to == "urdu" || to == "nastaliq")
    return transliterate(text, "urdu", to, options, output, skipStart, skipEnd);
  inditrans::handlers::UrduPronunciationReader reader(text, lookup);
  auto extension = ReaderExtension::bind(reader);
  return transliterateWithReaderExtension(text, "urdu", to, options, output, extension, skipStart, skipEnd);
}
