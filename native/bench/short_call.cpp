#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "inditrans.h"

using Clock = std::chrono::steady_clock;

struct Case {
  std::string_view from;
  std::string_view to;
  std::string_view label;
  std::string_view input;
};

constexpr std::array cases {
  Case { "devanagari", "telugu", "indic-to-indic", "श्री॒ गुरुभ्यो नमः । नमस्ते भारतम् । " },
  Case { "devanagari", "tamil", "tamil-output", "श्री॒ गुरुभ्यो नमः । नमस्ते भारतम् । " },
  Case { "iso", "devanagari", "latin-input", "śrī gurubhyo namaḥ, namaste bhāratam. " },
  Case { "indic", "iso", "virtual-indic-input", "श्री॒ गुरुभ्यो नमः । नमस्ते भारतम् । " },
  Case { "vedic", "devanagari", "vedic-input", "श्री॒ गुरुभ्यो नमः । नमस्ते भारतम् । " },
  Case { "devanagari", "iso", "latin-output", "श्री॒ गुरुभ्यो नमः । नमस्ते भारतम् । " },
  Case { "devanagari", "telugu", "expansion-heavy", "अॅॐऍ" },
  Case { "devanagari", "telugu", "protected-spans", "##a protected span with raw text नमस्ते and symbols 🙂 ##" },
  Case { "devanagari", "telugu", "mixed-protected-spans", "नमस्ते ##a long protected raw span with text नमस्ते and symbols 🙂## भारतम् । " },
};

void measure(const Case& testCase) {
  constexpr size_t warmups = 16;
  constexpr size_t samples = 10000;
  std::vector<uint64_t> latencyNs(samples);
  uint64_t checksum = 0;

  for (size_t i = 0; i < warmups; ++i) {
    const auto output = transliterate(testCase.input, testCase.from, testCase.to, TranslitOptions::None);
    checksum += output.empty() ? 0 : static_cast<uint8_t>(output[i % output.size()]);
  }

  for (size_t i = 0; i < samples; ++i) {
    const auto start = Clock::now();
    const auto output = transliterate(testCase.input, testCase.from, testCase.to, TranslitOptions::None);
    const auto end = Clock::now();
    latencyNs[i] = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
    checksum += output.empty() ? 0 : static_cast<uint8_t>(output[i % output.size()]);
  }

  const auto checkOutput = transliterate(testCase.input, testCase.from, testCase.to, TranslitOptions::None);
  uint64_t outputHash = 14695981039346656037ull;
  for (const unsigned char byte : checkOutput) {
    outputHash = (outputHash ^ byte) * 1099511628211ull;
  }

  std::ranges::sort(latencyNs);
  constexpr size_t p95Index = (samples * 95 + 99) / 100 - 1;
  std::cout << testCase.label << ',' << testCase.input.size() << ',' << latencyNs[samples / 2] << ','
            << latencyNs[p95Index] << ',' << checksum << ',' << outputHash << '\n';
}

int main() {
  std::cout << "case,input_bytes,p50_ns,p95_ns,sample_checksum,output_fnv1a64\n";
  for (const auto& testCase : cases) measure(testCase);
}
