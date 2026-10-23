#include "preservation_policy.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size > 64 * 1024)
    return 0;
  const auto* bytes = size ? reinterpret_cast<const char*>(data) : "";
  std::string output;
  inditrans::handlers::SpecialMarkerFilter::stripSpecialMarkers(std::string_view(bytes, size), output);
  return 0;
}
