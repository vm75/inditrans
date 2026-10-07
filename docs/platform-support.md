# Platform and toolchain support

This matrix combines package-declared platforms with the build paths in the
repository. A configured CI job is not evidence of acceptance until its run is
green. Local results below are from the final direct-access engine sources;
hosted jobs added in this change are awaiting execution.

## Declared toolchain requirements

- Native engine: CMake 3.20 or later and a compiler with C++23 support.
  Repository guidance lists Clang 17+ and MinGW-w64 GCC 13+ for the Windows
  cross-build. Native CMake explicitly requires C++23.
- WebAssembly: Emscripten 6.0.10, selected with `-std=c++23` in both release
  build paths.
- Flutter: Android, iOS, Linux, macOS, Windows, and Web are declared in
  `flutter/pubspec.yaml`. The Dart SDK constraint is `^3.10.8`; the effective
  minimum is Flutter 3.38.10, which supplies the required Dart SDK and includes
  the macOS FFI plugin framework fix. CI pins Flutter 3.47.5 / Dart 3.13.4.
- Node.js: Node 18.3 or later; the release build uses Emscripten 6.0.10.
- Android uses CMake 3.22.1 and the pinned Flutter 3.47.5 NDK default
  `28.2.13676358`; CI records this value from Flutter's Gradle extension.
- Apple plugin podspecs select C++23 and retain their current deployment
  targets (iOS 9.0, macOS 10.11).

## Acceptance evidence

| Platform/path | Toolchain and C++23 selection | Build | Tests/runtime | Status |
|---|---|---|---|---|
| Linux native | Local CMake 4.4.4 + GCC 16.2.1; CMake project selects C++23. Native tests compiled with Clang 23.1.1 and `-std=c++23`. | Release shared library and tests pass. | Native suite and ASan/UBSan suite pass. LSan cannot run in this sandbox because ptrace is restricted. | Locally verified; LSan unverified. |
| Linux Flutter | Flutter 3.47.5 / Dart 3.13.4; CMake engine library. | Release library builds. | `dart analyze` passes. Flutter test runner cannot bind its local test socket in this sandbox. | Analysis verified; Flutter tests unverified locally. |
| Windows MSVC | New `windows-2025` job; MSVC `/std:c++23`, CMake project C++23. | DLL, exported C ABI, Flutter Windows example, and PowerShell standalone/JS Wasm release builds configured in CI. | Native test executable configured in CI; hosted result pending. | Pending CI run. |
| Windows MinGW | MinGW-w64 GCC 13+ through `tool/cmake/mingw64.cmake`. | Existing CI cross-build and export/dependency checks. | Wine smoke runs only on push. | CI configured; current run not available in this report. |
| macOS and iOS | New `macos-15` job; Apple Clang, CMake C++23, podspec C++23. | Native engine plus Flutter macOS/iOS examples configured in CI. | Native C++ suite configured; hosted result pending. | Pending CI run. |
| Android arm64 | New Ubuntu job; Flutter 3.47.5, NDK 28.2.13676358, CMake 3.22.1, engine C++23. | Flutter example APK build configured in CI. | No emulator/device test is configured. | Build result pending; runtime not exercised. |
| Wasm and Node | Emscripten 6.0.10, `-std=c++23`; Node 25.8.1 locally. | Standalone and JS release outputs rebuild byte-identically to checked-in assets. | Standalone smoke and Node/Jest 47 tests pass. | Locally verified. |
| Browser Wasm | Chromium stable in new CI browser job; JS single-file artifact from Emscripten 6.0.10. | Browser fixture exercises the shared cases. | Each conversion runs twice; browser process cannot start in this sandbox because Crashpad socket operations are denied. CI result pending. | Pending CI run. |

The CI workflows are the source of the tested hosted versions. Record the
actual runner image, compiler, CMake, Flutter, and NDK versions from successful
job logs when closing the acceptance task. Do not infer runtime validation from
a cross-build.
