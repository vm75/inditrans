# inditrans

A functional transliterator library for the most common [Indic/Brahmic](https://en.wikipedia.org/wiki/Brahmic_scripts) scripts.

- No dependencies and no server needed. Uses wasm for near native performance.
- Supports ITRANS toggle transliteration with ## and skipping XML tags
- Special handling for scripts like Tamil, Malayalam, Gurmukhi, etc. Tuned for reability.
- Supports [vedic accents](https://en.wikipedia.org/wiki/Vedic_accent) for vedic scriptures.

<!-- - [Demo](http://vm75.github.io/inditrans/example/) -->
- [Changelog](CHANGELOG.md)

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
npm install @vm75/inditrans
```

## Usage example

```javascript
import * as inditrans from 'inditrans';

async function test() {
  // Init needs to complete before Inditrans could be used
  await indiTrans.init();

  result = inditrans.transliterate(
    "श्री॒ गु॒रु॒भ्यो नमः॒ । ह॒रिः॒ ओ३म् ॥",
    inditrans.Script.devanagari,
    inditrans.Script.tamil,
    inditrans.Option.IgnoreVedicAccents + inditrans.Option.TamilSuperscripted
  );
  ...
}
```
