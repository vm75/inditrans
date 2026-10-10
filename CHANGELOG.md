# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.13.0]
- **Transliteration features**:
  - Add `skipStart` and `skipEnd` delimiter support to allow passthrough of protected regions (defaults to `##...##`).
  - Add Harvard-Kyoto (`Script.hk`) and additional notation support across runtimes.
  - Handle empty, single-character, and oversized protected-span delimiters safely.
- **Engine architecture & performance**:
  - Upgrade engine to C++23 standard across native, Flutter, Apple, Android, Windows, and WASM builds.
  - Optimize memory usage: streaming chunk readers with reusable token chunks, non-owning character maps, stack-allocated readers and writers, and allocation-free sinks.
  - Replace dynamic runtime map allocations with compact, static lookup tables and specialized script type conversions.
  - Match transliteration buffer allocation and deallocation across boundaries (`releaseBuffer`).
- **Toolchains & platforms**:
  - Reproducible Linux-to-Windows x86-64 DLL cross-compilation with MinGW-w64 (`make dll`) exporting strict C ABI symbols (`transliterate`, `isScriptSupported`, `releaseBuffer`).
  - Statically link MinGW C++ runtime libraries to eliminate external DLL runtime dependencies.
  - Upgrade and pin Emscripten to 6.0.10 for standalone Flutter WASM and Node.js WASM builds with deterministic parity checks.
  - Upgrade Node.js package to TypeScript 6.x and Node 24 support.
  - Raise minimum Flutter SDK constraint to 3.38.10 / Dart 3.10.8 across declared platforms.
- **Release engineering & repository reorganization**:
  - Reorganize repository into canonical root structure: move shared C++ engine sources, CLI, and test suite from `flutter/native/` to root `native/`.
  - Establish automated, tag-driven release workflow (`.github/workflows/release.yml`) for `vX.Y.Z` Git tags.
  - Enable GitHub Actions OIDC trusted publishing for both pub.dev and npm (`@vm75/inditrans`).
  - Move repository code-generation and maintenance tooling to Python.
  - Add comprehensive release regression test suite (`tool/test_release_engineering.py`) and validator alignment.
  - Expand project documentation: consolidated root `CHANGELOG.md`, `ARCHITECTURE.md`, `AGENTS.md`, and distribution guides.

## [0.12.1]
* Fixed tamil supersctipt encoding

## [0.12.0]
* Fix toScript

## [0.11.1]
* Update dart Option

## [0.11.0]
* Fixed error conditions during transliterate. Fixed android build.

## [0.10.1]
* Update topics

## [0.10.0]
* Updated universal_ffi to version 1.1.0

## [0.9.0]
* flutter: use universal_ffi

## [0.8.0]
* upgraded wasm_ffi for flutter and added vanilla dart example

## [0.7.1]
* Fixed Windows compile issues

## [0.7.0]
* Fixing many issues in flutter. Adding indic to scripts.

## [0.6.0]
* Refactoring and constants generation

## [0.5.0]
* Added support for Gurmukhi adhak

## [0.4.6]
* Fixed Zero Width Joiner and Non-Joiner

## [0.4.5]
* Support compound alternates and Malayalam Chillus

## [0.4.4]
* Fixed ITRANS and unit tests

## [0.4.3]
* added support for xml tag skip

## [0.4.2]
* Added case insensitive support for iast, iso and ipa. Added support for extended latin chars with composition

## [0.4.1]
* Split symbols, Zero Width chars and Vedic symbols

## [0.4.0]
* Lot of refactoring.

## [0.3.6]
* Refactored nodejs interface. Updated readme.

## [0.3.5]
* Fixed wasm, tests and nodejs support

## [0.3.4]
* Fixed some corner case

## [0.3.3]
* Added support for Tamil prefix

## [0.3.2]
* Renamed indicroman to readablelatin

## [0.3.1]
* Improved Tamil transliteration based on rules

## [0.2.0]
* Fixed Sinhara symbols

## [0.1.9]
* modified easyroman and renamed to readablelatin

## [0.1.8]
* Made InferAnuswara automatic. Fixed Diacritic for Vowels.

## [0.1.7]
* fixed bug for indic

## [0.1.6]
* exported indic, added 2 scripts, fixed roman

## [0.1.5]
* Added fromString to Script enum

## [0.1.4]
* re-publish from correct branch.

## [0.1.3]
* Fixed build error

## [0.1.2]
* Added option to use ASCII numerals

## [0.1.1]
* fixed inferAnuswara for consonants after ma

## [0.1.0]
* Removed unnecessary Inditrans class. Added helpful scripts.

## [0.0.6]
* fixed example.

## [0.0.5]
* fixed android build issue.

## [0.0.4]
* removed unnecessary code and files.

## [0.0.3]
* Added comments and fixed formatting and example.

## [0.0.2]
* Fixed ffi_lib for non-web

## [0.0.1]
* Initial release
