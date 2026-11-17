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
  Gradle 8.14.3, CMake 3.22.1, and Android NDK r30 LTS
  (`30.0.16248370`) pinned for the Flutter plugin and example app.
  Previously, the plugin CMake build resolved NDK r27 even when Flutter
  reported NDK r28 as its default.
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
| Android arm64 | Ubuntu 24.04, Flutter 3.47.5 / Dart 3.13.4, AGP 8.11.1, Kotlin 2.2.20, Gradle 8.14.3, CMake 3.22.1. Historical build: Flutter reported NDK r28; plugin CMake used NDK r27. The revised workflow selects NDK r30 and adds 16 KiB checks. | Previous Flutter example APK build passed; new APK/AAB checks await acceptance. | No emulator/device test is configured. |
| Wasm and Node | Emscripten 6.0.10; Node 24 in CI. | Standalone Wasm smoke, 47 Jest tests, and npm package dry-run passed; release artifacts rebuilt. | Node smoke and package tests passed. |
| Browser Wasm | Chrome 155.0.8059.39; JS single-file artifact from Emscripten 6.0.10. | Shared browser fixture passed 47 conversions repeated twice, including output release. | Browser execution passed. |

The Android build emitted a notice that newer Android Gradle Plugin and Kotlin
versions will be required by a future Flutter release; the pinned toolchain
passed this acceptance run. Android app runtime behavior was not exercised.

## Android 16 KiB page-size compatibility (issues #72 and #76)

Android NDK r30 LTS (`30.0.16248370`) is pinned in
`flutter/android/build.gradle` and `flutter/example/android/app/build.gradle`.
The Android CMake linker also requests 16 KiB ELF segment alignment for
consumers that use older toolchains. Native dependencies supplied by the
host app or other plugins must be compatible independently.

For a local consumer app, point its `pubspec.yaml` dependency at
`<checkout>/flutter`. Packaged releases copy the same CMake rules into
`flutter/src/` via `python3 scripts/prepare_flutter_package.py`.

To check an Android example build from the repository root:

```sh
cd flutter/example
flutter pub get
flutter build apk --release --target-platform android-arm64
flutter build appbundle --release --target-platform android-arm64
cd ../..
python3 scripts/check_android_page_alignment.py \
  flutter/example/build/app/outputs/flutter-apk/app-release.apk \
  flutter/example/build/app/outputs/bundle/release/app-release.aab
```

The checker inspects all 64-bit shared libraries in APKs/AABs for 16 KiB ELF
`PT_LOAD` alignment, and checks ZIP data alignment for uncompressed APK
libraries. It does not inspect AAB `BundleConfig.pb`: separately use
`bundletool dump config --bundle=<path-to-app-release.aab>` and confirm
`PAGE_ALIGNMENT_16K`. Test on a 16 KiB-page Android device/emulator and
verify the release AAB using Play Console before publishing.
