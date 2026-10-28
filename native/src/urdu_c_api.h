#pragma once

#include "urdu_lexicon_pack.h"
#include "reader_extension.h"
#include "static_scripts.h"
#include <cstdlib>
#include <cstring>

bool transliterateUrduWithLexicon(const std::string_view& input, const std::string_view& to,
    TranslitOptions options, UrduLexicon lexicon, std::string& output,
    const std::string_view& skipStart, const std::string_view& skipEnd) noexcept {
  if (options * TranslitOptions::DisableInference || lexicon.empty()
      || getScriptWriterMap(to) == getScriptWriterMap("urdu")
      || !inditrans::handlers::validUrduLexicon(lexicon))
    return transliterate(input, "urdu", to, options, output, skipStart, skipEnd);
  inditrans::handlers::UrduPronunciationReader reader(input, lexicon);
  auto extension = ReaderExtension::bind(reader);
  return transliterateWithReaderExtension(input, "urdu", to, options, output, extension, skipStart, skipEnd);
}

extern "C" char* CALL_CONV transliterateUrduWithLexiconData(const char* text, const char* to,
    unsigned long options, const char* data, unsigned long length, const char* skipStart, const char* skipEnd) {
  if (!text || !to) return nullptr;
#if defined(__cpp_exceptions)
  try {
#endif
  const auto bytes = data ? std::span(reinterpret_cast<const uint8_t*>(data), size_t(length))
                          : std::span<const uint8_t>();
  std::string result;
  if (!transliterateUrduWithPack(text, to, static_cast<TranslitOptions>(options), bytes, result,
          skipStart ? skipStart : "##", skipEnd ? skipEnd : "##")) return nullptr;
  auto* buffer = static_cast<char*>(std::malloc(result.size() + 1));
  if (!buffer) return nullptr;
  std::memcpy(buffer, result.c_str(), result.size() + 1);
  return buffer;
#if defined(__cpp_exceptions)
  } catch (...) {
    // If constructing the optional pack fails, retain the legacy reader.
    try {
      return ::transliterate(text, "urdu", to, options, skipStart ? skipStart : "##", skipEnd ? skipEnd : "##");
    } catch (...) {
      return nullptr;
    }
  }
#endif
}
