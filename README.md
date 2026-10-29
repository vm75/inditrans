# inditrans

[![CI](https://github.com/vm75/inditrans/actions/workflows/ci.yml/badge.svg)](https://github.com/vm75/inditrans/actions/workflows/ci.yml)
[![pub.dev](https://img.shields.io/pub/v/inditrans?label=pub.dev)](https://pub.dev/packages/inditrans)
[![npm](https://img.shields.io/npm/v/@vm75/inditrans?label=npm)](https://www.npmjs.com/package/@vm75/inditrans)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

A transliteration library for [Indic/Brahmic](https://en.wikipedia.org/wiki/Brahmic_scripts), Urdu/Perso-Arabic and Roman scripts, implemented as a C++23 engine compiled to a shared library (native) and WASM (Web/Node.js). The same phoneme-level pipeline is exposed to Flutter/Dart and JavaScript/TypeScript.

- No server required. FFI (native) or WASM (Web/Node.js) for near-native performance.
- ITRANS toggle transliteration — wrap any region in `##…##` to pass it through unchanged.
- XML/HTML tag pass-through — tags are never transliterated.
- Special handling for Tamil, Malayalam, Gurmukhi, and other scripts tuned for readability.
- [Vedic accent](https://en.wikipedia.org/wiki/Vedic_accent) support for Vedic scriptures.

Try the [live demo](https://vm75.github.io/inditrans/) to compare scripts, type with ASCII shortcuts, and copy transliterated text.

## Supported scripts

**Indic/Brahmic:** Assamese, Bengali, Burmese, Devanagari, Grantha, Gujarati, Gurmukhi,
Kannada, Khmer, Malayalam, Oriya, Sinhala, Tamil, Telugu, Thai, Tibetan

**Urdu/Perso-Arabic:** Urdu (`urdu`, alias `nastaliq`), rendered with a Nastaliq-capable font. Explicit vowels are supported; omitted vowels are not inferred.

Forward target approximations and notation extensions are described in the
[mapping review](docs/phonetic-mappings.md).

**Standard Roman schemes:**
[IAST](https://en.wikipedia.org/wiki/International_Alphabet_of_Sanskrit_Transliteration) ·
[IPA](https://en.wikipedia.org/wiki/International_Phonetic_Alphabet) ·
[ISO 15919](https://en.wikipedia.org/wiki/ISO_15919) ·
[ITRANS](https://www.aczoom.com/itrans/) ·
[TITUS](https://titus.uni-frankfurt.de/indexe.htm) ·
[Harvard-Kyoto](https://en.wikipedia.org/wiki/Harvard-Kyoto) ·
[Velthuis](https://en.wikipedia.org/wiki/Velthuis) ·
[SLP1](https://en.wikipedia.org/wiki/SLP1) ·
[WX](https://en.wikipedia.org/wiki/WX_notation)

**Custom Roman schemes:** Indlish (my own take on a readable Roman representation for Indian scripts)

Vedic accent support is a per-script capability, independent of these groups.
Tamil belongs to Indic and retains its specialized transliteration rules.
The [demo](demo/README.md) labels choices that support Vedic accent output.

### Symbol coverage and limitations

Digits 0–9, Om and danda punctuation have script mappings. Additional native
punctuation is normalized when converting between scripts:

| Input | Output convention |
|---|---|
| Tibetan `་`, `༌` | Space in other scripts; the nonbreaking distinction is lost |
| Tibetan `༏`, `༑` / `༐`, `༒` | Single / double danda equivalents |
| Sinhala `෴`, Thai `๛`, Khmer `៚` | Double danda equivalent |
| Devanagari `॰`, Gujarati `૰`, Gurmukhi `੶`, Bengali/Assamese `৽` | Native abbreviation sign where available, otherwise ASCII `.` |
| Khmer `៖` | ASCII `:` in other scripts |
| Tibetan `༔`, Urdu `،` | Native comma-like sign where available, otherwise ASCII `,` |

Roman danda output retains each scheme's existing punctuation (`.`/`..` in
IAST). These are deliberately lossy punctuation conventions, not reversible
scholarly encodings. ASCII spaces, commas and colons retain their existing
pass-through behavior; they are not automatically converted to native signs.
Same-script conversion keeps the original punctuation. Symbol tables do not
imply complete Unicode or native-language coverage.

Unmapped characters are preserved, so output can contain source-script signs.
The remaining work is tracked here:

- [Contextual symbols and repetition (#66)](https://github.com/vm75/inditrans/issues/66):
  Burmese `၌ ၍ ၎ ၏`, Khmer `ៗ ៘`, and Thai `ๆ` need word/phrase expansion conventions.
- [Traditional numbers and fractions (#67)](https://github.com/vm75/inditrans/issues/67):
  Tamil `௰–௲`, Malayalam/Odia/Telugu fractions, Tibetan half digits, Khmer
  divination numerals and Shan digits need numeric-system-specific treatment.
- [Specialist signs (#68)](https://github.com/vm75/inditrans/issues/68): currency,
  calendar/accounting, siddham/isshar, opening, ritual and astrological signs,
  plus remaining Urdu punctuation, need preservation or conversion policies.
- [Native-language reading coverage (#69)](https://github.com/vm75/inditrans/issues/69):
  ordinary Burmese, Khmer and Thai text uses letters and marks beyond the
  Sanskrit/Pali-oriented readers. Demo samples use supported spellings; that
  does not establish full language coverage.

### ITRANS notation notes

`inditrans` implements uniform phonemic vowel mapping adhering to the [ITRANS specification](https://www.aczoom.com/itrans/):
- **Short vowels**: `e` and `o` encode short vowels across all scripts (e.g. Kannada `ಎ` / `ೆ`, Telugu `ఎ` / `ె`, Tamil `எ` / `ெ`, Malayalam `എ` / `െ`, Devanagari `ऎ` / `ॆ`). The legacy markers `^e` and `^o` remain supported as aliases for `e` and `o`.
- **Long vowels**: `E` and `O` encode long vowels (e.g. Kannada `ಏ` / `ೇ`, Telugu `ఏ` / `ే`, Tamil `ஏ` / `ே`, Malayalam `ഏ` / `േ`, Devanagari `ए` / `े`). `ee` and `oo` also continue to represent long vowels where standard.
- **Examples**:
  - `te` $\to$ `तॆ` / `ತೆ` (short *e*); `tE` $\to$ `ते` / `ತೇ` (long *e*)
  - `to` $\to$ `तॊ` / `ತೊ` (short *o*); `tO` $\to$ `तो` / `ತೋ` (long *o*)
  - `tere bina` $\to$ `ತೆರೆ ಬಿನ` (Kannada), `तॆरॆ बिन` (Devanagari)
  - `tErE bina` $\to$ `ತೇರೇ ಬಿನ` (Kannada), `तेरे बिन` (Devanagari)

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
scripts/    Build, code-generation, release, and maintenance commands
demo/       Browser demo and its component tests
tests/      Integration tests against built artifacts
test-files/ Shared test-case JSON used by all distributions
```

## Building and testing

```sh
make all          # build native CLI, WASM, and Flutter plugin
make test         # run C++ native tests
make testall      # run native, WASM, Flutter, Node.js, demo, and script tests
make test_demo    # run demo component tests
make test_scripts # run maintenance script regressions
make bench-all    # run the native benchmark summary
make dll          # cross-compile Windows x86-64 DLL using MinGW-w64
make validate     # validate version consistency and changelog
```

Requires: CMake ≥ 3.20, Clang/LLVM ≥ 17 (or C++23-capable compiler), Emscripten 6.0.10, Flutter ≥ 3.38.10 / Dart ≥ 3.10.8, Node.js ≥ 18.3 / Yarn.

See [platform support](docs/platform-support.md) for the declared platform matrix, tested toolchains, and current acceptance evidence.

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
- **Toolchain**: Cross-compiled using MinGW-w64 (`x86_64-w64-mingw32-g++`) with CMake toolchain `scripts/cmake/mingw64.cmake`.
- **Language standard**: Enforces C++23 internally (GCC ≥ 13).
- **Public ABI**: Plain C interface (`extern "C"`) exposing `transliterate`, `transliterateUrduWithLexiconData`, `isScriptSupported`, and `releaseBuffer`. No C++ types (`std::string`, STL containers, exceptions) cross the boundary, ensuring full compatibility with MSVC callers, Dart FFI, and other runtimes.
- **Runtime dependencies**: Linked with `-static-libgcc -static-libstdc++` to eliminate MinGW C++ runtime DLL dependencies (`libstdc++-6.dll`, `libgcc_s_seh-1.dll`).

## Documentation

- [Flutter package README](flutter/README.md)
- [Node.js package README](nodejs/README.md)
- [Engine architecture](ARCHITECTURE.md)
- [Consolidated changelog](CHANGELOG.md)
- [Release process](docs/release.md)
- [Agent guide](AGENTS.md)

## License

Inditrans source code is [MIT](LICENSE). The built-in **experimental Urdu
pronunciation dictionary is separately licensed CC BY-SA 4.0** and attributed
to Wiktionary contributors; see [Urdu data licensing and source hashes](scripts/urdu/README.md).
Distributors of native/WASM binaries must preserve this data attribution and
share-alike terms. The base engine license does not apply to this embedded data.

### Urdu pronunciation (on by default)

Urdu → Indlish now recovers vowels for **4,252 dictionary entries** without
an extra flag or downloadable asset. The 122,869-byte IUL1 corpus is embedded
in native libraries and WASM, with its index initialized lazily on the first
Urdu conversion. Words absent from the dictionary fall back to literal
transliteration; no sentence-level homograph disambiguation is claimed.

```sh
make out/inditrans
./out/inditrans -f urdu -t indlish 'جیسی کرنی ویسی بھرنی۔'
# jaisI karnI vaisI bharnI!
```

Use `-o DisableInference` for deterministic literal Urdu letter mapping,
or `--urdu-lexicon path/to/alternative.iul` to override the bundled
dictionary for CLI conversions. In the browser demo, **Urdu pronunciation is
on by default**, and the existing **Disable inference** option restores
literal output. The experimental data is not independently human-reviewed:
[see limitations](scripts/urdu/README.md).
