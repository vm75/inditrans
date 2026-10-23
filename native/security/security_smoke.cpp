#include "inditrans.h"
#include "preservation_policy.h"
#include "static_scripts.h"
#include "utf.h"

#include <array>
#include <atomic>
#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

size_t outputCapacity(size_t bytes, const ScriptReaderMap& reader, const ScriptWriterMap& writer) noexcept;

int main() {
  const char fourByte[] = "\xF0\x9F\x98\x80";
  const auto character = UtfUtils::nextUtf8Char(fourByte);
  const char* fourByteCursor = fourByte;
  if (character.size() != 4 || UtfUtils::toChar32(fourByteCursor).len != 4)
    return 1;

  std::string filtered;
  inditrans::handlers::SpecialMarkerFilter::stripSpecialMarkers("xʼy", filtered);
  if (filtered != "xy")
    return 2;

  Utf8StringBuilder builder;
  builder.reserve(2);
  builder += "a";
  builder += "bcdef";
  builder.replace(1, 1, "XY");
  auto* owned = builder.release();
  const bool builderOk = std::string_view(owned) == "aXYcdef";
  std::free(owned);
  if (!builderOk)
    return 3;

  auto* empty = ::transliterate("", "itrans", "devanagari", 0, "##", "##");
  if (!empty || *empty != '\0') {
    releaseBuffer(empty);
    return 4;
  }
  releaseBuffer(empty);
  releaseBuffer(nullptr);
  if (!isScriptSupported("devanagari") || isScriptSupported("not-a-script"))
    return 5;

  const auto preserved = transliterate("क##plain <b>text</b>##ख", "devanagari", "telugu", TranslitOptions::None);
  if (preserved != "కplain <b>text</b>ఖ")
    return 6;

  TranslitBuffer reused;
  for (int call = 0; call < 1000; ++call) {
    if (!transliterate("namaste", "itrans", "devanagari", TranslitOptions::None, reused)
        || std::string_view(reused.get()) != "नमस्तॆ")
      return 9;
  }

  const auto* romanReader = getScriptReaderMap("itrans");
  const auto* indicWriter = getScriptWriterMap("devanagari");
  if (!romanReader || !indicWriter
      || outputCapacity((std::numeric_limits<size_t>::max() - 1) / 3, *romanReader, *indicWriter)
          != ((std::numeric_limits<size_t>::max() - 1) / 3) * 3 + 1
      || outputCapacity(std::numeric_limits<size_t>::max(), *romanReader, *indicWriter) != 0)
    return 10;

  std::string largeInput;
  largeInput.reserve(64 * 1024);
  for (size_t i = 0; i < 64 * 1024; ++i)
    largeInput += 'k';
  const auto largeOutput = transliterate(largeInput, "itrans", "devanagari", TranslitOptions::None);
  if (largeOutput.size() < largeInput.size() * 3 || largeOutput.size() > largeInput.size() * 8)
    return 7;

  std::atomic<bool> threadFailure { false };
  std::vector<std::thread> threads;
  for (int thread = 0; thread < 8; ++thread) {
    threads.emplace_back([&threadFailure] {
      for (int call = 0; call < 100; ++call) {
        if (transliterate("नमस्ते", "devanagari", "tamil", TranslitOptions::None) != "நமஸ்தே")
          threadFailure.store(true, std::memory_order_relaxed);
      }
    });
  }
  for (auto& thread : threads)
    thread.join();
  return threadFailure.load(std::memory_order_relaxed) ? 8 : 0;
}
