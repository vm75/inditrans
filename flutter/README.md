# inditrans

An experimental transliterator library for some [Indic/Brahmi](https://en.wikipedia.org/wiki/Brahmic_scripts) scripts.

* Available on all platforms. Uses ffi for non-Web platforms and wasm for Web.
* Self-contained and performant.
* Supports [vedic accents](https://en.wikipedia.org/wiki/Vedic_accent). Useful for vedic scriptures.

References:
- [inditrans on Github](https://github.com/vm75/inditrans)
- [inditrans on pub.dev](https://pub.dev/packages/inditrans)

## Supported scripts
* Brahmi scripts:
  * bengali
  * devanagari
  * gujarati
  * gurmukhi
  * kannada
  * malayalam
  * oriya
  * telugu
  * tamil
* Roman scripts
  * iast (https://en.wikipedia.org/wiki/International_Alphabet_of_Sanskrit_Transliteration)
  * ipa (https://en.wikipedia.org/wiki/International_Phonetic_Alphabet)
  * iso 15919 (https://en.wikipedia.org/wiki/ISO_15919)
  * itrans (https://en.wikipedia.org/wiki/ITRANS)
  * romansimple (custom)

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
