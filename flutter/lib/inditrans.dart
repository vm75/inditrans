library inditrans;

import 'package:universal_ffi/ffi.dart';
import 'package:universal_ffi/ffi_helper.dart';
import 'package:universal_ffi/ffi_utils.dart';

import 'src/bindings.dart';
import 'src/option.dart';
import 'src/script.dart';

export 'src/option.dart' show Option;
export 'src/script.dart' show Script, ScriptNameExtension, ToScriptExtension;

const bool kIsWeb = bool.fromEnvironment('dart.library.js_interop');

late FfiHelper _ffiHelper;
late InditransBindings _bindings;

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
init([String? modulePath]) async {
  _ffiHelper = await FfiHelper.load(
    modulePath ?? 'inditrans',
    options: {
      modulePath == null ? 'is-ffi-plugin' : '',
      'is-standalone-wasm',
    },
  );

  _bindings = InditransBindings(_ffiHelper.library);
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
  return _ffiHelper.safeUsing((Arena arena) {
    final nativeText = text.toNativeUtf8(allocator: arena);
    final nativeFrom = from.name.toNativeUtf8(allocator: arena);
    final nativeTo = to.name.toNativeUtf8(allocator: arena);
    final buffer = _bindings.transliterate(nativeText.cast<Uint8>(),
        nativeFrom.cast<Uint8>(), nativeTo.cast<Uint8>(), options?.value ?? 0);
    final result = buffer.cast<Utf8>().toDartString();
    return result;
  });
}

/// Checks if [script] is supported by the library.
bool isScriptSupported(String script) {
  return _ffiHelper.safeUsing((Arena arena) {
    final nativeScript = script.toNativeUtf8(allocator: arena);
    final result = _bindings.isScriptSupported(nativeScript.cast<Uint8>());
    return result == 1;
  });
}
