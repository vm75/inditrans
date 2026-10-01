#include <iostream>
#include <cstring>
#include <string_view>
#include "exports.h"

int main() {
  std::cout << "Starting Windows DLL smoke test..." << std::endl;

  // 1. Test isScriptSupported
  if (!isScriptSupported("devanagari")) {
    std::cerr << "FAIL: devanagari script not reported as supported" << std::endl;
    return 1;
  }
  std::cout << "PASS: isScriptSupported('devanagari')" << std::endl;

  // 2. Test transliterate: "namaste" from hk (Harvard-Kyoto) to devanagari -> "नमस्ते"
  const char* text = "namaste";
  char* result = transliterate(text, "hk", "devanagari", 0, "##", "##");
  if (!result) {
    std::cerr << "FAIL: transliterate returned null" << std::endl;
    return 2;
  }

  constexpr std::string_view expected = "नमस्ते";
  if (expected != result) {
    std::cerr << "FAIL: transliterate output mismatch! Expected '" << expected
              << "', got '" << result << "'" << std::endl;
    releaseBuffer(result);
    return 3;
  }
  std::cout << "PASS: transliterated '" << text << "' -> '" << result << "'" << std::endl;

  // 3. Test releaseBuffer
  releaseBuffer(result);
  std::cout << "PASS: releaseBuffer called successfully" << std::endl;

  std::cout << "All Windows smoke test checks passed!" << std::endl;
  return 0;
}
