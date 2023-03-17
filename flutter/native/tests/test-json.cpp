#include "doctest.h"

#include <json.h>
#include <vector>

TEST_CASE("Testing JsonReader") {
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
  CHECK(val != std::nullopt);
  auto obj = std::get<JsonOject>(*val);
  auto types = obj.get<JsonArray>("types");
  CHECK(types != std::nullopt);
  CHECK(types->size() == 7);
  CHECK(std::get<std::string>((*types)[0]) == "om=ॐ");
  CHECK(std::get<int64_t>((*types)[1]) == 123);
  CHECK(std::get<int64_t>((*types)[2]) == -123);
  CHECK(std::get<long double>((*types)[3]) - static_cast<long double>(1234.5678) < 0.0001);
  CHECK(std::get<bool>((*types)[4]) == true);
  CHECK(std::get<bool>((*types)[5]) == false);
  CHECK(std::get<void*>((*types)[6]) == nullptr);

  auto nested = obj.get<JsonArray>("nested");
  CHECK(nested != std::nullopt);
  CHECK(nested->size() == 3);
  CHECK(std::get<std::string>((*nested)[0]) == "val1");
  CHECK(std::get<std::string>((*nested)[1]) == "val2");
  auto nestedObj = std::get<JsonOject>((*nested)[2]);
  CHECK(nestedObj.get<std::string>("x") == "y");
}
