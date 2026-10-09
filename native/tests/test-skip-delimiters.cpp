#include "inditrans.h"
#include "ut.hpp"

#include <string>
#include <string_view>

using namespace boost::ut;

suite<"Protected-span delimiter safety"> protectedSpanDelimiterTests = [] {
  "empty start disables protected-span matching"_test = [] {
    constexpr std::string_view input = "क # ख ## ग";
    constexpr std::string_view expected = "క # ఖ ## గ";

    expect(transliterate(input, "devanagari", "telugu", TranslitOptions::None, "", "##") == expected);
    expect(transliterate(input, "devanagari", "telugu", TranslitOptions::None, "", "") == expected);

    std::string output;
    expect(transliterate(input, "devanagari", "telugu", TranslitOptions::None, output, "", "##"));
    expect(output == expected);
  };

  "empty closing delimiter without an opening span"_test = [] {
    expect(transliterate("क # ख", "devanagari", "telugu", TranslitOptions::None, "##", "")
        == "క # ఖ");
  };

  "opening delimiter longer than input"_test = [] {
    const std::string veryLongStart(8192, '#');
    expect(transliterate("क # ख", "devanagari", "telugu", TranslitOptions::None, veryLongStart, "##")
        == "క # ఖ");
  };
};
