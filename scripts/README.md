# Repository scripts

Repository maintenance commands use Python 3 and the standard library. Run
root-level commands from the repository root:

```sh
python3 scripts/generate_headers.py
python3 scripts/generate_bindings.py
python3 scripts/generate_test_data.py
python3 scripts/import_script.py indic newScript
python3 scripts/bump_version.py patch --log "Describe the release change"
python3 scripts/verify_release.py --tag=v0.13.0
python3 scripts/verify_wasm_build_parity.py
```

## Test organization

Tests of these commands live in `scripts/tests/`. Run them with `make test_scripts`
or `python3 -m unittest discover -s scripts/tests`. They also run in `make validate`,
`make testall`, and CI.

Component tests stay with their components: `demo/tests/`, `native/tests/`,
`flutter/test/`, and `nodejs/test/`. Shared artifact smoke tests live in
`tests/integration/`; shared transliteration cases remain in `test-files/`.

```sh
make test_demo
make test_wasm
python3 tests/integration/browser_smoke.py --browser chromium
```

`scripts/verify_dll.sh` uses `tests/integration/windows_smoke.cpp` for its optional
Wine runtime check. Maintenance and verification commands themselves stay here.

`generate_demo_font.py` and `verify_demo_font.py` maintain and check the demo's
bundled Indic fallback font. Their optional dependencies are pinned inline and
isolated by `uv run`; see [font sources and reproduction](../demo/fonts/README.md).
They are not needed to serve the demo or run the standard-library script tests.

## Generation and validation

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

`node scripts/wasm_bench.mjs flutter/assets/inditrans.wasm [case]` measures the
standalone C ABI on the ten native throughput workloads, at short, 4 KiB, and
64 KiB scales. CSV output includes median/p95 timing, output sizes, and FNV-1a
hashes. Timed calls include result release; UTF-8 decoding and hashing happen
outside timing. Use the same Node version and CPU for both revisions; the
64 KiB ceiling lets the eager reader in `perf-00-baseline` fit its original
standalone memory limit.
