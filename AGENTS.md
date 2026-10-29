# Inditrans — Agent Guide

## Project overview

`inditrans` is a monorepo containing a C++23 transliteration engine and two distribution
packages: a Flutter FFI plugin (`flutter/`) and a TypeScript/JavaScript package (`nodejs/`).
The engine (`native/src/`) is compiled to a shared library for native platforms and
to WASM for Web and Node.js. All distributions share the same test-case suite and script data.

## Repository map

```
native/src/         C++23 engine: inditrans.cpp, exports.h, script_data.h, …
native/cli/         Standalone CLI tool (same C++ source)
native/tests/       C++ engine test suite
flutter/assets/     inditrans.wasm — WASM binary for Flutter Web
flutter/lib/            Dart public API (inditrans.dart, src/script.dart, src/option.dart)
flutter/lib/src/bindings.dart   Auto-generated FFI bindings — do not edit by hand
flutter/test/           Dart test suite
flutter/example/        Flutter example app
flutter/example.dart/   Vanilla Dart (non-Flutter) example
nodejs/src/             TypeScript API (index.ts, Script.ts, Option.ts)
nodejs/test/            Node.js/Jest test suite
js/                     WASM JS build consumed by nodejs/
scripts/                Build, release, and maintenance commands; JSON files are canonical inputs
wordlists/<script>/     One native→IPA wordlist.tsv per source script
lexicons/               Compiled pronunciation packs and technical reports
scripts/python/         Shared code-generation logic for script, option, and trie tables
scripts/reader_data.json Canonical accents, exclusive symbols, Tamil prefixes
scripts/tests/          Regression tests for maintenance commands
demo/tests/             Browser demo component tests (Node.js)
tests/integration/      Browser, standalone WASM, and Windows DLL smoke tests
test-files/             Shared test-case JSON used by all distributions
Makefile                Top-level build, test, and publish targets
```

## Working commands

### Whole-repo
- **Build everything**: `make all`
- **Build Windows DLL**: `make dll` (or `make windows`; cross-compiles Windows x86-64 DLL using MinGW-w64)
- **Run C++ tests**: `make test`
- **Run native performance benchmark**: `make bench`
- **Run isolated shared-reader lookup benchmark**: `make bench-lookup`
- **Probe lookup metadata allocations (Linux/glibc)**: `make bench-lookup-alloc-linux`
- **Compile pronunciation lexicons**: `make lexicons`
- **Check lexicon reproducibility**: `make check-lexicons`
- **Regenerate script/reader data and enums**: `python3 scripts/generate_headers.py`
- **Run per-call latency benchmark**: `make bench-short` (includes returned-string destruction)
- **Repeat short-call benchmark for stable latency claims**: `make bench-short-repeat` (5 full runs by default; override with `PERF_REPEATS`; optional Linux pinning with `PERF_CPU`)
- **Measure first-call latency in fresh processes**: `make bench-cold` (four representative paths, 501 samples each by default; override with `COLD_BENCH_SAMPLES`; optional Linux pinning with `PERF_CPU`)
- **Run output-size/expansion benchmark**: `make output-size-bench` (`make mem-bench` is a compatibility alias)
- **Run Linux/glibc allocation probe**: `make bench-alloc-linux`
- **Run all available benchmarks with a formatted summary**: `make bench-all`
- **Save a performance baseline**: `make bench-save` (writes `out/bench-baseline-*.csv`)
- **Compare current results against saved baseline**: `make bench-compare` (flags regressions ≥ 5%; output mismatches fail)
- **Capture a cumulative tuning snapshot**: `make perf-snapshot PERF_NAME=00-baseline`
- **Run historical and current benchmarks**: `python3 scripts/run-perf.py --run-tags` (uses persistent sparse worktrees under `out/perf/worktrees/` and current harness; specify tags e.g. `python3 scripts/run-perf.py perf-00-baseline --current`)
- **Generate the cumulative tuning report**: `make perf-report`
- **Make performance regressions fail**: `BENCH_STRICT=1 make bench-compare`
- **Run all tests**: `make testall`
- **Run demo keyboard and session regressions**: `make test_demo` (Node.js; no browser or dependencies required)
- **Run maintenance script regressions**: `make test_scripts` (Python standard library)
- **Validate release/version**: `make validate`
- **Publish all**: `make publish` (local manual fallback; prefer tag-based CI release)

#### Performance regression testing

Before making a change that might affect performance:
```bash
make bench-save                    # capture baseline at current commit
# … make changes …
make bench-compare                 # report metric changes and verify output hashes
BENCH_STRICT=1 make bench-compare  # use for a CI-style performance gate
```
`bench-compare` validates the FNV-1a output hash for every throughput and short-call case. A hash mismatch or missing case always fails. By default, performance regressions are reported but do not make the command fail; `BENCH_STRICT=1` makes regressions at or above the threshold fail. Override the default 5% threshold with `BENCH_REGRESSION_THRESHOLD=<percent>`.

