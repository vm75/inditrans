import fs from "node:fs";
import { performance } from "node:perf_hooks";
// Calls the standalone C ABI and includes releaseBuffer in each timed call.
// The 64 KiB ceiling also fits the perf-00-baseline engine's eager reader.
const artifact = process.argv[2];
if (!artifact)
  throw new Error("Usage: node wasm_bench.mjs artifact.wasm [case]");
const instance = new WebAssembly.Instance(
  new WebAssembly.Module(fs.readFileSync(artifact)),
  {},
);
const e = instance.exports;
if (e.__wasm_call_ctors) e.__wasm_call_ctors();
else if (e._initialize) e._initialize();
const seedIndic = "श्री॒ गुरुभ्यो नमः । नमस्ते भारतम् । ";
const cases = [
  ["devanagari", "telugu", "indic-to-indic", seedIndic],
  ["devanagari", "tamil", "tamil-output", seedIndic],
  [
    "iso",
    "devanagari",
    "latin-input",
    "śrī gurubhyo namaḥ, namaste bhāratam. ",
  ],
  ["indic", "iso", "virtual-indic-to-latin", seedIndic],
  ["indic", "devanagari", "virtual-indic-to-indic", seedIndic],
  ["indic", "tamil", "virtual-indic-to-tamil", seedIndic],
  ["devanagari", "iso", "latin-output", seedIndic],
  ["devanagari", "telugu", "expansion-heavy", "अॅॐऍ"],
  [
    "devanagari",
    "telugu",
    "protected-spans",
    "##a protected span with raw text नमस्ते and symbols 🙂 ##",
  ],
  [
    "devanagari",
    "telugu",
    "mixed-protected-spans",
    "नमस्ते ##a long protected raw span with text नमस्ते and symbols 🙂## भारतम् । ",
  ],
];
if (process.argv[3] && !cases.some((entry) => entry[2] === process.argv[3])) {
  throw new Error(`Unknown benchmark case: ${process.argv[3]}`);
}
const encode = (value) => {
  const bytes = Buffer.from(value + "\0");
  const ptr = e.malloc(bytes.length);
  if (!ptr) throw new Error("malloc failed");
  new Uint8Array(e.memory.buffer, ptr, bytes.length).set(bytes);
  return ptr;
};
const read = (ptr) => {
  if (!ptr) throw new Error("transliterate failed");
  const bytes = new Uint8Array(e.memory.buffer);
  let end = ptr;
  while (bytes[end]) ++end;
  return bytes.subarray(ptr, end);
};
console.log(
  "case,input_bytes,median_ns,sample_p95_ns,output_size_sink,output_fnv1a64",
);
for (const [from, to, label, seed] of cases) {
  if (process.argv[3] && process.argv[3] !== label) continue;
  for (const target of [32, 4096, 65536]) {
    const text = seed.repeat(Math.ceil(target / Buffer.byteLength(seed)));
    const inputBytes = Buffer.byteLength(text);
    const args = [text, from, to, "##", "##"].map(encode);
    const call = () =>
      e.transliterate(args[0], args[1], args[2], 0, args[3], args[4]);
    for (let i = 0; i < 32; ++i) e.releaseBuffer(call());
    const samples = [];
    const reps = target === 32 ? 2000 : target === 4096 ? 200 : 32;
    for (let sample = 0; sample < 31; ++sample) {
      const start = performance.now();
      for (let i = 0; i < reps; ++i) {
        const result = call();
        e.releaseBuffer(result);
      }
      samples.push(((performance.now() - start) * 1e6) / reps);
    }
    const result = call();
    const bytes = read(result);
    let hash = 14695981039346656037n;
    for (const byte of bytes)
      hash = ((hash ^ BigInt(byte)) * 1099511628211n) & 0xffffffffffffffffn;
    samples.sort((a, b) => a - b);
    console.log(
      [
        label,
        inputBytes,
        Math.round(samples[15]),
        Math.round(samples[29]),
        bytes.length * 31,
        hash.toString(),
      ].join(","),
    );
    e.releaseBuffer(result);
    args.forEach((ptr) => e.free(ptr));
  }
}
