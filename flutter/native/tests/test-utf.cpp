#include "ut.hpp"

#include <string_view>
#include <utf.h>
#include <vector>

using namespace boost::ut;

suite<"UTF-8 Tests"> utf8Tests = [] {
  "Testing UTF-8"_test = [] {
    constexpr Utf8Char utf8Char = "न"_uc8;
    constexpr Utf32Char utf32Char = U'न'_uc32;
    expect(utf8Char.char32().ch == utf32Char);
    expect(utf8Char.string() == "न");
    expect(utf8Char.view() == "न");
    expect(utf32Char.string() == "न");

    constexpr auto in = "श्री॒ गु॒रु॒भ्यो नमः॒ । ह॒रिः॒ ओ३म् ॥"_us8;
    std::string out;
    size_t offset {};
    while (!in.emptyAt(offset)) {
      out += (in(offset)).string();
    }

    expect(in == std::string_view(out.c_str(), out.length()));

    expect(Utf8Char("").isZero());
  };
};
