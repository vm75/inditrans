# inditrans

[![pub.dev](https://img.shields.io/pub/v/inditrans)](https://pub.dev/packages/inditrans)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](../LICENSE)

A transliteration library for [Indic/Brahmic](https://en.wikipedia.org/wiki/Brahmic_scripts)
and Latin scripts for Flutter and Dart. Uses FFI on native platforms and WASM on Web for
near-native performance, with no server dependency.

- Available on all platforms (Android, iOS, Linux, macOS, Windows, Web).
- ITRANS toggle transliteration — wrap any region in `##…##` to pass it through unchanged.
- XML/HTML tag pass-through — tags are never transliterated.
- Special handling for Tamil, Malayalam, Gurmukhi, and other scripts tuned for readability.
- [Vedic accent](https://en.wikipedia.org/wiki/Vedic_accent) support for Vedic scriptures.

## Supported scripts

**Indic/Brahmic:** Assamese, Bengali, Burmese, Devanagari, Gujarati, Gurmukhi, Kannada,
Khmer, Malayalam, Oriya, Sinhala, Tamil, Tamil-Extended, Telugu, Thai, Tibetan

**Latin/Roman:**
[IAST](https://en.wikipedia.org/wiki/International_Alphabet_of_Sanskrit_Transliteration) ·
[IPA](https://en.wikipedia.org/wiki/International_Phonetic_Alphabet) ·
[ISO 15919](https://en.wikipedia.org/wiki/ISO_15919) ·
[ITRANS](https://en.wikipedia.org/wiki/ITRANS) ·
[TITUS](https://titus.uni-frankfurt.de/indexe.htm) ·
ReadableLatin

## Requirements

- Flutter ≥ 2.11.0 / Dart SDK ^3.10.8
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

The `skipStart` and `skipEnd` parameters (default `"##"`) mark regions that are not
transliterated and are passed through unchanged.

## Development

```sh
flutter pub get          # install Dart dependencies
dart analyze             # lint and static analysis
flutter test             # run tests (native library must be built)
dart run ffigen --config ffigen.yaml  # regenerate FFI bindings after native API changes
```

## Documentation

- [Changelog](CHANGELOG.md)
- [Project on GitHub](https://github.com/vm75/inditrans)
- [Node.js / JavaScript package](https://www.npmjs.com/package/@vm75/inditrans)

## License

[MIT](../LICENSE)
