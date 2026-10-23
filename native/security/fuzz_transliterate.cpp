#include "inditrans.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
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

  const auto selector = size ? data[0] : uint8_t { 0 };
  const auto from = std::string_view(scripts[selector % scripts.size()]);
  auto to = std::string_view(scripts[(size > 1 ? data[1] : selector) % scripts.size()]);
  if (from == to)
    to = std::string_view(scripts[(selector + 1) % scripts.size()]);

  const auto rawOptions = static_cast<unsigned long>((size > 2 ? data[2] : selector) & 0x7f);
  const auto options = static_cast<TranslitOptions>(rawOptions);
  const auto* inputBytes = size > 3 ? reinterpret_cast<const char*>(data + 3) : "";
  const auto inputSize = size > 3 ? size - 3 : 0;
  const std::string_view input(inputBytes, inputSize);

  TranslitBuffer buffer;
  (void)transliterate(input, from, to, options, buffer);

  std::string appended = "prefix";
  (void)transliterate(input, from, to, options, appended);

  // Exercise the public C ABI with valid, owned, NUL-terminated argument strings.
  std::string nulTerminated(input);
  auto* cOutput = ::transliterate(nulTerminated.c_str(), from.data(), to.data(), rawOptions, "##", "##");
  releaseBuffer(cOutput);
  return 0;
}
