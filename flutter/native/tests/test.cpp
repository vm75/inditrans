#include <chrono>
#include <fstream>
#include <inditrans.h>
#include <iomanip>
#include <iostream>
#include <json.h>
#include <utilities.h>
#include <vector>

using InditransLogger = void(const std::string&);
extern InditransLogger* inditransLogger;

void testAssert(std::string_view test, bool result) noexcept {
  std::cout << "  Test: " << test << (result ? " PASSED" : "FAILED") << std::endl;
}

template <class TimeT = std::chrono::milliseconds, class ClockT = std::chrono::steady_clock> struct measure {
  template <class F, class... Args> static auto duration(F&& func, Args&&... args) {
    auto start = ClockT::now();
    std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
    return std::chrono::duration_cast<TimeT>(ClockT::now() - start);
  }
};

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

void testJson() noexcept {
  auto val = JsonReader::parseJson(std::string_view(R"(
    {
      "types" : ["om=\u0950", 123, -123, 1234.5678, true, false, null],
      "nested": [ "val1", "val2", {
        "x" : "y"
      } ]
    }
  )"));
  std::cout << std::endl;
}

bool testTranslit(const std::string_view& description, const std::string_view& from, const std::string_view& to,
    const std::string_view& text, const std::string_view& expected, const std::string_view& optStr) noexcept {
  static size_t testNum { 1 };
  auto res = transliterate(text, from, to, getTranslitOptions(optStr));
  bool result = res == expected;
  std::cout << "  Test #" << std::setw(3) << testNum++ << ": " << description << ": (" << from << " -> " << to
            << ", options=" << optStr << "): " << (result ? " PASSED" : "FAILED") << std::endl;
  if (!result) {
    std::cout << "    Input   : " << text << std::endl;
    std::cout << "    Expected: " << expected << std::endl;
    std::cout << "    Observed: " << res << std::endl;
  }

  return result;
}

void testAllTranslit() noexcept {
  std::ifstream testsFile("assets/test-cases.json", std::ios::binary);
  if (testsFile.is_open()) {

    std::vector<char> jsonBuffer(std::istreambuf_iterator<char>(testsFile), {});
    std::string_view bufferView { jsonBuffer.data(), jsonBuffer.size() };
    auto inputsHolder = JsonReader::parseJson(bufferView);
    if (inputsHolder == std::nullopt) {
      return;
    }

    auto inputs = std::get<JsonArray>(*inputsHolder);

    std::cout << std::endl << "Inditrans tests" << std::endl;
    size_t failedCount {};
    for (const auto& input : inputs) {
      auto inputObj = std::get<JsonOject>(input);

      auto description = inputObj.get<std::string>("description");
      auto text = inputObj.get<std::string>("text");
      auto source = inputObj.get<std::string>("script");

      auto targets = inputObj.get<JsonArray>("targets");
      for (auto& entry : *targets) {
        auto targetObj = std::get<JsonOject>(entry);
        auto target = targetObj.get<std::string>("script");
        auto expected = targetObj.get<std::string>("text");

        std::string options {};
        if (targetObj.get<std::string>("options") != std::nullopt) {
          options = *targetObj.get<std::string>("options");
        }

        failedCount += !testTranslit(*description, *source, *target, *text, *expected, options);
      }
    }

    std::cout << "Summary: " << failedCount << " tests failed." << std::endl;
  }
}

void testConvert(const std::vector<char>& buffer) noexcept {
  auto out = UtfUtils::toUtf32Str({ buffer.data(), buffer.size() });
  UtfUtils::toUtf8Str<Utf8StringBuilder>({ out.c_str(), out.length() });
}

void testCopy(const std::vector<char>& buffer) noexcept {
  std::vector<char> copy;
  copy.reserve(buffer.size());
  memcpy(copy.data(), buffer.data(), buffer.size());
}

void translitProxy(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, std::unique_ptr<char>& out) noexcept {
  transliterate(input, from, to, options, out);
}

void testPerf(bool prof) noexcept {
  std::ifstream input("dist/perf.txt", std::ios::binary);
  if (input.is_open()) {
    std::cout << std::endl << "Perf test" << std::endl;
    std::vector<char> inBuffer(std::istreambuf_iterator<char>(input), {});
    std::unique_ptr<char> out;
    std::cout << "  Transliterate to telugu: "
              << measure<>::duration(translitProxy, inBuffer.data(), "devanagari", "telugu", TranslitOptions::None, out)
                     .count()
              << " ms" << std::endl;

    std::cout << "  Transliterate to tamil: "
              << measure<>::duration(translitProxy, inBuffer.data(), "devanagari", "tamil", TranslitOptions::None, out)
                     .count()
              << " ms" << std::endl;

    std::cout
        << "  Transliterate to roman: "
        << measure<>::duration(translitProxy, inBuffer.data(), "devanagari", "iso", TranslitOptions::None, out).count()
        << " ms" << std::endl;
    if (!prof) {
      std::cout << "  TestConvert Runtime: " << measure<>::duration(testConvert, inBuffer).count() << " ms"
                << std::endl;
      std::cout << "  TestCopy Runtime: " << measure<>::duration(testCopy, inBuffer).count() << " ms" << std::endl;
    }
  }
}

int main(int argc, const char** argv) {
  inditransLogger = [](const std::string& msg) {
    std::cout << msg;
    std::cout << std::endl;
  };
  bool prof = argc > 1 && std::string(argv[1]) == std::string("-p");
  if (argc > 4 && std::string(argv[1]) == std::string("-t")) {
    std::unique_ptr<char> out;
    translitProxy(argv[2], argv[3], argv[4], TranslitOptions::None, out);
    std::cout << out.get() << std::endl;
    return 0;
  }
  if (!prof) {
    testJson();
    testUtf();
    testAllTranslit();
  }
  testPerf(prof);

  return 0;
}
