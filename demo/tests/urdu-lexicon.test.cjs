// Exercise explicit and bundled pronunciation packs against the checked-in WASM.
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
test('explicit Urdu overrides preserve fixture readings independently of the bundled wordlist', () => {
  const bytes = readFileSync(join(root, 'demo/urdu-example.iul'));
  const raw = alloc(bytes);
  const handle = wasm.createUrduLexiconHandle(raw, bytes.length);
  wasm.free(raw);
  assert.ok(handle, 'compiled demo pack must be valid');
  try {
    const source = 'جیسی کرنی ویسی بھرنی';
    const expected = 'jaisI karnI vaisI bharnI';
    assert.equal(ordinary('जैसी कर्नी वैसी भर्नी', 'devanagari', 'indlish'), expected);
    assert.equal(assisted(source, 'indlish', handle), expected);
    const bundled = ordinary(source, 'urdu', 'indlish');
    assert.equal(ordinary(source + '۔', 'urdu', 'indlish'), bundled + ordinary('۔', 'urdu', 'indlish'));
    assert.notEqual(ordinary(source, 'urdu', 'indlish', 64), expected);
    assert.equal(assisted(source + '۔', 'indlish', handle), expected + ordinary('۔', 'urdu', 'indlish'));
    assert.equal(assisted(source, 'indlish', handle, 64), ordinary(source, 'urdu', 'indlish', 64));
    assert.equal(assisted('زززززززززززز', 'indlish', handle), ordinary('زززززززززززز', 'urdu', 'indlish'));
    assert.equal(assisted('##جیسی## جیسی', 'indlish', handle), 'جیسی jaisI');
  } finally { wasm.releaseUrduLexiconHandle(handle); }
});

test('full experimental Urdu lexicon loads through the runtime and retains conservative fallback', () => {
  const pack = readFileSync(join(root, 'lexicons/urdu.iul'));
  const report = JSON.parse(readFileSync(join(root, 'lexicons/urdu.json'), 'utf8'));
  assert.ok(pack.length > 100000);
  assert.ok(report.entries > 4252);
  assert.equal(report.script, 'urdu');
  assert.equal(report.wordlist.name, 'wordlist.tsv');
  assert.equal(pack.length, report.pack_bytes);
  const raw = alloc(pack);
  const handle = wasm.createUrduLexiconHandle(raw, pack.length);
  wasm.free(raw);
  assert.ok(handle, 'full experimental Urdu pack must be accepted by native binary parser');
  try {
    assert.equal(assisted('جیسی کرنی ویسی بھرنی', 'indlish', handle), ordinary('جیسی کرنی ویسی بھرنی', 'urdu', 'indlish'));
    assert.equal(ordinary('قلم', 'urdu', 'indlish'), 'qalam');
    assert.equal(ordinary('پل', 'urdu', 'indlish'), assisted('پل', 'indlish', handle));
    assert.equal(ordinary('پل', 'urdu', 'indlish', 64), 'pl');
    assert.equal(assisted('زززززززززززز', 'indlish', handle), ordinary('زززززززززززز', 'urdu', 'indlish'));
    assert.equal(assisted('##جیسی## جیسی', 'indlish', handle), ordinary('##جیسی## جیسی', 'urdu', 'indlish'));
  } finally { wasm.releaseUrduLexiconHandle(handle); }
});
