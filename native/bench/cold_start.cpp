#include <chrono>
#include <cstdint>
#include <iostream>
#include <string_view>

#include "inditrans.h"

using Clock = std::chrono::steady_clock;

int main() {
  constexpr std::string_view input = "श्री॒ गुरुभ्यो नमः । नमस्ते भारतम् । ";
  const auto start = Clock::now();
  const auto output = transliterate(input, "devanagari", "telugu", TranslitOptions::None);
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
  std::cout << elapsedNs << ',' << output.size() << ',' << outputHash << '\n';
}
