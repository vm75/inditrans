#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

#ifdef INDTRANSLIT_ALLOC_PROBE
#include <dlfcn.h>
#endif

#include "inditrans.h"

int main(int argc, char** argv) {
#ifdef INDTRANSLIT_ALLOC_PROBE
  using AllocProbe = void (*)();
  const auto resetAllocProbe = reinterpret_cast<AllocProbe>(dlsym(RTLD_DEFAULT, "alloc_probe_reset"));
  const auto reportAllocProbe = reinterpret_cast<AllocProbe>(dlsym(RTLD_DEFAULT, "alloc_probe_report"));
  if (resetAllocProbe == nullptr || reportAllocProbe == nullptr) return 2;
#endif

  const auto mode = argc > 1 ? std::string_view(argv[1]) : std::string_view("devanagari");
  const auto targetBytes = argc > 2 ? static_cast<size_t>(std::stoull(argv[2])) : size_t { 1 << 20 };
  const auto repetitions = argc > 3 ? static_cast<size_t>(std::stoull(argv[3])) : size_t { 1 };
  const auto warmups = argc > 4 ? static_cast<size_t>(std::stoull(argv[4])) : size_t { 0 };
  std::string_view seed;
  std::string_view from;
  std::string_view to;
  TranslitOptions options = TranslitOptions::None;
  if (mode == "roman") {
    seed = "śrī gurubhyo namaḥ, namaste bhāratam. ";
    from = "iso";
    to = "devanagari";
  } else if (mode == "expansion") {
    seed = "अॅॐऍ";
    from = "devanagari";
    to = "telugu";
  } else if (mode == "protected") {
    seed = "##a protected span with raw text नमस्ते and symbols 🙂 ##";
    from = "devanagari";
    to = "telugu";
  } else if (mode == "mixed-protected") {
    seed = "नमस्ते ##a long protected raw span with text नमस्ते and symbols 🙂## भारतम् । ";
    from = "devanagari";
    to = "telugu";
  } else if (mode == "iso-to-tamil") {
    seed = "śrī gurubhyo namaḥ, namaste bhāratam. ";
    from = "iso";
    to = "tamil";
  } else if (mode == "tamil-to-iso") {
    seed = "தங்கப் பதக்கம் சர்ச்சைக்குரிய செய்திகள் பாப்பா பம்பா அம்பா க்ருபை தந்து ரக்ஷியம்ப । ";
    from = "tamil";
    to = "iso";
  } else if (mode == "tamil-to-devanagari") {
    seed = "தங்கப் பதக்கம் சர்ச்சைக்குரிய செய்திகள் பாப்பா பம்பா அம்பா க்ருபை தந்து ரக்ஷியம்ப । ";
    from = "tamil";
    to = "devanagari";
  } else if (mode == "tamil-superscript-output") {
    seed = "श्री॒ गुरुभ्यो नमः । नमस्ते भारतम् । ";
    from = "devanagari";
    to = "tamil";
    options = TranslitOptions::TamilSuperscripted;
  } else {
    seed = "श्री॒ गुरुभ्यो नमः । नमस्ते भारतम् । ";
    from = "devanagari";
    to = "telugu";
  }
  std::string input;
  input.reserve(targetBytes);
  while (input.size() < targetBytes) input.append(seed);

  for (size_t i = 0; i < warmups; ++i) {
    const auto output = transliterate(input, from, to, options);
    (void)output;
  }

  uint64_t checksum = 0;
  size_t outputBytes = 0;
#ifdef INDTRANSLIT_ALLOC_PROBE
  resetAllocProbe();
#endif
  for (size_t i = 0; i < repetitions; ++i) {
    const auto output = transliterate(input, from, to, options);
    outputBytes = output.size();
    for (const auto byte : output) checksum += static_cast<uint8_t>(byte);
  }
#ifdef INDTRANSLIT_ALLOC_PROBE
  reportAllocProbe();
#endif
  std::cout << mode << ',' << input.size() << ',' << outputBytes << ',' << repetitions << ',' << checksum << '\n';
}