On Linux with glibc, the allocation baseline includes `malloc + calloc + realloc` event counts, total requested allocation bytes, peak requested live bytes, and live bytes after the measured call. `*-allocs.csv` is the cold/first-call measurement with no warmup; `*-allocs-warm.csv` repeats the probe after one unmeasured warmup to isolate recurring work. On other platforms, allocation instrumentation is skipped while throughput and latency comparisons still run.

The baseline path can be overridden:
```bash
make bench-save    BENCH_BASELINE=out/bench-before-pr42
make bench-compare BENCH_BASELINE=out/bench-before-pr42
```
The `out/bench-baseline-*.csv` files should not be committed; they are local measurement artefacts.

The PR performance gate compares against `perf-00-baseline` with the current
benchmark harness and unchanged tagged engine sources. The previous baseline
is archived as `perf-00-baseline-pre-0.15.0`. Use the main tag for
architecture comparisons; see `docs/performance-reassessment.md` for local evidence.

For cumulative tuning progress, use numbered snapshots under `out/perf/`:
```bash
# Automated worktree-based snapshotting across historical tags and current revision:
python3 scripts/run-perf.py perf-00-baseline --current
# Or manual snapshot capture in the active workspace:
make perf-snapshot PERF_NAME=00-baseline
make perf-snapshot PERF_NAME=01-buffer-ownership
make perf-report
```
`scripts/run-perf.py` measures historical tags using persistent sparse Git worktrees
under `out/perf/worktrees/` without checking out or modifying the primary working tree.
Worktrees isolate `native/src/` and are reused across invocations; clean them up manually
via `git worktree remove out/perf/worktrees/<tag>` (or `git worktree prune` if `out/` was removed).
`perf-snapshot` reuses `bench-save`, captures the cold-start paths, force-rebuilds the standalone Wasm artifact, and records its raw byte size. `perf-report` compares every discovered numbered snapshot with `00-baseline`, distinguishes cold from warm allocation metrics, and treats `peak heap` as allocator-probe peak requested live bytes rather than process RSS. Use `bench-short-repeat` when making latency claims; a single snapshot timing is indicative only. See `docs/performance.md` for the baseline procedure and `docs/performance-project.md` for completed milestones and accepted trade-offs.

### Flutter (`flutter/` directory)
- **Get dependencies**: `flutter pub get`
- **Lint / analyze**: `dart analyze`
- **Test**: `flutter test` (native library must be built first)
- **Regenerate FFI bindings**: `python3 ../scripts/generate_bindings.py` (from `flutter/`)
- **Build native (Linux)**: `cmake -B native/build_linux native/src && cmake --build native/build_linux`
- **Cross-compile native (Windows)**: `cmake -S native/src -B native/build_win -DCMAKE_TOOLCHAIN_FILE=$(pwd)/scripts/cmake/mingw64.cmake && cmake --build native/build_win`
- **Publish**: `flutter pub publish`

### Node.js (`nodejs/` directory)
- **Install dependencies**: `yarn`
- **Build and test**: `yarn test`
- **Publish**: `npm publish --access=public`

## Engineering constraints

- C++23 is the required project language standard. Toolchains must meet minimums:
  CMake ≥ 3.20, Clang/LLVM ≥ 17 (or C++23-capable compiler), Emscripten 6.0.10, Node.js ≥ 18.3.
- Windows `inditrans.dll` (x86-64) is cross-compiled from Linux using MinGW-w64 (`x86_64-w64-mingw32-g++`)
  via `scripts/cmake/mingw64.cmake` (`make dll`). Requires C++23 (MinGW GCC ≥ 13).
- The public DLL ABI is strictly a C interface (`extern "C"`) exporting `transliterate`,
  `transliterateUrduWithLexiconData`, `isScriptSupported`, and `releaseBuffer`. No C++ ABI types (`std::string`, STL containers, exceptions)
  cross the boundary, guaranteeing compatibility across compilers (MinGW, MSVC) and Dart FFI.
- The Windows DLL is linked with `-static -static-libgcc -static-libstdc++` to eliminate MinGW
  support-runtime DLL dependencies (`libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`).
- `flutter/lib/src/bindings.dart` is generated from `native/src/exports.h` by
  `scripts/generate_bindings.py` — edit the header, then regenerate; never hand-edit the bindings file.
- `flutter/assets/inditrans.wasm` and `js/public/inditrans.js` must be rebuilt from C++
  source whenever the native API changes; they are not auto-generated by `flutter build`.
- `nodejs/src/Script.ts` and `nodejs/src/Option.ts` are generated from `scripts/`; do not
  hand-edit them.
- `native/src/script_data.h` is generated from `scripts/script_data.json` and
  `scripts/reader_data.json`. Readers/writers and Tamil prefixes must remain immutable,
  with no runtime builders, caches, equivalent parsing, or writer-table binding.
- Canonical script families are `indic`, `roman`, and `persoArabic`. Each script
  declares `supportsVedicAccents`; Tamil remains in `indic` with
  `processingType: "tamil"` to preserve its dedicated engine handlers.
