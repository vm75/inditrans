#include "ut.hpp"

#include <json-reader.h>
#include <vector>

using namespace boost::ut;

suite<"JSON tests"> jsonTests = [] {
  "Testing JsonReader"_test = [] {
    auto val = JsonReader::parseJson(std::string_view(R"(
      {
        // Line comment
        "types" : ["om=\u0950", 123, -123, 1234.5678, true, false, null],
        "nested": [ "val1", "val2", {
          "x" : /*
        Block comment
        */  "y"
        }

        ]
      }
    )"));
    expect(val != std::nullopt);
    auto obj = std::get<JsonOject>(*val);
    auto types = obj.get<JsonArray>("types");
    expect(types != std::nullopt);
    expect(types->size() == 7);
    expect(std::get<std::string>((*types)[0]) == "om=ॐ");
    expect(std::get<int64_t>((*types)[1]) == 123);
    expect(std::get<int64_t>((*types)[2]) == -123);
    expect(std::get<long double>((*types)[3]) - static_cast<long double>(1234.5678) < 0.0001);
    expect(std::get<bool>((*types)[4]) == true);
    expect(std::get<bool>((*types)[5]) == false);
    expect(std::get<void*>((*types)[6]) == nullptr);

    auto nested = obj.get<JsonArray>("nested");
    expect(nested != std::nullopt);
    expect(nested->size() == 3);
    expect(std::get<std::string>((*nested)[0]) == "val1");
    expect(std::get<std::string>((*nested)[1]) == "val2");
    auto nestedObj = std::get<JsonOject>((*nested)[2]);
    expect(nestedObj.get<std::string>("x") == "y");
  };
};
