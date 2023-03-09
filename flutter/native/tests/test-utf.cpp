#include <iostream>
#include <string_view>
#include <utf.h>
#include <vector>

void testAssert(std::string_view test, bool result) noexcept {
  std::cout << "  Test: " << test << (result ? " PASSED" : "FAILED") << std::endl;
}

void testUtf() noexcept {
  std::cout << std::endl << "Utf tests" << std::endl;

  constexpr Utf8Char utf8Char = "न"_uc8;
  constexpr Utf32Char utf32Char = U'न'_uc32;
  testAssert("utf8Char.char32", utf8Char.char32().ch == utf32Char);
  testAssert("utf8Char.string", utf8Char.string() == "न");
  testAssert("utf8Char.view", utf8Char.view() == "न");
  testAssert("utf32Char.string", utf32Char.string() == "न");

  constexpr auto in = "श्री॒ गु॒रु॒भ्यो नमः॒ । ह॒रिः॒ ओ३म् ॥"_us8;
  std::string out;
  size_t offset {};
  while (!in.emptyAt(offset)) {
    out += (in(offset)).string();
  }

  testAssert("utf8String iterate", in == std::string_view(out.c_str(), out.length()));

  testAssert("Utf8Char.isZero", Utf8Char("").isZero());
}

int main() {
  testUtf();
  return 0;
}
