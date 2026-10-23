#include "preservation_policy.h"
#include "utf.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size > 64 * 1024)
    return 0;

  const auto* bytes = size ? reinterpret_cast<const char*>(data) : "";
  const std::string_view input(bytes, size);
  size_t offset = 0;
  while (offset < input.size()) {
    const auto length = UtfUtils::nextCharLen(input.data() + offset);
    if (length > input.size() - offset)
      break; // The raw decoder has no bound parameter; never call it on a truncated sequence here.

    const char* cursor = input.data() + offset;
    const auto decoded = UtfUtils::toChar32(cursor);
    if (static_cast<size_t>(cursor - input.data()) != offset + decoded.len)
      __builtin_trap();
    offset += decoded.len;
  }

  // Exercise marker handling on a prefix whose inferred UTF-8 sequence lengths fit the view.
  std::string_view complete(input.data(), offset);
  std::string output;
  inditrans::handlers::SpecialMarkerFilter::stripSpecialMarkers(complete, output);
  return 0;
}
