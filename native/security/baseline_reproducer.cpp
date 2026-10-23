// Build against the vulnerable audit base to reproduce the original findings.
#include "inditrans.h"
#include "preservation_policy.h"
#include "utf.h"

#include <cstdlib>
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
  } else if (mode == "output-alias") {
    std::string output;
    for (int i = 0; i < 1024; ++i)
      output += "क";
    const std::string_view input(output);
    (void)transliterate(input, "devanagari", "tamil", TranslitOptions::None, output);
  } else if (mode == "invalid-options") {
    (void)::transliterate("a", "itrans", "devanagari", ~0ul, "##", "##");
  } else if (mode == "null-text") {
    (void)::transliterate(nullptr, "itrans", "devanagari", 0, "##", "##");
  } else if (mode == "null-from") {
    (void)::transliterate("a", nullptr, "devanagari", 0, "##", "##");
  } else if (mode == "null-to") {
    (void)::transliterate("a", "itrans", nullptr, 0, "##", "##");
  } else if (mode == "null-skip-start") {
    (void)::transliterate("a", "itrans", "devanagari", 0, nullptr, "##");
  } else if (mode == "null-skip-end") {
    (void)::transliterate("a", "itrans", "devanagari", 0, "##", nullptr);
  } else if (mode == "null-script-query") {
    (void)::isScriptSupported(nullptr);
  } else if (mode == "flutter-missing-release") {
    auto* output = ::transliterate("namaste", "itrans", "devanagari", 0, "##", "##");
    if (output) {
      const std::string copied(output);
      if (copied.empty())
        return 3;
      // Intentionally models the pre-fix Dart wrapper, which omitted releaseBuffer.
    }
  } else if (mode == "builder-replace-short") {
    Utf8StringBuilder builder;
    builder.reserve(32);
    builder += "abcdef";
    builder.replace(1, 4, "x");
  } else if (mode == "builder-realloc-failure") {
    Utf8StringBuilder builder;
    failNextRealloc = true;
    builder += 'x';
  } else if (mode == "default-empty-view") {
    (void)transliterate(std::string_view {}, "itrans", "devanagari", TranslitOptions::None);
  } else {
    return 2;
  }
  return 0;
}
