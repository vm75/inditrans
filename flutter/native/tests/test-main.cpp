#include "ut.hpp"

using namespace boost::ut;

void testInitransMain() noexcept;

int main() {

  suite<"inditrans tests"> inditransTests = [] { testInitransMain(); };

  return 0;
}
