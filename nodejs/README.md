# @vm75/inditrans

[![npm](https://img.shields.io/npm/v/@vm75/inditrans)](https://www.npmjs.com/package/@vm75/inditrans)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://github.com/vm75/inditrans/blob/master/LICENSE)

A transliteration library for [Indic/Brahmic](https://en.wikipedia.org/wiki/Brahmic_scripts)
and Latin scripts for Node.js and the browser. Powered by a C++23 engine compiled to WASM
for near-native performance, with no server dependency.

- ITRANS toggle transliteration — wrap any region in `##…##` to pass it through unchanged.
- XML/HTML tag pass-through — tags are never transliterated.
- Special handling for Tamil, Malayalam, Gurmukhi, and other scripts tuned for readability.
- [Vedic accent](https://en.wikipedia.org/wiki/Vedic_accent) support for Vedic scriptures.
- Full TypeScript typings included.

## Supported scripts

**Indic/Brahmic:** Assamese, Bengali, Burmese, Devanagari, Gujarati, Gurmukhi, Kannada,
Khmer, Malayalam, Oriya, Sinhala, Tamil, Tamil-Extended, Telugu, Thai, Tibetan

**Latin/Roman:**
[IAST](https://en.wikipedia.org/wiki/International_Alphabet_of_Sanskrit_Transliteration) ·
[IPA](https://en.wikipedia.org/wiki/International_Phonetic_Alphabet) ·
[ISO 15919](https://en.wikipedia.org/wiki/ISO_15919) ·
[ITRANS](https://www.aczoom.com/itrans/) ·
[TITUS](https://titus.uni-frankfurt.de/indexe.htm) ·
Inglish (my own take on a readable roman/latin representation for Indian scripts)

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
