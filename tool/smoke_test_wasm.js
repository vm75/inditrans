#!/usr/bin/env node

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const wasmPath = path.resolve(__dirname, '../flutter/assets/inditrans.wasm');

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

const exports = instance.exports;

// 3. Verify required exports exist for Flutter loader and C API
const requiredExports = [
  'memory',
  'malloc',
  'free',
  'transliterate',
  'isScriptSupported',
  'releaseBuffer',
];

for (const name of requiredExports) {
  if (typeof exports[name] === 'undefined') {
    console.error(`Error: Missing required WASM export: ${name}`);
    process.exit(1);
  }
}

// 4. Perform static initialization if present (__wasm_call_ctors or _initialize)
if (typeof exports.__wasm_call_ctors === 'function') {
  exports.__wasm_call_ctors();
} else if (typeof exports._initialize === 'function') {
  exports._initialize();
}

// Helper functions for reading/writing UTF-8 strings
function writeUtf8(str) {
  const bytes = Buffer.from(str + '\0', 'utf8');
  const ptr = exports.malloc(bytes.length);
  if (!ptr) {
    throw new Error('Failed to allocate memory in WASM module');
  }
  new Uint8Array(exports.memory.buffer, ptr, bytes.length).set(bytes);
  return ptr;
}

function readUtf8(ptr) {
  if (!ptr) return '';
  const mem = new Uint8Array(exports.memory.buffer, ptr);
  let len = 0;
  while (mem[len] !== 0) len++;
  return Buffer.from(mem.subarray(0, len)).toString('utf8');
}

// 5. Test isScriptSupported
const devanagariPtr = writeUtf8('devanagari');
const supported = exports.isScriptSupported(devanagariPtr);
exports.free(devanagariPtr);

if (supported !== 1) {
  console.error(`Error: isScriptSupported('devanagari') returned ${supported}, expected 1`);
  process.exit(1);
}

// 6. Test transliterate call: Harvard-Kyoto "namaste" to Devanagari -> "नमस्ते"
const textPtr = writeUtf8('namaste');
const fromPtr = writeUtf8('hk');
const toPtr = writeUtf8('devanagari');
const skipStartPtr = writeUtf8('##');
const skipEndPtr = writeUtf8('##');

const outPtr = exports.transliterate(textPtr, fromPtr, toPtr, 0, skipStartPtr, skipEndPtr);
const result = readUtf8(outPtr);

exports.releaseBuffer(outPtr);
exports.free(textPtr);
exports.free(fromPtr);
exports.free(toPtr);
exports.free(skipStartPtr);
exports.free(skipEndPtr);

const expected = 'नमस्ते';
if (result !== expected) {
  console.error(`Error: Transliteration mismatch! Got '${result}', expected '${expected}'`);
  process.exit(1);
}

console.log(`✓ Standalone WASM smoke test passed (${wasmBuffer.length} bytes, ${imports.length} imports, result: '${result}')`);
