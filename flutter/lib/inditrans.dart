library inditrans;

import 'src/bindings.dart';
import 'src/ffi_proxy.dart';
import 'src/types.dart';
import 'src/utils.dart';

export 'src/types.dart';

class Inditrans {
  static late dynamic _platformLib;
  static late InditransBindings _bindings;
  static late Allocator _allocator;

  static init() async {
    _platformLib = await InditransDynamicLib.lib;
    _bindings = InditransBindings(_platformLib);
    _allocator = InditransDynamicLib.allocator;
  }

  static String transliterate(String text, String from, String to, [TranslitOptions options = TranslitOptions.None]) {
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
    // ignore
  }
}
