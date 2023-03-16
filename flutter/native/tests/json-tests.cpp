#include <iostream>
#include <json.h>
#include <vector>

void testJson() noexcept {
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
  if (!val) {
    std::cout << "Failed to parse JSON" << std::endl;
    return;
  }
  auto obj = std::get<JsonOject>(*val);
  auto types = obj.get<JsonArray>("types");
  if (!types) {
    std::cout << "Failed to get types" << std::endl;
    return;
  }
  for (const auto& type : *types) {
    if (std::holds_alternative<std::string>(type)) {
      std::cout << std::get<std::string>(type) << std::endl;
    } else if (std::holds_alternative<int64_t>(type)) {
      std::cout << std::get<int64_t>(type) << std::endl;
    } else if (std::holds_alternative<long double>(type)) {
      std::cout << std::get<long double>(type) << std::endl;
    } else if (std::holds_alternative<bool>(type)) {
      std::cout << std::get<bool>(type) << std::endl;
    } else if (std::holds_alternative<void*>(type)) {
      std::cout << "null" << std::endl;
    }
  }

  auto nested = obj.get<JsonArray>("nested");
  if (!nested) {
    std::cout << "Failed to get nested" << std::endl;
    return;
  }
  for (const auto& type : *nested) {
    if (std::holds_alternative<std::string>(type)) {
      std::cout << std::get<std::string>(type) << std::endl;
    } else if (std::holds_alternative<JsonOject>(type)) {
      auto obj = std::get<JsonOject>(type);
      auto x = obj.get<std::string>("x");
      if (!x) {
        std::cout << "Failed to get x" << std::endl;
        return;
      }
      std::cout << *x << std::endl;
    }
  }
}

int main() {
  testJson();
  return 0;
}