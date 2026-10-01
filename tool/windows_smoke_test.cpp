#include <iostream>
#include <cstring>
#include "exports.h"

int main() {
  std::cout << "Starting Windows DLL smoke test..." << std::endl;

  // 1. Test isScriptSupported
  if (!isScriptSupported("devanagari")) {
    std::cerr << "FAIL: devanagari script not reported as supported" << std::endl;
    return 1;
  }
  std::cout << "PASS: isScriptSupported('devanagari')" << std::endl;

  // 2. Test transliterate
  const char* text = "namaste";
  char* result = transliterate(text, "itrans", "devanagari", 0, "##", "##");
  if (!result) {
    std::cerr << "FAIL: transliterate returned null" << std::endl;
    return 2;
  }
  std::cout << "PASS: transliterated '" << text << "' -> '" << result << "'" << std::endl;

  if (std::strlen(result) == 0) {
    std::cerr << "FAIL: transliterate result is empty" << std::endl;
    releaseBuffer(result);
    return 3;
  }

  // 3. Test releaseBuffer
  releaseBuffer(result);
  std::cout << "PASS: releaseBuffer called successfully" << std::endl;

  std::cout << "All Windows smoke test checks passed!" << std::endl;
  return 0;
}
