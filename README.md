# inditrans

[![pub.dev](https://img.shields.io/pub/v/inditrans?label=pub.dev)](https://pub.dev/packages/inditrans)
[![npm](https://img.shields.io/npm/v/@vm75/inditrans?label=npm)](https://www.npmjs.com/package/@vm75/inditrans)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

A transliteration library for [Indic/Brahmic](https://en.wikipedia.org/wiki/Brahmic_scripts) and Latin scripts, implemented as a C++20 engine compiled to a shared library (native) and WASM (Web/Node.js). The same phoneme-level pipeline is exposed to Flutter/Dart and JavaScript/TypeScript.

- No server required. FFI (native) or WASM (Web/Node.js) for near-native performance.
- ITRANS toggle transliteration — wrap any region in `##…##` to pass it through unchanged.
- XML/HTML tag pass-through — tags are never transliterated.
- Special handling for Tamil, Malayalam, Gurmukhi, and other scripts tuned for readability.
- [Vedic accent](https://en.wikipedia.org/wiki/Vedic_accent) support for Vedic scriptures.

## Supported scripts

**Indic/Brahmic:** Assamese, Bengali, Burmese, Devanagari, Gujarati, Gurmukhi, Kannada,
Khmer, Malayalam, Oriya, Sinhala, Tamil, Tamil-Extended, Telugu, Thai, Tibetan

**Standard Latin notations:**
[IAST](https://en.wikipedia.org/wiki/International_Alphabet_of_Sanskrit_Transliteration) ·
[IPA](https://en.wikipedia.org/wiki/International_Phonetic_Alphabet) ·
[ISO 15919](https://en.wikipedia.org/wiki/ISO_15919) ·
[ITRANS](https://en.wikipedia.org/wiki/ITRANS) ·
[TITUS](https://titus.uni-frankfurt.de/indexe.htm) ·
[Harvard-Kyoto](https://en.wikipedia.org/wiki/Harvard-Kyoto) ·
[Velthuis](https://en.wikipedia.org/wiki/Velthuis) ·
[SLP1](https://en.wikipedia.org/wiki/SLP1) ·
[WX](https://en.wikipedia.org/wiki/WX_notation)

**Custom Latin notations:** ReadableLatin

## Distributions

| Platform | Package | Install |
|---|---|---|
| Flutter / Dart | [pub.dev/packages/inditrans](https://pub.dev/packages/inditrans) | `flutter pub add inditrans` |
| Node.js / Browser | [npmjs.com/@vm75/inditrans](https://www.npmjs.com/package/@vm75/inditrans) | `npm install @vm75/inditrans` |

## Repository layout

```
flutter/    Flutter FFI plugin (Dart + C++ shared library + WASM)
nodejs/     TypeScript/JavaScript package (WASM via Emscripten)
js/         WASM build output consumed by nodejs/
native/     Canonical C++20 engine (source, tests, and CLI)
tool/       Code-generation utilities (script data, bindings, version bump)
test-files/ Shared test-case JSON used by all distributions
```

## Building from source

```sh
make all          # build native CLI, WASM, and Flutter plugin
make test         # run C++ native tests
make test_nodejs  # run Node.js tests
make publish      # publish flutter + nodejs packages
```

Requires: CMake ≥ 3.10, Clang/LLVM ≥ 14, Emscripten 2.0.31, Flutter ≥ 2.11, Node.js / Yarn.

## Documentation

- [Flutter package README](flutter/README.md)
- [Node.js package README](nodejs/README.md)
- [Engine architecture](ARCHITECTURE.md)
- [Flutter changelog](flutter/CHANGELOG.md)
- [Node.js changelog](nodejs/CHANGELOG.md)

## License

[MIT](LICENSE)
