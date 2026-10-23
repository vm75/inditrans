#include "exports.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace {

constexpr std::array scripts {
    "assamese", "bengali", "burmese", "devanagari", "gujarati", "gurmukhi", "kannada", "khmer",
    "malayalam", "oriya", "sinhala", "tamil", "tamil-extended", "telugu", "tibetan", "thai",
    "iast", "ipa", "iso", "itrans", "titus", "harvard-kyoto", "velthuis", "slp1", "wx",
    "readablelatin", "indic", "invalid-script",
};

unsigned long optionValue(const uint8_t* data, size_t size) {
  unsigned long value = 0;
  const auto count = size < sizeof(value) ? size : sizeof(value);
  if (count != 0)
    std::memcpy(&value, data, count);
  return value;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size > 64 * 1024)
    return 0;

  const uint8_t selector = size ? data[0] : uint8_t { 0 };
  const auto from = scripts[selector % scripts.size()];
  const auto to = scripts[(selector + 1) % scripts.size()];

  const auto* textBytes = size > 1 ? reinterpret_cast<const char*>(data + 1) : "";
  std::string text(textBytes, size > 1 ? size - 1 : 0);

  const auto* delimiterBytes = size > 2 ? reinterpret_cast<const char*>(data + 2) : "##";
  const auto delimiterSize = size > 2 ? std::min<size_t>(size - 2, 64) : 2;
  std::string delimiter(delimiterBytes, delimiterSize);
  const auto options = optionValue(size > 1 ? data + 1 : nullptr, size > 1 ? size - 1 : 0);

  const char* textArg = (selector & 0x01) ? nullptr : text.c_str();
  const char* fromArg = (selector & 0x02) ? nullptr : from;
  const char* toArg = (selector & 0x04) ? nullptr : to;
  const char* skipStartArg = (selector & 0x08) ? nullptr : delimiter.c_str();
  const char* skipEndArg = (selector & 0x10) ? nullptr : delimiter.c_str();
  char* result = ::transliterate(textArg, fromArg, toArg, options, skipStartArg, skipEndArg);
  releaseBuffer(result);

  const char* query = (selector & 0x20) ? nullptr : from;
  (void)isScriptSupported(query);
  (void)isScriptSupported(nullptr);
  releaseBuffer(nullptr);
  return 0;
}
