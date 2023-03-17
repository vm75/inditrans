#include "doctest.h"

#include <string_view>
#include <utf.h>
#include <vector>

TEST_CASE("Testing UTF-8") {
  constexpr Utf8Char utf8Char = "न"_uc8;
  constexpr Utf32Char utf32Char = U'न'_uc32;
  CHECK(utf8Char.char32().ch == utf32Char);
  CHECK(utf8Char.string() == "न");
  CHECK(utf8Char.view() == "न");
  CHECK(utf32Char.string() == "न");

  constexpr auto in = "श्री॒ गु॒रु॒भ्यो नमः॒ । ह॒रिः॒ ओ३म् ॥"_us8;
  std::string out;
  size_t offset {};
  while (!in.emptyAt(offset)) {
    out += (in(offset)).string();
  }

  CHECK(in == std::string_view(out.c_str(), out.length()));

  CHECK(Utf8Char("").isZero());
}
