library inditrans;

import 'src/bindings.dart';
import 'src/ffi_proxy.dart';
import 'src/types.dart';
import 'src/utils.dart';

export 'src/types.dart';

class Inditrans {
  static final Future<Inditrans> _instance = _init();
  final InditransBindings _bindings;
  final Allocator _allocator;

  Inditrans._(DynamicLibrary lib, Allocator allocator)
      : _bindings = InditransBindings(lib),
        _allocator = allocator;

  static Future<Inditrans> _init() async {
    final dynLib = await InditransDynamicLib.lib;
    return Inditrans._(dynLib, InditransDynamicLib.allocator);
  }

  static get instance => _instance;

  String transliterate(String text, String from, String to, [TranslitOptions options = TranslitOptions.None]) {
    final staging = StagingMemory(_allocator);

    final cText = staging.toNativeString(text);
    final cFrom = staging.toNativeString(from);
    final cTo = staging.toNativeString(to);

    final buffer = _bindings.transliterate(cText, cFrom, cTo, options.value);
    final result = staging.fromNativeString(buffer);

    staging.freeAll();
    return result;
  }

  static registerWith(registrar) {
    // TODO : check if it is needed
  }
}
