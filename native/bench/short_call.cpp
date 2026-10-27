#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

#include "inditrans.h"

using Clock = std::chrono::steady_clock;

struct Case {
  std::string_view from;
  std::string_view to;
  std::string_view label;
  std::string_view input;
  TranslitOptions options = TranslitOptions::None;
};

constexpr std::string_view devanagariSeed = "श्री॒ गुरुभ्यो नमः । नमस्ते भारतम् । ";
constexpr std::string_view romanSeed = "śrī gurubhyo namaḥ, namaste bhāratam. ";
constexpr std::string_view expansionSeed = "अॅॐऍ";
constexpr std::string_view protectedSeed = "##a protected span with raw text नमस्ते and symbols 🙂 ##";
constexpr std::string_view mixedProtectedSeed = "नमस्ते ##a long protected raw span with text नमस्ते and symbols 🙂## भारतम् । ";
constexpr std::string_view tamilSeed = "தங்கப் பதக்கம் சர்ச்சைக்குரிய செய்திகள் பாப்பா பம்பா அம்பா க்ருபை தந்து ரக்ஷியம்ப । ";
constexpr std::string_view tamilSuperscriptSeed = "ஶ்ரீ கு³ருப்⁴யோ நம꞉ । ";
constexpr std::string_view tamilPrefixSeed = "அதுபோல ";
constexpr std::string_view tamilCompoundVowelsSeed = "கொ கோ கௌ ";
constexpr std::string_view gurmukhiAdhakSeed = "ਹੱਦ ਬੱਚਿਓ ਕੱਦ । ";

// Explicit Indic sources (including Grantha) and ISO; no virtual Indic reader.
constexpr std::array cases {
  Case { "devanagari", "telugu", "devanagari-to-telugu", devanagariSeed },
  Case { "devanagari", "tamil", "tamil-output", devanagariSeed },
  Case { "iso", "devanagari", "iso-to-devanagari", romanSeed },
  Case { "devanagari", "iso", "devanagari-to-iso", devanagariSeed },
  Case { "devanagari", "telugu", "expansion-heavy", expansionSeed },
  Case { "devanagari", "telugu", "protected-spans", protectedSeed },
  Case { "devanagari", "telugu", "mixed-protected-spans", mixedProtectedSeed },
  Case { "tamil", "devanagari", "tamil-to-devanagari", tamilSeed },
  Case { "tamil", "iso", "tamil-to-iso", tamilSeed },
  Case { "tamil", "devanagari", "tamil-superscript-input", tamilSuperscriptSeed, TranslitOptions::TamilSuperscripted },
  Case { "devanagari", "tamil", "tamil-superscript-output", devanagariSeed, TranslitOptions::TamilSuperscripted },
  Case { "devanagari", "tamil", "tamil-traditional-output", devanagariSeed, TranslitOptions::TamilTraditional },
  Case { "tamil", "iso", "tamil-prefix", tamilPrefixSeed },
  Case { "tamil", "iso", "tamil-compound-vowels", tamilCompoundVowelsSeed },
  Case { "iso", "tamil", "iso-to-tamil", romanSeed },
  Case { "gurmukhi", "devanagari", "gurmukhi-adhak", gurmukhiAdhakSeed },
  Case { "assamese", "devanagari", "assamese-to-devanagari", "নম॒স্তে॑ ভা॒ৰতম্ । " },
  Case { "bengali", "devanagari", "bengali-to-devanagari", "নম॒স্তে॑ ভা॒রতম্ । " },
  Case { "gujarati", "devanagari", "gujarati-to-devanagari", "નમ॒સ્તે॑ ભા॒રતમ્ । " },
  Case { "kannada", "devanagari", "kannada-to-devanagari", "ನಮ॒ಸ್ತೇ॑ ಭಾ॒ರತಮ್ । " },
  Case { "malayalam", "devanagari", "malayalam-to-devanagari", "നമ॒സ്തേ॑ ഭാ॒രതമ് । " },
  Case { "oriya", "devanagari", "oriya-to-devanagari", "ନମ॒ସ୍ତେ॑ ଭା॒ରତମ୍ । " },
  Case { "telugu", "devanagari", "telugu-to-devanagari", "నమ॒స్తే॑ భా॒రతమ్ । " },
  Case { "grantha", "devanagari", "grantha-to-devanagari", "𑌨𑌮॒𑌸𑍍𑌤𑍇॑ 𑌭𑌾॒𑌰𑌤𑌮𑍍 । " },
};

void measure(const Case& testCase) {
  constexpr size_t warmups = 16;
  constexpr size_t samples = 10000;
  std::vector<uint64_t> latencyNs(samples);
  uint64_t outputSizeSink = 0;

  for (size_t i = 0; i < warmups; ++i) {
    const auto output = transliterate(testCase.input, testCase.from, testCase.to, testCase.options);
    outputSizeSink += output.size();
  }

  for (size_t i = 0; i < samples; ++i) {
    size_t outputSize = 0;
    const auto start = Clock::now();
    {
      const auto output = transliterate(testCase.input, testCase.from, testCase.to, testCase.options);
      outputSize = output.size();
    }
    const auto end = Clock::now();
    latencyNs[i] = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
    outputSizeSink += outputSize;
  }

  const auto checkOutput = transliterate(testCase.input, testCase.from, testCase.to, testCase.options);
  uint64_t outputHash = 14695981039346656037ull;
  for (const unsigned char byte : checkOutput) {
    outputHash = (outputHash ^ byte) * 1099511628211ull;
  }

  std::ranges::sort(latencyNs);
  constexpr size_t p95Index = (samples * 95 + 99) / 100 - 1;
  std::cout << testCase.label << ',' << testCase.input.size() << ',' << latencyNs[samples / 2] << ','
            << latencyNs[p95Index] << ',' << outputSizeSink << ',' << outputHash << '\n';
}

int main() {
  std::cout << "case,input_bytes,p50_ns,p95_ns,output_size_sink,output_fnv1a64\n";
  for (const auto& testCase : cases) measure(testCase);
}
