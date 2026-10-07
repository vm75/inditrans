# Platform and toolchain support

This matrix records hosted acceptance for the final compact-static engine on
commit `e302ae3` (PR run 37560001579). All listed jobs passed. A successful
cross-build confirms compilation and packaging only; it does not imply a
runtime test on that platform.

## Declared toolchain requirements

- Native engine: CMake 3.20 or later and a compiler with C++23 support.
  Repository guidance lists Clang 17+ and MinGW-w64 GCC 13+ for the Windows
  cross-build.
- WebAssembly: Emscripten 6.0.10, selected with `-std=c++23` in both release
  build paths.
- Flutter: Android, iOS, Linux, macOS, Windows, and Web are declared in
  `flutter/pubspec.yaml`. CI pins Flutter 3.47.5 / Dart 3.13.4.
- Node.js: Node 18.3 or later; CI uses Node 24 and Emscripten 6.0.10.
- Android build: Android Gradle Plugin 8.11.1, Kotlin Gradle Plugin 2.2.20,
  Gradle 8.14.3, and CMake 3.22.1. Flutter reports NDK 28.2.13676358 as its
  default; the Flutter plugin's native CMake build resolved NDK 27.0.12077973.
- Apple plugin podspecs select C++23 and retain their current deployment
  targets (iOS 9.0, macOS 10.11).

## Acceptance evidence

| Platform/path | Toolchain and C++23 selection | Build and test evidence | Runtime coverage |
|---|---|---|---|
| Linux native | Ubuntu 24.04 hosted GNU 13.3.0 for Flutter's CMake library; local baseline used Clang 23.1.1 and CMake 4.4.4. | C++ tests and ASan/LSan/UBSan passed. | Native tests passed. |
| Linux Flutter | Flutter 3.47.5 / Dart 3.13.4. | Linux shared library build, `dart analyze`, and Flutter tests passed. | Flutter tests passed. |
| Windows MSVC | `windows-2025`, Visual Studio 18, MSVC 19.51.36260.0; CMake requests C++23; MSVC test compile uses `/std:c++latest`. | DLL C ABI export checks, native tests, Flutter Windows example, and PowerShell standalone/JS Wasm builds passed. | Native tests passed; Flutter example build verified. |
| Windows MinGW | Ubuntu 24.04, MinGW-w64 GCC 13.0.0, C++23. | DLL cross-build and export/dependency checks passed. | Cross-build only; Wine smoke is not part of the PR acceptance run. |
| macOS and iOS | `macos-15`, AppleClang 17.0.0.17000013; CMake and podspecs select C++23. | Native tests and Flutter macOS/iOS example builds passed. | Native tests passed; Flutter examples build verified. |
| Android arm64 | Ubuntu 24.04, Flutter 3.47.5 / Dart 3.13.4, AGP 8.11.1, Kotlin 2.2.20, Gradle 8.14.3, CMake 3.22.1. Flutter reports default NDK 28.2.13676358; plugin CMake used NDK 27.0.12077973. | Flutter example APK build passed. | No emulator/device test is configured. |
| Wasm and Node | Emscripten 6.0.10; Node 24 in CI. | Standalone Wasm smoke, 47 Jest tests, and npm package dry-run passed; release artifacts rebuilt. | Node smoke and package tests passed. |
| Browser Wasm | Chrome 155.0.8059.39; JS single-file artifact from Emscripten 6.0.10. | Shared browser fixture passed 47 conversions repeated twice, including output release. | Browser execution passed. |

The Android build emitted a notice that newer Android Gradle Plugin and Kotlin
versions will be required by a future Flutter release; the pinned toolchain
passed this acceptance run. Android app runtime behavior was not exercised.
