#pragma once

#include "pronunciation_pack.h"
#include "urdu_pronunciation.h"
#include "static_scripts.h"

/// The caller owns a previously validated immutable lookup view.
template <typename Policy>
static bool transliterateUsingLookup(std::string_view input, std::string_view from,
    std::string_view to, TranslitOptions options, PronunciationLexiconLookup lookup,
    std::string& output, std::string_view skipStart, std::string_view skipEnd) noexcept {
  if (options * TranslitOptions::DisableInference || lookup.empty()
      || getScriptWriterMap(from) == getScriptWriterMap(to))
    return transliterate(input, from, to, options, output, skipStart, skipEnd);
  inditrans::handlers::PronunciationReader<Policy> reader(input, lookup);
  auto extension = ReaderExtension::bind(reader);
  return transliterateWithReaderExtension(input, from, to, options, output, extension, skipStart, skipEnd);
}

bool transliterateWithLexicon(std::string_view input, std::string_view from,
    std::string_view to, TranslitOptions options, const PronunciationLexiconPack& pack,
    std::string& output, std::string_view skipStart, std::string_view skipEnd) noexcept {
  // Urdu preserves its established alias/marked-word policy. Other scripts use
  // exact normalized keys; additional policies can be registered here.
  if (getScriptReaderMap(from) == getScriptReaderMap("urdu"))
    return transliterateUsingLookup<inditrans::handlers::UrduWordPolicy>(
        input, from, to, options, pack.lexicon(), output, skipStart, skipEnd);
  return transliterateUsingLookup<inditrans::handlers::ExactWordPolicy>(
      input, from, to, options, pack.lexicon(), output, skipStart, skipEnd);
}
