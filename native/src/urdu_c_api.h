#pragma once

#include "urdu_lexicon_pack.h"
#include "reader_extension.h"
#include "static_scripts.h"
#include <cstdlib>
#include <cstring>
#include <memory>

/// Assumes a lexicon already validated by UrduLexiconPack::load(). The public
/// span-based C++ API still validates caller-provided arrays on every call.
static bool transliterateUrduWithValidatedLexicon(std::string_view input, std::string_view to,
    TranslitOptions options, UrduLexicon lexicon, std::string& output,
    std::string_view skipStart, std::string_view skipEnd) noexcept {
  if (options * TranslitOptions::DisableInference || lexicon.empty()
      || getScriptWriterMap(to) == getScriptWriterMap("urdu"))
    return transliterate(input, "urdu", to, options, output, skipStart, skipEnd);
  inditrans::handlers::UrduPronunciationReader reader(input, lexicon);
  auto extension = ReaderExtension::bind(reader);
  return transliterateWithReaderExtension(input, "urdu", to, options, output, extension, skipStart, skipEnd);
}

bool transliterateUrduWithLexicon(const std::string_view& input, const std::string_view& to,
    TranslitOptions options, UrduLexicon lexicon, std::string& output,
    const std::string_view& skipStart, const std::string_view& skipEnd) noexcept {
  if (!inditrans::handlers::validUrduLexicon(lexicon))
    return transliterate(input, "urdu", to, options, output, skipStart, skipEnd);
  return transliterateUrduWithValidatedLexicon(input, to, options, lexicon, output, skipStart, skipEnd);
}

namespace {
struct UrduLexiconHandle {
  UrduLexiconPack pack;
};

char* allocateUrduResult(const std::string& value) noexcept {
  auto* buffer = static_cast<char*>(std::malloc(value.size() + 1));
  if (!buffer) return nullptr;
  std::memcpy(buffer, value.c_str(), value.size() + 1);
  return buffer;
}
} // namespace

/// Parse once, then convert any number of times without another CRC pass or
/// per-word allocation. The caller must release the opaque handle exactly once.
extern "C" void* CALL_CONV createUrduLexiconHandle(const char* data, unsigned long length) {
  if (!data || !length) return nullptr;
#if defined(__cpp_exceptions)
  try {
#endif
    auto handle = std::make_unique<UrduLexiconHandle>();
    if (!handle->pack.load(std::span(reinterpret_cast<const uint8_t*>(data), size_t(length))))
      return nullptr;
    return handle.release();
#if defined(__cpp_exceptions)
  } catch (...) {
    return nullptr;
  }
#endif
}

extern "C" void CALL_CONV releaseUrduLexiconHandle(void* handle) {
  delete static_cast<UrduLexiconHandle*>(handle);
}

extern "C" char* CALL_CONV transliterateUrduWithLexiconHandle(const char* text, const char* to,
    unsigned long options, const void* handle, const char* skipStart, const char* skipEnd) {
  if (!text || !to) return nullptr;
#if defined(__cpp_exceptions)
  try {
#endif
    const auto lexicon = handle ? static_cast<const UrduLexiconHandle*>(handle)->pack.lexicon() : UrduLexicon {};
    std::string result;
    if (!transliterateUrduWithValidatedLexicon(text, to, static_cast<TranslitOptions>(options),
            lexicon, result, skipStart ? skipStart : "##", skipEnd ? skipEnd : "##"))
      return nullptr;
    return allocateUrduResult(result);
#if defined(__cpp_exceptions)
  } catch (...) {
    return ::transliterate(text, "urdu", to, options, skipStart ? skipStart : "##", skipEnd ? skipEnd : "##");
  }
#endif
}

/// Backward-compatible one-shot API. New callers should prefer a reusable
/// handle when they intend to convert more than one text.
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
    return allocateUrduResult(result);
#if defined(__cpp_exceptions)
  } catch (...) {
    return ::transliterate(text, "urdu", to, options, skipStart ? skipStart : "##", skipEnd ? skipEnd : "##");
  }
#endif
}
