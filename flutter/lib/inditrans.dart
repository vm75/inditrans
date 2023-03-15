library inditrans;

import 'src/bindings.dart';
import 'src/ffi_proxy.dart';
import 'src/types.dart';
import 'src/utils.dart';

export 'src/types.dart' show Option, Script, InditransScriptExtension;

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
String transliterate(String text, Script from, Script to,
    [Option options = Option.None]) {
  final staging = StagingMemory(_allocator);

  final nativeText = staging.toNativeString(text);
  final nativeFrom = staging.toNativeString(from.toString());
  final nativeTo = staging.toNativeString(to.toString());

  final buffer =
      _bindings.transliterate(nativeText, nativeFrom, nativeTo, options.value);
  final result = staging.fromNativeString(buffer);

  staging.freeAll();
  return result;
}
