# inditrans

[![CI](https://github.com/vm75/inditrans/actions/workflows/ci.yml/badge.svg)](https://github.com/vm75/inditrans/actions/workflows/ci.yml)
[![pub.dev](https://img.shields.io/pub/v/inditrans?label=pub.dev)](https://pub.dev/packages/inditrans)
[![npm](https://img.shields.io/npm/v/@vm75/inditrans?label=npm)](https://www.npmjs.com/package/@vm75/inditrans)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

A transliteration library for [Indic/Brahmic](https://en.wikipedia.org/wiki/Brahmic_scripts) and Latin scripts, implemented as a C++23 engine compiled to a shared library (native) and WASM (Web/Node.js). The same phoneme-level pipeline is exposed to Flutter/Dart and JavaScript/TypeScript.

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
native/     Canonical C++23 engine (source, tests, CLI, and benchmarks)
tool/       Code-generation utilities (script data, bindings, version bump)
test-files/ Shared test-case JSON used by all distributions
```

## Building and testing
 
```sh
make all          # build native CLI, WASM, and Flutter plugin
make test         # run C++ native tests
make testall      # run native, Flutter, and Node.js tests
make bench-all    # run the native benchmark summary
make dll          # cross-compile Windows x86-64 DLL using MinGW-w64
make validate     # validate version consistency and changelog
```

Requires: CMake ≥ 3.20, Clang/LLVM ≥ 17 (or C++23-capable compiler), Emscripten 6.0.10, Flutter ≥ 2.11, Node.js ≥ 18.3 / Yarn.

### Performance benchmarking

The native benchmark suite measures throughput (MB/s and ns/call), per-call latency, output-size expansion, and—on Linux with glibc—heap allocation events, requested allocation bytes, peak requested live bytes, and bytes still live after the measured call.

```sh
make bench-all          # run all available benchmarks and print a formatted summary
make bench              # throughput benchmark (10 cases × 3 approximate target sizes)
make bench-short        # per-call latency benchmark, including result destruction
make output-size-bench  # report input/output bytes and expansion ratio
make mem-bench          # backward-compatible alias for output-size-bench
make bench-alloc-linux  # Linux/glibc allocation probe
```

The allocator probe is optional. `bench-all`, `bench-save`, and `bench-compare` still run on other platforms and skip allocation metrics instead of failing during setup.

To compare an optimization against a saved baseline:

```sh
make bench-save                         # capture baseline metrics before making changes
# ... make changes ...
make bench-compare                      # report changes; always fails on output-hash mismatch
BENCH_STRICT=1 make bench-compare       # also fail on regressions at or above the threshold
```

`bench-compare` validates the FNV-1a output hash for every throughput and short-call case before treating timing changes as meaningful. Performance regressions are flagged at 5% by default; override the threshold with `BENCH_REGRESSION_THRESHOLD=<percent>`.

### Windows DLL cross-compilation (Linux → Windows)

To cross-compile the 64-bit Windows shared library (`inditrans.dll`) from Linux:

```sh
sudo apt-get install mingw-w64 cmake
make dll
```

- **Target**: Windows x86-64 (`inditrans.dll`), copied to `flutter/example.dart/inditrans.dll`.
- **Toolchain**: Cross-compiled using MinGW-w64 (`x86_64-w64-mingw32-g++`) with CMake toolchain `tool/cmake/mingw64.cmake`.
- **Language standard**: Enforces C++23 internally (GCC ≥ 13).
- **Public ABI**: Plain C interface (`extern "C"`) exposing `transliterate`, `isScriptSupported`, and `releaseBuffer`. No C++ types (`std::string`, STL containers, exceptions) cross the boundary, ensuring full compatibility with MSVC callers, Dart FFI, and other runtimes.
- **Runtime dependencies**: Linked with `-static-libgcc -static-libstdc++` to eliminate MinGW C++ runtime DLL dependencies (`libstdc++-6.dll`, `libgcc_s_seh-1.dll`).

## Documentation

- [Flutter package README](flutter/README.md)
- [Node.js package README](nodejs/README.md)
- [Engine architecture](ARCHITECTURE.md)
- [Consolidated changelog](CHANGELOG.md)
- [Release process](docs/release.md)
- [Agent guide](AGENTS.md)

## License

[MIT](LICENSE)
