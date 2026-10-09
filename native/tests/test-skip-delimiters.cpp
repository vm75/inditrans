#include "inditrans.h"
#include "ut.hpp"

#include <string>
#include <string_view>

using namespace boost::ut;

suite<"Protected-span delimiter safety"> protectedSpanDelimiterTests = [] {
  "empty delimiters use the default protected-span markers"_test = [] {
    constexpr std::string_view input = "क # ख ## ग ## घ";
    constexpr std::string_view expected = "క # ఖ  ग  ఘ";

    expect(transliterate(input, "devanagari", "telugu", TranslitOptions::None, "", "##") == expected);
    expect(transliterate(input, "devanagari", "telugu", TranslitOptions::None, "", "") == expected);
    expect(transliterate(input, "devanagari", "telugu", TranslitOptions::None, "##", "") == expected);

    std::string output;
    expect(transliterate(input, "devanagari", "telugu", TranslitOptions::None, output, "", "##"));
    expect(output == expected);
  };

  "empty closing delimiter defaults to ##"_test = [] {
    expect(transliterate(std::string_view("क##ख##ग"), "devanagari", "telugu", TranslitOptions::None, "##", "")
        == "కखగ");
  };

  "opening delimiter longer than input"_test = [] {
    const std::string veryLongStart(8192, '#');
    expect(transliterate(std::string_view("क # ख"), "devanagari", "telugu", TranslitOptions::None, veryLongStart, "##")
        == "క # ఖ");
  };
};
