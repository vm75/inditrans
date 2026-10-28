// Exercise the shipped optional IUL1 demo pack against the checked-in WASM.
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const { test } = require('node:test');
const assert = require('node:assert/strict');

const root = join(__dirname, '../..');
const wasm = new WebAssembly.Instance(new WebAssembly.Module(
  readFileSync(join(root, 'flutter/assets/inditrans.wasm'))), {}).exports;
wasm.__wasm_call_ctors?.();
const encoder = new TextEncoder(), decoder = new TextDecoder();

function alloc(bytes) {
  const ptr = wasm.malloc(bytes.length);
  assert.ok(ptr);
  new Uint8Array(wasm.memory.buffer, ptr, bytes.length).set(bytes);
  return ptr;
}
function word(text) { return alloc(encoder.encode(text + '\0')); }
function take(ptr) {
  assert.ok(ptr);
  const bytes = new Uint8Array(wasm.memory.buffer);
  let end = ptr;
  while (bytes[end]) end++;
  const value = decoder.decode(bytes.subarray(ptr, end));
  wasm.releaseBuffer(ptr);
  return value;
}
function ordinary(text, from, to, flags = 0) {
  const inputs = [text, from, to, '##', '##'].map(word);
  try {
    return take(wasm.transliterate(inputs[0], inputs[1], inputs[2], flags, inputs[3], inputs[4]));
  } finally { inputs.forEach(wasm.free); }
}
function assisted(text, to, handle, flags = 0) {
  const inputs = [text, to, '##', '##'].map(word);
  try {
    return take(wasm.transliterateUrduWithLexiconHandle(inputs[0], inputs[1], flags,
      handle, inputs[2], inputs[3]));
  } finally { inputs.forEach(wasm.free); }
}
test('example Urdu proverb receives lexical vowels only with the optional pack', () => {
  const bytes = readFileSync(join(root, 'demo/urdu-example.iul'));
  assert.ok(bytes.length < 3000000);
  const raw = alloc(bytes);
  const handle = wasm.createUrduLexiconHandle(raw, bytes.length);
  wasm.free(raw);
  assert.ok(handle, 'IUL1 demo pack must be valid');
  try {
    const source = 'جیسی کرنی ویسی بھرنی';
    const expected = 'jaisI karnI vaisI bharnI';
    assert.equal(ordinary('जैसी कर्नी वैसी भर्नी', 'devanagari', 'indlish'), expected);
    assert.equal(assisted(source, 'indlish', handle), expected);
    assert.notEqual(ordinary(source, 'urdu', 'indlish'), expected);
    assert.equal(assisted(source + '۔', 'indlish', handle), expected + ordinary('۔', 'urdu', 'indlish'));
    assert.equal(assisted(source, 'indlish', handle, 64), ordinary(source, 'urdu', 'indlish', 64));
    assert.equal(assisted('قلم', 'indlish', handle), ordinary('قلم', 'urdu', 'indlish'));
    assert.equal(assisted('##جیسی## جیسی', 'indlish', handle), 'جیسی jaisI');
  } finally { wasm.releaseUrduLexiconHandle(handle); }
});

test('full experimental Urdu lexicon loads through the runtime and retains conservative fallback', () => {
  const pack = readFileSync(join(root, 'demo/urdu-pronunciation.iul'));
  const report = JSON.parse(readFileSync(join(root, 'scripts/urdu/concordant-report.json'), 'utf8'));
  assert.ok(pack.length <= 3000000);
  assert.equal(report.packed_entries, 4252);
  assert.equal(pack.length, report.pack_bytes);
  const raw = alloc(pack);
  const handle = wasm.createUrduLexiconHandle(raw, pack.length);
  wasm.free(raw);
  assert.ok(handle, 'full experimental Urdu pack must be accepted by native binary parser');
  try {
    assert.equal(assisted('جیسی کرنی ویسی بھرنی', 'indlish', handle), 'jaisI karnI vaisI bharnI');
    assert.equal(assisted('قلم', 'indlish', handle), ordinary('قلم', 'urdu', 'indlish'));
    assert.equal(assisted('##جیسی## جیسی', 'indlish', handle), 'جیسی jaisI');
  } finally { wasm.releaseUrduLexiconHandle(handle); }
});
