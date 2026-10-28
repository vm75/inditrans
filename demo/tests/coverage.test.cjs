// Check the shipped engine, rather than assuming a table entry is a readable key.
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const { runInNewContext } = require('node:vm');
const { test } = require('node:test');
const assert = require('node:assert/strict');
const data = readFileSync(join(__dirname, '../keyboard-data.js'), 'utf8');
const html = readFileSync(join(__dirname, '../index.html'), 'utf8');
const accentCode = html.slice(html.indexOf('function keyboardAccents'), html.indexOf('// Font dev tools:'));
const { SCRIPT_CATALOG, KEYBOARD_MAPS, nativeKeyboardValues, nativeKeyboardExtras, keyboardAccents } =
  runInNewContext(data + accentCode + ';({SCRIPT_CATALOG, KEYBOARD_MAPS, nativeKeyboardValues, nativeKeyboardExtras, keyboardAccents})');
const wasm = new WebAssembly.Instance(new WebAssembly.Module(
  readFileSync(join(__dirname, '../../flutter/assets/inditrans.wasm'))), {}).exports;
wasm.__wasm_call_ctors?.();
const encoder = new TextEncoder();
const decoder = new TextDecoder();
function convert(text, source) {
  const pointers = [text, source, 'iast', '##', '##'].map(value => {
    const bytes = encoder.encode(value + '\0');
    const ptr = wasm.malloc(bytes.length);
    new Uint8Array(wasm.memory.buffer).set(bytes, ptr);
    return ptr;
  });
  const ptr = wasm.transliterate(...pointers.slice(0, 3), 8, ...pointers.slice(3));
  assert.ok(ptr, source);
  const bytes = new Uint8Array(wasm.memory.buffer);
  let end = ptr;
  while (bytes[end]) end++;
  const result = decoder.decode(bytes.subarray(ptr, end));
  wasm.releaseBuffer(ptr);
  pointers.forEach(ptr => wasm.free(ptr));
  return result;
}
const nativeCharacters = /[\p{Script=Devanagari}\p{Script=Bengali}\p{Script=Gujarati}\p{Script=Gurmukhi}\p{Script=Kannada}\p{Script=Malayalam}\p{Script=Oriya}\p{Script=Sinhala}\p{Script=Tamil}\p{Script=Telugu}\p{Script=Grantha}\p{Script=Khmer}\p{Script=Myanmar}\p{Script=Thai}\p{Script=Tibetan}\p{Script=Arabic}]/u;
function check(source, text, label) {
  const output = convert(text, source);
  assert.ok(!nativeCharacters.test(output), `${source} ${label}: ${JSON.stringify(text)} -> ${JSON.stringify(output)}`);
}
test('all bundled samples transliterate without unsupported native characters', () => {
  for (const [source, entry] of Object.entries(SCRIPT_CATALOG)) {
    if (!entry.sample || entry.source === false) continue;
    // Protected spans and markup intentionally preserve native characters.
    check(source, entry.sample.replace(/##[\s\S]*?##/g, '').replace(/<[^>]*>/g, ''), 'sample');
  }
});
test('all visible native keyboard keys are readable by the shipped engine', () => {
  for (const [source, entry] of Object.entries(SCRIPT_CATALOG)) {
    if (!['indic', 'persoArabic'].includes(entry.kind)) continue;
    const layout = KEYBOARD_MAPS[source];
    for (const group of ['vowels', 'vowelMarks', 'consonants', 'otherDiacritics', 'symbols']) {
      const values = group === 'symbols' ? [...layout.symbols, ...layout.vedicSymbols] : layout[group];
      for (const value of nativeKeyboardValues(source, group, values).filter(Boolean)) check(source, value, group);
    }
    for (const { value } of nativeKeyboardExtras(source)) check(source, value, 'extra');
    for (const value of keyboardAccents(source)) check(source, value, 'accent');
  }
});
