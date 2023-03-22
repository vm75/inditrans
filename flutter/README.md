# inditrans

An experimental transliterator library for some [Indic/Brahmic](https://en.wikipedia.org/wiki/Brahmic_scripts) scripts.

- Available on all platforms. Uses ffi for non-Web platforms and wasm for Web.
- Uses ffi (non-Web)/wasm (Web) for near native performance.
- No dependencies (for transliteration) and no server needed.
- Supports [vedic accents](https://en.wikipedia.org/wiki/Vedic_accent). Useful for vedic scriptures.
- Smart transliteration to and from Tamil. Supports tamil superscript.
- Supports ITRANS toggle transliteration with ## and skipping HTML tags

References:

- [inditrans on Github](https://github.com/vm75/inditrans)
- [inditrans for Flutter](https://pub.dev/packages/inditrans)
- [inditrans for nodejs](https://www.npmjs.com/package/@vm75/inditrans)

## Supported scripts

- Indic/Brahmic scripts:
  - Assamese
  - Bengali
  - Burmese
  - Devanagari
  - Gujarati
  - Gurmukhi
  - Kannada
  - Khmer
  - Malayalam
  - Oriya
  - Sinhala
  - Tamil
  - Tamil-Extended
  - Telugu
  - Tibetan
  - Thai
- Latin/Roman scripts:
  - IAST (https://en.wikipedia.org/wiki/International_Alphabet_of_Sanskrit_Transliteration)
  - IPA (https://en.wikipedia.org/wiki/International_Phonetic_Alphabet)
  - ISO 15919 (https://en.wikipedia.org/wiki/ISO_15919)
  - ITRANS (https://en.wikipedia.org/wiki/ITRANS)
  - ReadableLatin (easy to read in Latin alphabets with some customizations)
  - TITUS (https://titus.uni-frankfurt.de/indexe.htm)

## Installation

```
flutter pub add inditrans
```

## Usage example

```dart
import 'package:inditrans/inditrans.dart' as inditrans;

Future<void> main() async {
  // Init needs to complete before Inditrans could be used
  await inditrans.init();

  final result = inditrans.transliterate(
      "श्री॒ गु॒रु॒भ्यो नमः॒ । ह॒रिः॒ ओ३म् ॥",
      inditrans.Script.devanagari,
      inditrans.Script.tamil,
      inditrans.Options.IgnoreVedicAccents + inditrans.Options.TamilSuperscripted);
  ...
}
```
