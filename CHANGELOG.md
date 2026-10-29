# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.17.1]
* Use readable Roman vowel combinations in Indlish, fixing misplaced breve marks in Urdu output.

## [0.17.0]
* Add generic pronunciation lexicons with bundled Urdu wordlist support, reusable lexicon handles, build tooling and cross-runtime APIs.

## [0.16.5]
* Accept common IPA vowel and consonant variants, breathy aspirates, affricates, diphthongs and syllabic liquids.
* Handle IPA syllable and stress marks, consonant gemination and nasal vowels, including precomposed forms.

## [0.16.4]
* Expand approximate script coverage, including Burmese, Thai, Khmer, Gurmukhi, Bengali, Assamese, Telugu and Kannada, and document remaining unmapped characters.
* Render Indlish velar and palatal nasals as ng and ny outside consonant clusters.

## [0.16.3]
* Support native punctuation, Tibetan syllable separators and abbreviation signs across scripts.
* Use supported demo samples, verify sample and keyboard coverage against WASM, and document remaining symbol limitations.
* Preserve separate edits for each demo input script and restore its sample independently.

## [0.16.2]
* Improve the demo keyboard with curated script keys and preserve existing ASCII shortcuts.
* Support Burmese medial signs and fix Tamil superscript Vedic accent attachment.
* Improve Indic Vedic accent font fallback in the demo.

## [0.16.1]
- Bundle native engine sources in the Flutter package so Android and desktop/Apple plugin builds work outside the monorepo (fixes #64).

## [0.16.0]
- Use lightweight same-script copying with configurable marker, Tamil annotation, accent, XML tag, and protected-span handling.
- Disable Vedic accent output and demo accent keys for WX and SLP1.
- Disable Vedic accent output and demo accent keys for Grantha.
- Group scripts as Indic, Roman, and Perso-Arabic, with explicit Vedic accent output support; keep Tamil in Indic with its specialized transliteration handlers.
- Use Roman terminology consistently across engine code, generated data, tooling, demo, and documentation.
- Replace the Malayalam-based `tamilExtended` profile with Unicode Grantha (`grantha`) across the engine, Dart/TypeScript/JavaScript APIs, and demo keyboard. The old script ID is removed.

## [0.15.0]
- Specialize common readers once per call to omit Thai and Urdu checks from Vedic, Tamil and Roman conversion loops.
- Focus native performance comparisons on Vedic scripts, Tamil and ISO; promote this engine to the main performance baseline.
- Keep MinGW support runtimes static when rebuilding the Windows example DLL.
- Add Urdu/Perso-Arabic transliteration (`urdu`, alias `nastaliq`), with explicit vowels, aspirates, shadda and limited hamza handling; configure Nastaliq fonts and RTL panes in the demo.
- Add missing ISO-profile notation inputs and native affricate, alveolar-stop, nasal and visarga-allophone identities; keep Sinhala ae distinct from candra e.
- Replace synthetic vowel/separator and consonant-marker approximations with native forms; make native reader choices explicit and reject unexplained generator collisions.
- Preserve Tamil written word-final stops with pulli instead of deleting them.
- Distinguish Odia ଵ (v) and ୱ (w), and emit Gurmukhi adhak for simple geminates after vowels.
- Correct Thai vowel order and Tibetan/Burmese/Khmer stacking; preserve additional real accents and diacritics rather than overwriting them.
- Preserve literal/protected/XML spans without applying writer-marker filtering.
- Display, copy and save raw engine output, deploy matching demo engine assets, and run actual Flutter tests from `make testall`.

## [0.14.1]
- **Indlish Romanization**:
  - Rename `readableLatin` to `indlish` across native C++ engine, Flutter/Dart (`Script.indlish`), and TypeScript/JavaScript packages (`Script.indlish`).
  - Update vowel and diacritic notation for intuitive English readability.
  - Implement contextual anuswara assimilation before varga consonants: velar, palatal, and dental stops assimilate to `n`, retroflex stops assimilate to `N`, and labial stops assimilate to `m`.
  - Fallback to explicit `ṁ` when anuswara occurs before non-varga consonants (sibilants, semivowels, aspirates), vowels, symbols, raw-string skip boundaries, or at word endings.
  - Honor `DisableInference` option to bypass contextual assimilation and consistently output explicit `ṁ`.
  - Preserve chandrabindu as `m̐` without nasal assimilation.

## [0.14.0]
- Add `DisableInference` across native, Flutter, Node.js, and standalone JavaScript APIs to disable anuswara assimilation and contextual Tamil pronunciation and letter substitutions.
- Automatically disable inference when reading Tamil with `TamilSuperscripted`, including case-insensitive source names; Tamil output formatting alone continues to infer pronunciation.
- Rebuild bundled WASM engines and cover inference options in native, shared fixture, and standalone WASM tests.

## [0.13.1]
- **Transliteration features**:
  - Adopt uniform phonemic mapping for ITRANS vowels adhering to the [ITRANS specification](https://www.aczoom.com/itrans/): `e`/`o` represent short vowels (Kannada `ಎ`/`ಒ`, Devanagari `ऎ`/`ऒ`), `E`/`O` represent long vowels (`ಏ`/`ಓ`, `ए`/`ओ`), with `^e`/`^o` supported as aliases (fixes #37).

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
  - Add comprehensive release regression test suite (`scripts/tests/test_release_engineering.py`) and validator alignment.
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
* Added case insensitive support for iast, iso and ipa. Added support for extended roman chars with composition

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
