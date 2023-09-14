library inditrans;

import 'package:wasm_ffi/ffi_proxy.dart';

import 'src/bindings.dart';
import 'src/option.dart';
import 'src/script.dart';
import 'src/wasm_lib.dart' if (dart.library.ffi) 'src/ffi_lib.dart';

export 'src/option.dart' show Option;
export 'src/script.dart' show Script, ScriptNameExtension, ToScriptExtension;

late dynamic _platformLib;
late InditransBindings _bindings;
late Allocator _allocator;

/// Inditrans init. Init should be completed before any use is attempted.
///
/// ```dart
/// import 'package:inditrans/inditrans.dart' as inditrans;
/// void main() async {
///   ...
///   await inditrans.init();
///   ...
/// }
/// ```
init() async {
  await InditransDynamicLib.init();
  _platformLib = InditransDynamicLib.lib;
  _bindings = InditransBindings(_platformLib);
  _allocator = InditransDynamicLib.allocator;
}

/// Transliterates [text] from [from] script to [to] script.
/// [Option] can be used for some specific config.
/// Should be called only after [init] is completed.
///
/// If the transliterate fails, returns an empty string.
///
/// ```dart
/// import 'package:inditrans/inditrans.dart' as inditrans;
/// ...
/// inditrans.transliterate('text', transliterate.Script.devanagari, transliterate.Script.tamil);
///
/// ```
String transliterate(String text, Script from, Script to, [Option? options]) {
  final result = using((Arena arena) {
    Pointer<Uint8> nativeText = text.toNativeUtf8(arena);
    Pointer<Uint8> nativeFrom = from.name.toNativeUtf8(arena);
    Pointer<Uint8> nativeTo = to.name.toNativeUtf8(arena);
    final buffer = _bindings.transliterate(
        nativeText, nativeFrom, nativeTo, options?.value ?? 0);

    return buffer.toDartString();
  }, _allocator);
  return result;
}

/// Checks if [script] is supported by the library.
bool isScriptSupported(String script) {
  return using((Arena arena) {
    Pointer<Uint8> nativeScript = script.toNativeUtf8(arena);
    final result = _bindings.isScriptSupported(nativeScript);
    return result == 1;
  }, _allocator);
}
