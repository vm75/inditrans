#!/usr/bin/env node

const fs = require('node:fs');
const path = require('node:path');

const wasmPath = path.resolve(__dirname, '../../flutter/assets/inditrans.wasm');

if (!fs.existsSync(wasmPath)) {
  console.error(`Error: WASM file not found at ${wasmPath}`);
  process.exit(1);
}

const wasmBuffer = fs.readFileSync(wasmPath);
if (wasmBuffer.length === 0) {
  console.error(`Error: WASM file at ${wasmPath} is empty`);
  process.exit(1);
}

let wasmModule;
try {
  wasmModule = new WebAssembly.Module(wasmBuffer);
} catch (e) {
  console.error(`Error: Failed to compile WASM module: ${e.message}`);
  process.exit(1);
}

// 1. Verify no unexpected imports (specifically wasi_snapshot_preview1.proc_exit)
const imports = WebAssembly.Module.imports(wasmModule);
for (const imp of imports) {
  if (imp.module === 'wasi_snapshot_preview1' && imp.name === 'proc_exit') {
    console.error('Error: WASM unexpectedly imports wasi_snapshot_preview1.proc_exit');
    process.exit(1);
  }
}

// 2. Instantiate standalone WASM module
let instance;
try {
  instance = new WebAssembly.Instance(wasmModule, {});
} catch (e) {
  console.error(`Error: Failed to instantiate WASM module: ${e.message}`);
  process.exit(1);
}

const wasmExports = instance.exports;

// 3. Verify required exports exist for Flutter loader and C API
const requiredExports = [
  'memory',
  'malloc',
  'free',
  'transliterate',
  'transliterateUrduWithLexiconData',
  'isScriptSupported',
  'releaseBuffer',
];

for (const name of requiredExports) {
  if (typeof wasmExports[name] === 'undefined') {
    console.error(`Error: Missing required WASM export: ${name}`);
    process.exit(1);
  }
}

// 4. Perform static initialization if present (__wasm_call_ctors or _initialize)
if (typeof wasmExports.__wasm_call_ctors === 'function') {
  wasmExports.__wasm_call_ctors();
} else if (typeof wasmExports._initialize === 'function') {
  wasmExports._initialize();
}

// Helper functions for reading/writing UTF-8 strings
function writeUtf8(str) {
  const bytes = Buffer.from(str + '\0', 'utf8');
  const ptr = wasmExports.malloc(bytes.length);
  if (!ptr) {
    throw new Error('Failed to allocate memory in WASM module');
  }
  new Uint8Array(wasmExports.memory.buffer, ptr, bytes.length).set(bytes);
  return ptr;
}

function readUtf8(ptr) {
  if (!ptr) return '';
  const mem = new Uint8Array(wasmExports.memory.buffer, ptr);
  let len = 0;
  while (mem[len] !== 0) len++;
  return Buffer.from(mem.subarray(0, len)).toString('utf8');
}

// 5. Test isScriptSupported
const devanagariPtr = writeUtf8('devanagari');
const supported = wasmExports.isScriptSupported(devanagariPtr);
wasmExports.free(devanagariPtr);

if (supported !== 1) {
  console.error(`Error: isScriptSupported('devanagari') returned ${supported}, expected 1`);
  process.exit(1);
}

for (const [script, expected] of [['grantha', 1], ['tamilExtended', 0]]) {
  const pointer = writeUtf8(script);
  const actual = wasmExports.isScriptSupported(pointer);
  wasmExports.free(pointer);
  if (actual !== expected) {
    throw new Error(`isScriptSupported('${script}') returned ${actual}, expected ${expected}`);
  }
}

// 6. Test transliterate call: Harvard-Kyoto "namaste" to Devanagari -> "नमस्ते"
const textPtr = writeUtf8('namaste');
const fromPtr = writeUtf8('hk');
const toPtr = writeUtf8('devanagari');
const skipStartPtr = writeUtf8('##');
const skipEndPtr = writeUtf8('##');

const outPtr = wasmExports.transliterate(textPtr, fromPtr, toPtr, 0, skipStartPtr, skipEndPtr);
const result = readUtf8(outPtr);

wasmExports.releaseBuffer(outPtr);
wasmExports.free(textPtr);
wasmExports.free(fromPtr);
wasmExports.free(toPtr);
wasmExports.free(skipStartPtr);
wasmExports.free(skipEndPtr);

const expected = 'नमस्ते';
if (result !== expected) {
  console.error(`Error: Transliteration mismatch! Got '${result}', expected '${expected}'`);
  process.exit(1);
}

console.log(`✓ Standalone WASM smoke test passed (${wasmBuffer.length} bytes, ${imports.length} imports, result: '${result}')`);

const lexicon = fs.readFileSync(path.resolve(__dirname, '../../test-files/urdu-test-lexicon.iul'));
const lexiconPtr = wasmExports.malloc(lexicon.length);
new Uint8Array(wasmExports.memory.buffer, lexiconPtr, lexicon.length).set(lexicon);
const urduArgs = ['کتاب', 'iast', '##', '##'].map(writeUtf8);
const urduResult = wasmExports.transliterateUrduWithLexiconData(
  urduArgs[0], urduArgs[1], 0, lexiconPtr, lexicon.length, urduArgs[2], urduArgs[3]);
if (readUtf8(urduResult) !== 'kitāb') throw new Error('Urdu pronunciation pack conversion failed');
wasmExports.releaseBuffer(urduResult);
wasmExports.free(lexiconPtr);
urduArgs.forEach(ptr => wasmExports.free(ptr));

// Exercise flags and supplementary-plane script text against the bundled engine.
for (const [text, from, to, options, expected] of [
  ['क॒ क॑ क᳚', 'devanagari', 'slp1', 0, 'ka ka ka'],
  ['क॒ क॑ क᳚', 'devanagari', 'wx', 0, 'ka ka ka'],
  ['नमस्ते', 'devanagari', 'grantha', 0, '𑌨𑌮𑌸𑍍𑌤𑍇'],
  ['अ॒ग्निमी॑ळे॒', 'devanagari', 'grantha', 0, '𑌅𑌗𑍍𑌨𑌿𑌮𑍀𑌳𑍇'],
  ['𑌨𑌮𑌸𑍍𑌤𑍇', 'grantha', 'devanagari', 0, 'नमस्ते'],
  ['𑌕𑍋', 'grantha', 'iso', 0, 'kō'],
  ['𑌕𑍍𑌷', 'indic', 'devanagari', 0, 'क्ष'],
  ['अंक', 'devanagari', 'tamil', 64, 'அம்க'],
  ['அக¹', 'tamil', 'devanagari', 64, 'अक'],
  ['அந', 'TAMIL', 'tamil', 2, 'அந'],
  ['अंक', 'devanagari', 'indlish', 0, 'anka'],
  ['अंक', 'devanagari', 'indlish', 64, 'aṁka'],
  ['अंट', 'devanagari', 'indlish', 0, 'aNTa'],
  ['अंट', 'devanagari', 'indlish', 64, 'aṁTa'],
]) {
  const pointers = [text, from, to, '##', '##'].map(writeUtf8);
  const output = wasmExports.transliterate(pointers[0], pointers[1], pointers[2], options, pointers[3], pointers[4]);
  const actual = readUtf8(output);
  wasmExports.releaseBuffer(output);
  pointers.forEach(ptr => wasmExports.free(ptr));
  if (actual !== expected) {
    console.error(`Error: ${from} -> ${to} with options ${options}: '${actual}', expected '${expected}'`);
    process.exit(1);
  }
}
console.log('✓ Standalone WASM Grantha and inference options passed');
