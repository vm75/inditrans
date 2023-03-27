#include "ut.hpp"

#include "yaml-reader.h"
#include <vector>

using namespace boost::ut;

constexpr std::string_view yamlStr = R"(
# jdkjdfhskf
---
  # lkjlkjlkj
suite1:
  type1:
    description1:
      source: text
      targets1:
        - script+option: text
        - script+option: text
    description2:
      source: text
      targets2:
        - script+option: text
        - script+option: text
---
- description1:
      source: text
      targets1:
        - script+option: true
        - script+option2: 1
- description2:
      source: text
      targets2:
        - script+option: 1.4
        - script+option2: NaN
)";

suite<"YAML tests"> yamlTests = [] {
  "Testing YamlReader"_test = [] {
    auto yaml = YamlReader::parse(std::string_view(yamlStr));
    std::cout << yaml.size() << std::endl;
  };
};
