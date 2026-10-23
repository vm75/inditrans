#include "inditrans.h"
#include "preservation_policy.h"
#include "utf.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

namespace {
bool failNextRealloc = false;
}

extern "C" void* __real_realloc(void*, size_t);
extern "C" void* __wrap_realloc(void* pointer, size_t size) {
  if (failNextRealloc) {
    failNextRealloc = false;
    return nullptr;
  }
  return __real_realloc(pointer, size);
}

int main(int argc, char** argv) {
  if (argc != 2)
    return 2;
  const std::string_view mode(argv[1]);

  if (mode == "truncated-marker") {
    const char bytes[] = { char(0xe2) };
    std::string output;
    inditrans::handlers::SpecialMarkerFilter::stripSpecialMarkers(std::string_view(bytes, sizeof(bytes)), output);
    return output.size() == 1 && static_cast<unsigned char>(output[0]) == 0xe2 ? 0 : 3;
  } else if (mode == "output-alias") {
    std::string output;
    for (int i = 0; i < 1024; ++i)
      output += "क";
    const std::string_view input(output);
    const auto expectedOutput = transliterate(input, "devanagari", "tamil", TranslitOptions::None);
    const auto expected = output + expectedOutput;
    if (!transliterate(input, "devanagari", "tamil", TranslitOptions::None, output))
      return 3;
    return output == expected ? 0 : 4;
  } else if (mode == "invalid-options") {
    auto* output = ::transliterate("a", "itrans", "devanagari", ~0ul, "##", "##");
    const bool rejected = output == nullptr;
    releaseBuffer(output);
    return rejected ? 0 : 3;
  } else if (mode == "null-text") {
    return ::transliterate(nullptr, "itrans", "devanagari", 0, "##", "##") == nullptr ? 0 : 3;
  } else if (mode == "null-from") {
    return ::transliterate("a", nullptr, "devanagari", 0, "##", "##") == nullptr ? 0 : 3;
  } else if (mode == "null-to") {
    return ::transliterate("a", "itrans", nullptr, 0, "##", "##") == nullptr ? 0 : 3;
  } else if (mode == "null-skip-start") {
    return ::transliterate("a", "itrans", "devanagari", 0, nullptr, "##") == nullptr ? 0 : 3;
  } else if (mode == "null-skip-end") {
    return ::transliterate("a", "itrans", "devanagari", 0, "##", nullptr) == nullptr ? 0 : 3;
  } else if (mode == "null-script-query") {
    return isScriptSupported(nullptr) == 0 ? 0 : 3;
  } else if (mode == "flutter-release") {
    auto* output = ::transliterate("namaste", "itrans", "devanagari", 0, "##", "##");
    if (!output)
      return 3;
    const std::string copied(output); // Mirrors Dart's toDartString() copy.
    releaseBuffer(output); // Mirrors the finally block in flutter/lib/inditrans.dart.
    return copied == "नमस्तॆ" ? 0 : 4;
  } else if (mode == "builder-replace-short") {
    Utf8StringBuilder builder;
    builder.reserve(32);
    builder += "abcdef";
    builder.replace(1, 4, "x");
    auto* output = builder.release();
    const bool valid = output && std::string_view(output) == "axf";
    std::free(output);
    return valid ? 0 : 3;
  } else if (mode == "builder-realloc-failure") {
    Utf8StringBuilder builder;
    failNextRealloc = true;
    builder += 'x';
    if (!builder.hasFailed())
      return 3;
    auto* output = builder.release();
    const bool rejected = output == nullptr;
    std::free(output);
    return rejected ? 0 : 4;
  } else if (mode == "builder-realloc-failure-after-growth") {
    Utf8StringBuilder builder;
    builder.reserve(8);
    builder += "abc";
    failNextRealloc = true;
    builder += "0123456789";
    if (!builder.hasFailed() || builder.size() != 3 || std::string_view(builder.back().view()) != "c")
      return 3;
    auto* output = builder.release();
    const bool rejected = output == nullptr;
    std::free(output);
    return rejected ? 0 : 4;
  } else if (mode == "translit-buffer-realloc-failure") {
    TranslitBuffer output(static_cast<char*>(std::malloc(4)));
    if (!output)
      return 3;
    std::memcpy(output.get(), "old", 4);
    failNextRealloc = true;
    const bool converted = transliterate("namaste", "itrans", "devanagari", TranslitOptions::None, output);
    return !converted && std::string_view(output.get()) == "old" ? 0 : 4;
  } else if (mode == "c-abi-realloc-failure") {
    failNextRealloc = true;
    auto* output = ::transliterate("namaste", "itrans", "devanagari", 0, "##", "##");
    const bool rejected = output == nullptr;
    releaseBuffer(output);
    return rejected ? 0 : 4;
  } else if (mode == "default-empty-view") {
    const auto result = transliterate(std::string_view {}, "itrans", "devanagari", TranslitOptions::None);
    return result.empty() ? 0 : 3;
  }
  return 2;
}
