// ignore_for_file: non_constant_identifier_names, constant_identifier_names

import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';

import 'bindings.dart';
import 'const_utils.dart';

extension FfiHelperOnString on String {
  Pointer<Char> toNativeArray(Arena arena) {
    return toNativeUtf8(allocator: arena).cast();
  }
}

class TranslitOptions extends AbstractFlag<TranslitOptions> {
  const TranslitOptions(int value) : super(value);
  static const None = TranslitOptions(0);
  static const IgnoreVedicAccents = TranslitOptions(1);
  static const IgnoreQuotedMarkers = TranslitOptions(2);
  static const TamilTraditional = TranslitOptions(4);
  static const TamilSuperscripted = TranslitOptions(8);
  static const InferAnuswara = TranslitOptions(16);
  static const RetainZeroWidthChars = TranslitOptions(32);

  @override
  TranslitOptions create(int value) {
    return TranslitOptions(value);
  }
}

class Inditrans {
  static const String _libName = 'inditrans';

  /// The dynamic library in which the symbols for [InditransBindings] can be found.
  static final DynamicLibrary _dylib = () {
    if (Platform.isMacOS || Platform.isIOS) {
      return DynamicLibrary.open('$_libName.framework/$_libName');
    }
    if (Platform.isAndroid || Platform.isLinux) {
      return DynamicLibrary.open('lib$_libName.so');
    }
    if (Platform.isWindows) {
      return DynamicLibrary.open('$_libName.dll');
    }
    throw UnsupportedError('Unknown platform: ${Platform.operatingSystem}');
  }();

  /// The bindings to the native functions in [_dylib].
  static final InditransBindings _bindings = InditransBindings(_dylib);

  static String transliterate(String text, String from, String to, [options = TranslitOptions.None]) {
    return using((Arena arena) {
      final buffer = _bindings.transliterate(text.toNativeArray(arena), from.toNativeArray(arena), to.toNativeArray(arena), options.value);

      final result = buffer.cast<Utf8>().toDartString();
      _bindings.releaseBuffer(buffer);

      return result;
    });
  }
}
