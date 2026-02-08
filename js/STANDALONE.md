# Inditrans Standalone Wrapper

A minimal JavaScript wrapper for using `inditrans.js` in your projects without adding it as a dependency.

## Quick Start

### 1. Copy Files

Copy these two files to your project:

- `public/inditrans.js` (the WASM module, ~89KB)
- `inditrans-standalone.js` (the wrapper)

### 2. Load in HTML

```html
<!-- Load inditrans.js first -->
<script src="path/to/inditrans.js"></script>

<!-- Then load the wrapper -->
<script src="path/to/inditrans-standalone.js"></script>
```

### 3. Use the API

```javascript
// Wait for the module to be ready
Inditrans.ready().then(() => {
  // Transliterate text
  const result = Inditrans.transliterate(
    'नमस्ते',
    Inditrans.Script.devanagari,
    Inditrans.Script.iast,
    Inditrans.Option.None
  );

  console.log(result); // Output: "namaste"
});
```

## API Reference

### `Inditrans.ready()`

Returns a Promise that resolves when the WASM module is initialized.

```javascript
Inditrans.ready()
  .then(() => {
    console.log('Ready to transliterate!');
  })
  .catch(error => {
    console.error('Failed to initialize:', error);
  });
```

### `Inditrans.transliterate(text, fromScript, toScript, options)`

Transliterates text from one script to another.

**Parameters:**

- `text` (string): The text to transliterate
- `fromScript` (string): Source script (use `Inditrans.Script.*`)
- `toScript` (string): Target script (use `Inditrans.Script.*`)
- `options` (number): Options flags (use `Inditrans.Option.*`, default: `Inditrans.Option.None`)

**Returns:** The transliterated text (string)

**Example:**

```javascript
const result = Inditrans.transliterate(
  'श्री गुरुभ्यो नमः',
  Inditrans.Script.devanagari,
  Inditrans.Script.iast,
  Inditrans.Option.None
);
// Output: "śrī gurubhyo namaḥ"
```

### `Inditrans.isScriptSupported(script)`

Check if a script is supported.

**Parameters:**

- `script` (string): The script name to check

**Returns:** `true` if supported, `false` otherwise

**Example:**

```javascript
if (Inditrans.isScriptSupported('devanagari')) {
  console.log('Devanagari is supported!');
}
```

### `Inditrans.getSupportedScripts()`

Get all supported scripts.

**Returns:** Array of script names (string[])

**Example:**

```javascript
const scripts = Inditrans.getSupportedScripts();
console.log(scripts); // ['assamese', 'bangla', 'devanagari', ...]
```

## Enums

### `Inditrans.Script`

Supported scripts for transliteration:

```javascript
Inditrans.Script = {
  assamese: 'assamese',
  bangla: 'bangla',
  bengali: 'bengali',
  burmese: 'burmese',
  devanagari: 'devanagari',
  gujarati: 'gujarati',
  gurmukhi: 'gurmukhi',
  hindi: 'hindi',
  hk: 'hk',
  iast: 'iast',
  indic: 'indic',
  ipa: 'ipa',
  iso: 'iso',
  itrans: 'itrans',
  kannada: 'kannada',
  khmer: 'khmer',
  malayalam: 'malayalam',
  odia: 'odia',
  oriya: 'oriya',
  punjabi: 'punjabi',
  readableLatin: 'readableLatin',
  sanskrit: 'sanskrit',
  sinhala: 'sinhala',
  slp1: 'slp1',
  tamil: 'tamil',
  tamilExtended: 'tamilExtended',
  telugu: 'telugu',
  thai: 'thai',
  tibetan: 'tibetan',
  titus: 'titus',
  velthuis: 'velthuis',
  wx: 'wx'
}
```

### `Inditrans.Option`

Transliteration options (can be combined using bitwise OR `|`):

```javascript
Inditrans.Option = {
  None: 0,                    // No special options
  TamilTraditional: 1,        // Use traditional Tamil consonants
  TamilSuperscripted: 2,      // Use superscripted Tamil consonants
  ASCIINumerals: 4,           // Force ASCII numerals
  IgnoreVedicAccents: 8,      // Ignore Vedic accents
  RetainSpecialMarkers: 16,   // Retain special markers
  NoXMLTagHandling: 32        // Do not handle XML tags
}
```

**Combining options:**

```javascript
// Use multiple options with bitwise OR
const options = Inditrans.Option.ASCIINumerals | Inditrans.Option.IgnoreVedicAccents;
```

## Examples

### Basic Usage

```javascript
Inditrans.ready().then(() => {
  // Devanagari to IAST
  const result1 = Inditrans.transliterate(
    'नमस्ते',
    'devanagari',
    'iast',
    0
  );
  console.log(result1); // "namaste"

  // ITRANS to Tamil
  const result2 = Inditrans.transliterate(
    'vanakkam',
    'itrans',
    'tamil',
    Inditrans.Option.None
  );
  console.log(result2); // "வநக்கம்"
});
```

### With Options

```javascript
Inditrans.ready().then(() => {
  const text = 'श्री गुरुभ्यो नमः १२३';

  // Without ASCII numerals
  const result1 = Inditrans.transliterate(
    text,
    Inditrans.Script.devanagari,
    Inditrans.Script.iast,
    Inditrans.Option.None
  );
  console.log(result1); // Includes Devanagari numerals

  // With ASCII numerals
  const result2 = Inditrans.transliterate(
    text,
    Inditrans.Script.devanagari,
    Inditrans.Script.iast,
    Inditrans.Option.ASCIINumerals
  );
  console.log(result2); // "... 123"
});
```

### Converting Between Multiple Scripts

```javascript
Inditrans.ready().then(() => {
  const text = 'namaste duniya';
  const scripts = ['devanagari', 'bengali', 'tamil', 'telugu', 'kannada'];

  scripts.forEach(script => {
    const result = Inditrans.transliterate(
      text,
      Inditrans.Script.itrans,
      script,
      Inditrans.Option.None
    );
    console.log(`${script}: ${result}`);
  });
});
```

### Interactive Example

See [standalone-example.html](standalone-example.html) for a complete interactive demo.

## Module System Support

The wrapper supports multiple module systems:

### Browser (Global)

```html
<script src="inditrans.js"></script>
<script src="inditrans-standalone.js"></script>
<script>
  Inditrans.ready().then(() => { /* ... */ });
</script>
```

### CommonJS (Node.js)

```javascript
const Inditrans = require('./inditrans-standalone.js');
// Note: inditrans.js must be loaded in the global scope first
```

### AMD (RequireJS)

```javascript
require(['inditrans-standalone'], function(Inditrans) {
  Inditrans.ready().then(() => { /* ... */ });
});
```

## Error Handling

Always wrap your calls in try-catch blocks:

```javascript
try {
  const result = Inditrans.transliterate(
    'नमस्ते',
    Inditrans.Script.devanagari,
    Inditrans.Script.iast,
    Inditrans.Option.None
  );
  console.log(result);
} catch (error) {
  console.error('Transliteration failed:', error.message);
}
```

## Testing

Open `standalone-example.html` in a browser to test the wrapper. Make sure both files are in the correct location:

- `public/inditrans.js`
- `inditrans-standalone.js`

## License

Same as the inditrans project.

## Need Help?

- Check the [main inditrans README](../README.md)
- See [standalone-example.html](standalone-example.html) for working examples
- Report issues on the project repository
