# Repository tools

Repository maintenance commands use Python 3 and the standard library. Run
root-level commands from the repository root:

```sh
python3 tool/generate_headers.py
python3 tool/generate_bindings.py
python3 tool/generate_test_data.py
python3 tool/import_script.py indic newScript
python3 tool/bump_version.py patch --log "Describe the release change"
python3 tool/verify_release.py --tag=v0.13.0
python3 tool/verify_wasm_build_parity.py
```

`generate_headers.py` reads `script_data.json`, `reader_data.json`,
`options.json`, and `docs/extended-latin.txt`, then updates generated C++, Dart,
TypeScript, and JavaScript declarations. The shared generator code lives in
`python/`; its ordering and output formatting are deliberate because generated
files are checked in and trie IDs depend on deterministic input order.

`generate_bindings.py` parses the small exported C ABI in
`native/src/exports.h`. Its type map is intentionally limited to the ABI's
current types. Extend the parser and mapping when that header adds a new kind
of argument or return value. The script locates the repository from its own
path, so it can also be run from `flutter/` as documented in that package.

`verify_wasm_build_parity.py` checks that the Unix and PowerShell release
commands use matching compiler/linker flags and artifact settings for both
Wasm targets. It is part of `make validate` and normal CI; debug commands are
allowed to differ.

The two upstream-data commands fetch their source JSON over HTTPS. The other
commands run offline. Flutter's own analyzer, package manager, and tests still
use the Dart SDK because they operate on the Flutter package rather than on
repository maintenance scripts.

`node tool/wasm_bench.mjs flutter/assets/inditrans.wasm [case]` measures the
standalone C ABI on the ten native throughput workloads, at short, 4 KiB, and
64 KiB scales. CSV output includes median/p95 timing, output sizes, and FNV-1a
hashes. Timed calls include result release; UTF-8 decoding and hashing happen
outside timing. Use the same Node version and CPU for both revisions; the
64 KiB ceiling lets the eager reader in `perf-00-baseline` fit its original
standalone memory limit.
