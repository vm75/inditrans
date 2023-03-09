#include <iostream>
#include <json.h>
#include <vector>

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

int main() {
  testJson();
  return 0;
}