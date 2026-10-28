library;

import 'dart:typed_data';

import 'package:universal_ffi/ffi.dart';
import 'package:universal_ffi/ffi_helper.dart';
import 'package:universal_ffi/ffi_utils.dart';

import 'src/bindings.dart';
import 'src/option.dart';
import 'src/script.dart';

export 'src/option.dart' show Option;
export 'src/script.dart' show Script, ScriptNameExtension, ToScriptExtension;

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
Future<void> init([String? modulePath]) async {
  _ffiHelper = await FfiHelper.load(
    modulePath ?? 'inditrans',
    options: {if (modulePath == null) LoadOption.isFfiPlugin, LoadOption.isStandaloneWasm},
  );

  _bindings = InditransBindings(_ffiHelper.library);
}

/// Write-only scripts: cannot be used as input sources (`from`).
const _readNotSupported = [Script.indlish, Script.wx];

/// Read-only scripts: cannot be used as destination targets (`to`).
const _writeNotSupported = [Script.indic];

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
String transliterate(
  String text,
  Script from,
  Script to, [
  Option? options,
  String skipStart = '##',
  String skipEnd = '##',
]) {
  if (text.isEmpty ||
      _readNotSupported.contains(from) ||
      _writeNotSupported.contains(to)) {
    return text;
  }
  return _ffiHelper.safeUsing((Arena arena) {
    final nativeText = text.toNativeUtf8(allocator: arena);
    final nativeFrom = from.name.toNativeUtf8(allocator: arena);
    final nativeTo = to.name.toNativeUtf8(allocator: arena);
    final nativeSkipStart = skipStart.toNativeUtf8(allocator: arena);
    final nativeSkipEnd = skipEnd.toNativeUtf8(allocator: arena);
    final buffer = _bindings.transliterate(
      nativeText.cast<Uint8>(),
      nativeFrom.cast<Uint8>(),
      nativeTo.cast<Uint8>(),
      options?.value ?? 0,
      nativeSkipStart.cast<Uint8>(),
      nativeSkipEnd.cast<Uint8>(),
    );
    final result = buffer.cast<Utf8>();
    return result == nullptr ? '' : result.toDartString();
  });
}

/// Transliterates Urdu with a caller-provided IUL1 pronunciation lexicon.
/// Invalid or empty lexicon bytes fall back to ordinary Urdu transliteration.
String transliterateUrduWithLexicon(
  String text,
  Script to,
  Uint8List lexicon, [
  Option? options,
  String skipStart = '##',
  String skipEnd = '##',
]) {
  if (text.isEmpty || _writeNotSupported.contains(to)) return text;
  return _ffiHelper.safeUsing((Arena arena) {
    final nativeText = text.toNativeUtf8(allocator: arena);
    final nativeTo = to.name.toNativeUtf8(allocator: arena);
    final nativeSkipStart = skipStart.toNativeUtf8(allocator: arena);
    final nativeSkipEnd = skipEnd.toNativeUtf8(allocator: arena);
    final data = lexicon.isEmpty ? nullptr.cast<Uint8>() : arena.allocate<Uint8>(lexicon.length);
    if (lexicon.isNotEmpty) data.asTypedList(lexicon.length).setAll(0, lexicon);
    final result = _bindings.transliterateUrduWithLexiconData(
      nativeText.cast<Uint8>(), nativeTo.cast<Uint8>(), options?.value ?? 0,
      data, lexicon.length, nativeSkipStart.cast<Uint8>(), nativeSkipEnd.cast<Uint8>(),
    );
    if (result == nullptr) return '';
    try {
      return result.cast<Utf8>().toDartString();
    } finally {
      _bindings.releaseBuffer(result);
    }
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
