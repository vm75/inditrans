#include "inditrans.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace {

constexpr std::array scripts {
    "assamese", "bengali", "burmese", "devanagari", "gujarati", "gurmukhi", "kannada", "khmer",
    "malayalam", "oriya", "sinhala", "tamil", "tamil-extended", "telugu", "tibetan", "thai",
    "iast", "ipa", "iso", "itrans", "titus", "harvard-kyoto", "velthuis", "slp1", "wx",
    "readablelatin", "indic",
};

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size > 64 * 1024)
    return 0;

  const auto* bytes = size ? reinterpret_cast<const char*>(data) : "";
  const std::string_view all(bytes, size);
  const auto selector = size ? data[0] : uint8_t { 0 };
  auto from = std::string_view(scripts[selector % scripts.size()]);
  auto to = std::string_view(scripts[(selector + (size > 1 ? data[1] : 1)) % scripts.size()]);
  if (from == to)
    to = std::string_view(scripts[(selector + 1) % scripts.size()]);

  const auto quarter = size / 4;
  const auto skipStart = quarter ? all.substr(quarter, quarter) : std::string_view("##");
  const auto skipEnd = quarter ? all.substr(quarter * 2, quarter) : std::string_view("##");
  const auto input = size > 2 ? all.substr(2) : std::string_view("<x>##protected##");
  const auto optionByte = size > 2 ? data[2] : uint8_t { 0 };
  const auto options = static_cast<TranslitOptions>(optionByte & 0x7f);

  TranslitBuffer output;
  (void)transliterate(input, from, to, options, output, skipStart, skipEnd);
  return 0;
}
