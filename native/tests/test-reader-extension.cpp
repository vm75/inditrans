#include "ut.hpp"
#include "inditrans.h"
#include "reader_extension.h"

using namespace boost::ut;

namespace {
struct ExampleExtension {
  bool hasPending() const noexcept { return false; }
  bool pull(const char*& ptr, const char* end, std::string_view, std::string_view,
      TranslitOptions, TokenOrString* output, size_t& size) noexcept {
    if (ptr == end || *ptr != '@') return false;
    ++ptr;
    output[size++] = ScriptToken(TokenType::Vowel, 1, ScriptType::Indic);
    return true;
  }
};
}

suite<"Reader extension"> readerExtensionTests = [] {
  "caller-provided reader is script neutral and opt in"_test = [] {
    ExampleExtension source;
    auto extension = ReaderExtension::bind(source);
    std::string output;
    expect(transliterateWithReaderExtension("@", "iast", "devanagari", None, output, extension, "##", "##"));
    expect(output == "आ");
    expect(transliterate("@", "iast", "devanagari", None) == "@");
    output.clear();
    expect(transliterateWithReaderExtension("@", "iast", "iast", None, output, extension, "##", "##"));
    expect(output == "@");
  };
};
