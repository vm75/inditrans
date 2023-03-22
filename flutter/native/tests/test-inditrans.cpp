#include "ut.hpp"

#include <chrono>
#include <fstream>
#include <functional>
#include <inditrans.h>
#include <iomanip>
#include <iostream>
#include <json.h>
#include <utilities.h>
#include <vector>

using namespace boost::ut;

using InditransLogger = void(const std::string&);
extern InditransLogger* inditransLogger;

TranslitOptions getTranslitOptions(const std::string_view& optStr) noexcept {
  Utf8String str(optStr);
  static constexpr std::array<std::string_view, static_cast<size_t>(MaxTranslitOptions)> optionStrings = {
    "None",
    "TamilTraditional",
    "TamilSuperscripted",
    "ASCIINumerals",
    "IgnoreVedicAccents",
    "RetainSpecialMarkers",
  };

  TranslitOptions mask { TranslitOptions::None };
  for (const auto& opt : str.split(" \t,:;/|&")) {
    auto match = std::find(optionStrings.begin(), optionStrings.end(), opt);
    if (match != optionStrings.end()) {
      const auto idx = std::distance(optionStrings.begin(), match);
      if (idx > 0) {
        mask = mask | static_cast<TranslitOptions>(1 << (idx - 1));
      }
    }
  }

  return mask;
}

template <class TimeT = std::chrono::milliseconds, class ClockT = std::chrono::steady_clock> struct measure {
  template <class F, class... Args> static auto duration(F&& func, Args&&... args) {
    auto start = ClockT::now();
    std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
    return std::chrono::duration_cast<TimeT>(ClockT::now() - start);
  }
};

void testAllTranslit(std::string_view file) noexcept {
  std::ifstream testsFile(file.data(), std::ios::binary);
  if (testsFile.is_open()) {

    std::vector<char> jsonBuffer(std::istreambuf_iterator<char>(testsFile), {});
    std::string_view bufferView { jsonBuffer.data(), jsonBuffer.size() };
    auto inputsHolder = JsonReader::parseJson(bufferView);
    if (inputsHolder == std::nullopt) {
      return;
    }

    auto inputs = std::get<JsonArray>(*inputsHolder);

    for (const auto& input : inputs) {
      auto inputObj = std::get<JsonOject>(input);

      auto description = inputObj.get<std::string>("description");
      auto text = inputObj.get<std::string>("text");
      auto source = inputObj.get<std::string>("script");
      auto type = inputObj.get<std::string>("type");
      if (source == std::nullopt && type == std::nullopt) {
        continue;
      }

      auto targets = inputObj.get<JsonArray>("targets");

      if (*type == "any-to-any") {
        for (auto& first : *targets) {
          for (auto& second : *targets) {
            auto sourceObj = std::get<JsonOject>(first);
            auto targetObj = std::get<JsonOject>(second);
            auto source = sourceObj.get<std::string>("script");
            auto target = targetObj.get<std::string>("script");
            if (source == target || !isScriptSupported(source->c_str()) || !isScriptSupported(target->c_str())) {
              continue;
            }
            auto text = sourceObj.get<std::string>("text");
            auto expected = targetObj.get<std::string>("text");

            std::string str = *description + " ( " + *source + " -> " + *target + " )";
            test(str.c_str()) = [&] {
              auto res = transliterate(*text, *source, *target, TranslitOptions::None);
              expect(res == *expected);
            };
          }
        }
        continue;
      }

      for (auto& entry : *targets) {
        auto targetObj = std::get<JsonOject>(entry);
        auto target = targetObj.get<std::string>("script");
        if (!isScriptSupported(source->c_str()) || !isScriptSupported(target->c_str())) {
          continue;
        }
        auto expected = targetObj.get<std::string>("text");

        std::string options {};
        if (targetObj.get<std::string>("options") != std::nullopt) {
          options = *targetObj.get<std::string>("options");
        }

        std::string str = *description + " ( " + *source + " -> " + *target + ", " + options + " )";
        test(str.c_str()) = [&] {
          auto res = transliterate(*text, *source, *target, getTranslitOptions(options));
          expect(res == *expected);

          if (type != std::nullopt && *type == "to-and-fro") {
            res = transliterate(*text, *source, *target, getTranslitOptions(options + " RetainSpecialMarkers"));
            auto res2 = transliterate(res, *target, *source, getTranslitOptions(options));
            expect(res2 == *text);
          }
        };
      }
    }
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
        << "  Transliterate to Latin: "
        << measure<>::duration(translitProxy, inBuffer.data(), "devanagari", "iso", TranslitOptions::None, out).count()
        << " ms" << std::endl;
    if (!prof) {
      std::cout << "  TestConvert Runtime: " << measure<>::duration(testConvert, inBuffer).count() << " ms"
                << std::endl;
      std::cout << "  TestCopy Runtime: " << measure<>::duration(testCopy, inBuffer).count() << " ms" << std::endl;
    }
  }
}

void testInitransMain() noexcept {
  suite<"test-cases.json tests"> inditransTests = [] { testAllTranslit("test-files/test-cases.json"); };
  suite<"sanscript-tests.json tests"> sanscriptTests = [] { testAllTranslit("test-files/sanscript-tests.json"); };
}