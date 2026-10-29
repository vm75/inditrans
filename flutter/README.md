# inditrans

[![pub.dev](https://img.shields.io/pub/v/inditrans)](https://pub.dev/packages/inditrans)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](../LICENSE)

A transliteration library for [Indic/Brahmic](https://en.wikipedia.org/wiki/Brahmic_scripts)
and Roman scripts for Flutter and Dart. Uses FFI on native platforms and WASM on Web for
near-native performance, with no server dependency.

- Available on all platforms (Android, iOS, Linux, macOS, Windows, Web).
- ITRANS toggle transliteration — wrap any region in `##…##` to pass it through unchanged.
- XML/HTML tag pass-through — tags are never transliterated.
- Special handling for Tamil, Malayalam, Gurmukhi, and other scripts tuned for readability.
- [Vedic accent](https://en.wikipedia.org/wiki/Vedic_accent) support for Vedic scriptures.

## Supported scripts

### Optional Urdu pronunciation

`transliterateUrduWithLexicon(text, to, bytes, options)` accepts a caller-supplied
IUL1 or IUL2 pack as `Uint8List` to override the bundled experimental dictionary.
Ordinary Urdu conversion uses the bundled dictionary without downloading data.
A single reading is selected without contextual disambiguation. Unknown,
explicitly marked and mixed-script words use the ordinary Urdu reader.
`DisableInference` bypasses the lexicon. See
[lexicon compilation and runtime behavior](../docs/lexicons.md) for the pack format and
current limitations.

**Indic/Brahmic:** Assamese, Bengali, Burmese, Devanagari, Grantha, Gujarati, Gurmukhi,
Kannada, Khmer, Malayalam, Oriya, Sinhala, Tamil, Telugu, Thai, Tibetan

**Roman schemes:**
[IAST](https://en.wikipedia.org/wiki/International_Alphabet_of_Sanskrit_Transliteration) ·
[IPA](https://en.wikipedia.org/wiki/International_Phonetic_Alphabet) ·
[ISO 15919](https://en.wikipedia.org/wiki/ISO_15919) ·
[ITRANS](https://www.aczoom.com/itrans/) ·
[TITUS](https://titus.uni-frankfurt.de/indexe.htm) ·
Indlish (my own take on a readable Roman representation for Indian scripts)

### ITRANS notation notes

`inditrans` implements uniform phonemic vowel mapping adhering to the [ITRANS specification](https://www.aczoom.com/itrans/):
- **Short vowels**: `e` and `o` encode short vowels (e.g. Kannada `ಎ` / `ೆ`, Telugu `ఎ` / `ె`, Tamil `எ` / `ெ`, Malayalam `എ` / `െ`, Devanagari `ऎ` / `ॆ`). `^e` and `^o` remain supported as aliases.
- **Long vowels**: `E` and `O` encode long vowels (e.g. Kannada `ಏ` / `ೇ`, Telugu `ఏ` / `ే`, Tamil `ஏ` / `ே`, Malayalam `ഏ` / `േ`, Devanagari `ए` / `े`).
- Examples: `te` $\to$ `तॆ` / `ತೆ`, `tE` $\to$ `ते` / `ತೇ`; `tere bina` $\to$ `ತೆರೆ ಬಿನ` (Kannada), `tErE bina` $\to$ `ತೇರೇ ಬಿನ` (Kannada) / `तेरे बिन` (Devanagari).

## Requirements

- Flutter ≥ 3.38.10 / Dart SDK ^3.10.8
- No additional runtime dependencies for transliteration

## Installation

```sh
flutter pub add inditrans
```

## Quick start

```dart
import 'package:inditrans/inditrans.dart' as inditrans;

Future<void> main() async {
  // init() must complete before transliterate() is called
  await inditrans.init();

  final result = inditrans.transliterate(
    "श्री॒ गु॒रु॒भ्यो नमः॒ । ह॒रिः॒ ओ३म् ॥",
    inditrans.Script.devanagari,
    inditrans.Script.tamil,
    inditrans.Options.IgnoreVedicAccents + inditrans.Options.TamilSuperscripted,
  );
  print(result);
}
```

## Configuration

Pass an `Option` value (or combine with `+`) as the fourth argument to `transliterate()`:

| Option | Description |
|---|---|
| `Option.TamilTraditional` | Traditional Tamil consonants only |
| `Option.TamilSuperscripted` | Superscripted Tamil consonants |
| `Option.ASCIINumerals` | Force ASCII numerals in output |
| `Option.IgnoreVedicAccents` | Strip Vedic accent markers |
| `Option.RetainSpecialMarkers` | Keep non-standard character markers |
| `Option.NoXMLTagHandling` | Treat XML/HTML tags as plain text |
| `Option.DisableInference` | Disable anuswara assimilation and contextual Tamil pronunciation/letter inference |

For Tamil **source** text, `Option.TamilSuperscripted` also enables `DisableInference` automatically; using it only for Tamil **output** does not. With `DisableInference`, a target's mapped anuswara is emitted instead of a contextually inferred nasal (which may differ in spelling).

The `skipStart` and `skipEnd` parameters (default `"##"`) mark regions that are not
transliterated and are passed through unchanged.

## Development

```sh
flutter pub get          # install Dart dependencies
dart analyze             # lint and static analysis
flutter test             # run tests (native library must be built)
python3 ../scripts/generate_bindings.py  # regenerate FFI bindings after native API changes
```

Checkout builds use the shared engine in `../native/src/`. Before publishing,
run `python3 scripts/prepare_flutter_package.py` from the repository root to
bundle it in `flutter/src/` and copy the changelog. Installed pub.dev packages
build from those bundled sources. Verify that layout with
`python3 tests/integration/flutter_package_smoke.py` from the repository root.

## Documentation

- [Changelog](CHANGELOG.md)
- [Project on GitHub](https://github.com/vm75/inditrans)
- [Node.js / JavaScript package](https://www.npmjs.com/package/@vm75/inditrans)

## License

[MIT](../LICENSE)

## Forward accuracy and Urdu

`Script.urdu` (alias `Script.nastaliq`) supports Urdu/Perso-Arabic text. Use a
Nastaliq-capable font and RTL text direction in your application. The returned
string is ordinary Unicode in logical order. Explicit vowels, aspirates and
shadda are supported; ordinary unvocalized Urdu needs lexical information to
recover omitted vowels, so bare consonants receive no guessed vowel.

Targets use their native letters or documented phonetic approximations; source
distinctions may merge. ISO input now accepts the additional spreadsheet
notations, including ê, ŭ, ĉ, ž, w and the Urdu loan-letter spellings. Artificial
extended-vowel separators/accents have been replaced by native approximations.
Real supported accents remain separate marks. There is no round-trip guarantee.

The script profiles implement transliteration, not complete language-specific
pronunciation. See the repository's [mapping review](https://github.com/vm75/inditrans/blob/main/docs/phonetic-mappings.md)
for exact notation behavior and limitations.

### Reusable Urdu pronunciation lexicon (experimental)

Call `UrduPronunciationLexicon.load(iulBytes)` after `init()`.
It returns null for invalid bytes. Call `lexicon.transliterate(text, Script.iast)`
for repeated conversion, then `lexicon.dispose()` when finished. The handle
owns decoded data and no longer needs the source bytes. The existing one-shot
`transliterateUrduWithLexicon()` remains available.
