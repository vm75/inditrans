#include "tamil_handler.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size > 4096)
    return 0;

  const auto* bytes = size ? reinterpret_cast<const char*>(data) : "";
  for (size_t offset = 0; offset <= size; ++offset) {
    const auto available = size - offset;
    const auto maxLength = std::min<size_t>(available, 4);
    for (size_t length = 0; length <= maxLength; ++length) {
      const std::string_view candidate(bytes + offset, length);
      const auto marker = inditrans::handlers::matchTamilMarker(candidate);
      if (marker && marker->byteLength > candidate.size())
        __builtin_trap();
      (void)inditrans::handlers::isTamilMarkerSpan(candidate);
    }
  }
  return 0;
}
