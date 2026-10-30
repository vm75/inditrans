# @vm75/inditrans

[![npm](https://img.shields.io/npm/v/@vm75/inditrans)](https://www.npmjs.com/package/@vm75/inditrans)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://github.com/vm75/inditrans/blob/master/LICENSE)

A transliteration library for [Indic/Brahmic](https://en.wikipedia.org/wiki/Brahmic_scripts)
and Roman scripts for Node.js and the browser. Powered by a C++23 engine compiled to WASM
for near-native performance, with no server dependency.

- ITRANS toggle transliteration — wrap any region in `##…##` to pass it through unchanged.
- XML/HTML tag pass-through — tags are never transliterated.
- Special handling for Tamil, Malayalam, Gurmukhi, and other scripts tuned for readability.
- [Vedic accent](https://en.wikipedia.org/wiki/Vedic_accent) support for Vedic scriptures.
- Full TypeScript typings included.

## Supported scripts

### Optional Urdu pronunciation

`transliterateUrduWithLexicon(text, to, bytes, options)` accepts an IUL1 or IUL2
pack as a `Uint8Array` to override the bundled experimental dictionary. Ordinary
Urdu conversion uses that dictionary without downloading data. A single reading
is selected without contextual disambiguation. Unknown, explicitly marked and
mixed-script words use the ordinary Urdu reader. `Option.DisableInference` bypasses the lexicon.
The pack-generation format and its current limitations are described in
[lexicon compilation and runtime behavior](../docs/lexicons.md).

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

- Node.js ≥ 18.3 (or modern browser with WebAssembly support)

## Installation

```sh
npm install @vm75/inditrans
# or
yarn add @vm75/inditrans
```

## Quick start

```typescript
import * as inditrans from '@vm75/inditrans';

async function main() {
  // init() must complete before transliterate() is called
  await inditrans.init();

  const result = inditrans.transliterate(
    "श्री॒ गु॒रु॒भ्यो नमः॒ । ह॒रिः॒ ओ३म् ॥",
    inditrans.Script.devanagari,
    inditrans.Script.tamil,
    inditrans.Option.IgnoreVedicAccents | inditrans.Option.TamilSuperscripted,
  );
  console.log(result);
}

main();
```

## API

### `init(): Promise<void>`

Loads and initialises the WASM module. Must be awaited before any other call.

### `transliterate(text, from, to, options?): string`

Transliterates `text` from the `from` script to the `to` script.

| Parameter | Type | Description |
|---|---|---|
| `text` | `string` | Input text |
| `from` | `Script` | Source script |
| `to` | `Script` | Target script |
| `options` | `Option` | Bitwise combination of option flags (default: `Option.None`) |

### `isScriptSupported(script: string): boolean`

Returns `true` when the given script name is recognised.

## Options

| Option | Value | Description |
|---|---|---|
| `Option.None` | `0` | No options (default) |
| `Option.TamilTraditional` | `1` | Traditional Tamil consonants only |
| `Option.TamilSuperscripted` | `2` | Superscripted Tamil consonants |
| `Option.DevanagariTraditional` | `128` | Traditional Devanagari orthography (maps Dravidian short e/o and consonants) |
| `Option.ASCIINumerals` | `4` | Force ASCII numerals in output |
| `Option.IgnoreVedicAccents` | `8` | Strip Vedic accent markers |
| `Option.RetainSpecialMarkers` | `16` | Keep non-standard character markers |
| `Option.NoXMLTagHandling` | `32` | Treat XML/HTML tags as plain text |
| `Option.DisableInference` | `64` | Disable anuswara assimilation and contextual Tamil pronunciation/letter inference |

For Tamil **source** text, `Option.TamilSuperscripted` automatically enables `DisableInference`. The same flag on Tamil **output** does not disable inference. When disabled, a mapped anuswara spelling is emitted rather than assimilated to the next consonant.

Combine flags with the bitwise OR operator: `Option.IgnoreVedicAccents | Option.TamilSuperscripted`.

## Development

```sh
yarn install    # install dependencies
yarn test       # build WASM and run tests
```

## Documentation

- [Changelog](CHANGELOG.md)
- [Project on GitHub](https://github.com/vm75/inditrans)
- [Flutter / Dart package](https://pub.dev/packages/inditrans)

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

For repeated Urdu conversions, create `new UrduPronunciationLexicon(iulBytes)`
and call `lexicon.transliterate(text, Script.iast)` as often as needed.
Always call `lexicon.dispose()` in a `finally` block; do not use a disposed
lexicon. Invalid lexicon bytes throw at construction. The original one-shot
`transliterateUrduWithLexicon()` remains available.
