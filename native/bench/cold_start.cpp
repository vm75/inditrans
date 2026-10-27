#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string_view>

#include "inditrans.h"

using Clock = std::chrono::steady_clock;

struct Case {
  std::string_view name;
  std::string_view from;
  std::string_view to;
  std::string_view input;
};

constexpr std::array cases {
  Case { "cold-devanagari-to-telugu", "devanagari", "telugu", "श्री॒ गुरुभ्यो नमः । नमस्ते भारतम् । " },
  Case { "cold-iso-to-devanagari", "iso", "devanagari", "śrī gurubhyo namaḥ, namaste bhāratam. " },
  Case { "cold-devanagari-to-tamil", "devanagari", "tamil", "श्री॒ गुरुभ्यो नमः । नमस्ते भारतम् । " },
  Case { "cold-iso-to-tamil", "iso", "tamil", "śrī gurubhyo namaḥ, namaste bhāratam. " },
  Case { "cold-tamil-to-iso", "tamil", "iso", "தங்கப் பதக்கம் சர்ச்சைக்குரிய செய்திகள் பாப்பா பம்பா அம்பா க்ருபை தந்து ரக்ஷியம்ப । " },
  Case { "cold-tamil-to-devanagari", "tamil", "devanagari", "தங்கப் பதக்கம் சர்ச்சைக்குரிய செய்திகள் பாப்பா பம்பா அம்பா க்ருபை தந்து ரக்ஷியம்ப । " },
};

const Case* findCase(std::string_view name) noexcept {
  for (const auto& testCase : cases) {
    if (testCase.name == name) return &testCase;
  }
  return nullptr;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: inditrans_cold_bench <case>\n";
    return 2;
  }

  const auto* testCase = findCase(argv[1]);
  if (testCase == nullptr) {
    std::cerr << "unknown cold-start case: " << argv[1] << '\n';
    return 2;
  }

  const auto start = Clock::now();
  const auto output = transliterate(testCase->input, testCase->from, testCase->to, TranslitOptions::None);
  const auto end = Clock::now();

  if (output.empty()) {
    std::cerr << "cold-start transliteration returned empty output\n";
    return 1;
  }

  uint64_t outputHash = 14695981039346656037ull;
  for (const unsigned char byte : output) {
    outputHash = (outputHash ^ byte) * 1099511628211ull;
  }

  const auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  std::cout << testCase->name << ',' << testCase->input.size() << ',' << elapsedNs << ',' << output.size() << ','
            << outputHash << '\n';
}
