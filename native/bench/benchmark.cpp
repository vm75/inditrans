#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

#include "inditrans.h"

using Clock = std::chrono::steady_clock;

struct Case {
  std::string_view from;
  std::string_view to;
  std::string_view label;
  std::string_view seed;
};

constexpr std::string_view devanagariSeed = "श्री॒ गुरुभ्यो नमः । नमस्ते भारतम् । ";
constexpr std::string_view latinSeed = "śrī gurubhyo namaḥ, namaste bhāratam. ";
constexpr std::string_view expansionSeed = "अॅॐऍ";
constexpr std::string_view protectedSeed = "##a protected span with raw text नमस्ते and symbols 🙂 ##";
constexpr std::string_view mixedProtectedSeed = "नमस्ते ##a long protected raw span with text नमस्ते and symbols 🙂## भारतम् । ";

constexpr std::array cases {
  Case { "devanagari", "telugu", "indic-to-indic", devanagariSeed },
  Case { "devanagari", "tamil", "tamil-output", devanagariSeed },
  Case { "iso", "devanagari", "latin-input", latinSeed },
  Case { "indic", "iso", "virtual-indic-to-latin", devanagariSeed },
  Case { "indic", "devanagari", "virtual-indic-to-indic", devanagariSeed },
  Case { "indic", "tamil", "virtual-indic-to-tamil", devanagariSeed },
  Case { "devanagari", "iso", "latin-output", devanagariSeed },
  Case { "devanagari", "telugu", "expansion-heavy", expansionSeed },
  Case { "devanagari", "telugu", "protected-spans", protectedSeed },
  Case { "devanagari", "telugu", "mixed-protected-spans", mixedProtectedSeed },
};

std::string makeInput(size_t targetBytes, std::string_view seed) {
  std::string input;
  input.reserve(targetBytes);
  while (input.size() < targetBytes) input.append(seed);
  return input;
}

void measure(size_t inputBytes, const Case& testCase) {
  const auto input = makeInput(inputBytes, testCase.seed);
  constexpr size_t samples = 31;
  const size_t repetitions = inputBytes < 128 ? 2000 : inputBytes < 8192 ? 200 : 8;
  std::array<double, samples> nsPerCall {};
  uint64_t outputSizeSink = 0;

  for (size_t sample = 0; sample < samples; ++sample) {
    size_t lastOutputSize = 0;
    const auto start = Clock::now();
    for (size_t i = 0; i < repetitions; ++i) {
      const auto output = transliterate(input, testCase.from, testCase.to, TranslitOptions::None);
      lastOutputSize = output.size();
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
    outputSizeSink += lastOutputSize;
    nsPerCall[sample] = static_cast<double>(elapsed) / repetitions;
  }

  const auto checkOutput = transliterate(input, testCase.from, testCase.to, TranslitOptions::None);
  uint64_t outputHash = 14695981039346656037ull;
  for (const unsigned char byte : checkOutput) {
    outputHash = (outputHash ^ byte) * 1099511628211ull;
  }

  std::ranges::sort(nsPerCall);
  constexpr size_t p95Index = (samples * 95 + 99) / 100 - 1;
  std::cout << testCase.label << ',' << input.size() << ',' << nsPerCall[samples / 2] << ','
            << nsPerCall[p95Index] << ',' << outputSizeSink << ',' << outputHash << '\n';
}

int main() {
  std::cout << "case,input_bytes,median_ns,sample_p95_ns,output_size_sink,output_fnv1a64\n";
  for (const auto& testCase : cases) {
    for (const auto inputBytes : { size_t { 32 }, size_t { 4096 }, size_t { 1 << 20 } }) {
      measure(inputBytes, testCase);
    }
  }
}