- `Script.indlish` and `Script.wx` are write-only (cannot be used as `from`).
  `Script.indic` is read-only (cannot be used as `to`). Enforced in Dart (`_readNotSupported` / `_writeNotSupported`) and by the native engine (which rejects `indlish` as a source reader and `indic` as a target writer).
- The Git tag (`vMAJOR.MINOR.PATCH`) is the sole release authority; package
  manifests and build configs are kept in sync via `python3 scripts/bump_version.py`.
- The single consolidated changelog is authoritative in root `CHANGELOG.md`.
- Before a Flutter package publish/dry-run, run `python3 scripts/prepare_flutter_package.py` to bundle canonical `native/src/` into Git-ignored `flutter/src/` (included by `.pubignore`) and copy the changelog. `verify_release.py --check-packages` does this and runs `tests/integration/flutter_package_smoke.py` to build outside the monorepo. Platform builds use canonical sources in a checkout and bundled sources in installed packages.
- Releases are tag-based (`vX.Y.Z`), validated by `scripts/verify_release.py`, and
  published via `.github/workflows/release.yml` with pub.dev OIDC and npm trusted publishing.
- For pub.dev, `dart-lang/setup-dart@v1` must initialize OIDC credentials even
  when Flutter is installed with `subosito/flutter-action`. For npm, do not set
  `registry-url` in `actions/setup-node` in the Yarn-based release job: its
  token-placeholder `.npmrc` breaks dependency installation without a token.
- In the release build job, build native libraries before Emscripten setup to avoid
  its `cmake/` directory interfering with native build command lookup.
- Windows DLL cross-build and validation run in `.github/workflows/ci.yml` for
  PRs/main (Wine runtime smoke tests on main); keep MinGW/Wine out of release jobs.
- The normal `.github/workflows/ci.yml` workflow runs fast Linux-based checks for
  pull requests targeting `main` and pushes to `main` (skipping doc-only changes);
  multi-platform acceptance (macOS, Windows MSVC, Android) is isolated in
  `.github/workflows/platform.yml` on `platform-*` tags or manual dispatch;
  release tags are handled by the separate release workflow.
- Pages stages `demo/` with the matching checkout's `js/` engine assets. Rebuild
  them together when output conventions change; see `demo/README.md`.
- The shared test-case suite is at `test-files/test-cases.json`; changes to the engine that
  affect expected output must be reflected there.
- Follow KISS and YAGNI: prefer the smallest change that satisfies the request.
- Preserve existing public API behaviour unless the task explicitly requires a breaking change.

## Context discipline

- Start with targeted search and this repository map.
- Read only files relevant to the task; do not load `build/`, `dist/`, `.dart_tool/`,
  `node_modules/`, or generated outputs.
- Follow linked documentation for subsystem details rather than broad file sweeps.

## Documentation routing

- GitHub project overview: [`README.md`](README.md)
- Flutter package usage: [`flutter/README.md`](flutter/README.md)
- Node.js package usage: [`nodejs/README.md`](nodejs/README.md)
- Browser demo usage and tests: [`demo/README.md`](demo/README.md)
- Lexicon input contract and runtime: [`docs/lexicons.md`](docs/lexicons.md)
- Maintenance scripts and test organization: [`scripts/README.md`](scripts/README.md)
- Engine internals, pipeline, data structures: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Performance baseline and reproduction: [`docs/performance.md`](docs/performance.md)
- Performance project completion record: [`docs/performance-project.md`](docs/performance-project.md)
- Platform and toolchain support: [`docs/platform-support.md`](docs/platform-support.md)
- Consolidated changelog: [`CHANGELOG.md`](CHANGELOG.md)
- Release process: [`docs/release.md`](docs/release.md)

## Definition of done

- `dart analyze` passes with no new errors or warnings (for Flutter changes).
- Relevant tests pass: `make test` (C++), `flutter test` (Dart), `yarn test` (Node.js).
- `python3 scripts/verify_release.py` (`make validate`) passes.
- If the native API changed: `bindings.dart` regenerated, WASM rebuilt, `Script.ts`/`Option.ts`
  regenerated.
- No unrelated files or dependencies were changed.
- Only documentation made inaccurate by the change was updated.

## Documentation maintenance

| Change | Documents to update |
|---|---|
| Flutter API, installation, usage | `flutter/README.md` |
| Node.js API, installation, usage | `nodejs/README.md` |
| GitHub project overview, repo layout | `README.md` |
| Agent commands, constraints, routing | `AGENTS.md` |
| Native API (`exports.h`) | regenerate `bindings.dart`, WASM, `Script.ts`/`Option.ts` |
| Engine pipeline, data structures, invariants | `ARCHITECTURE.md` |
| Release process or CI/CD workflow | `docs/release.md`, `AGENTS.md` |
| New user-visible release | `CHANGELOG.md`, bump versions via `scripts/bump_version.py` |
