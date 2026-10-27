// Use canonical sources in a checkout and bundled sources in a pub.dev package.
#if __has_include("../../../native/src/inditrans.cpp")
#include "../../../native/src/inditrans.cpp"
#else
#include "../../src/inditrans.cpp"
#endif
