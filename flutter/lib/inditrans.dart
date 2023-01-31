library inditrans;

import 'dart:convert';

import 'src/bindings.dart';
import 'src/utils.dart';
import 'src/proxy_ffi.dart';

export 'src/utils.dart' show TranslitOptions;

class Inditrans {
  static final Future<Inditrans> _instance = _init();
  final InditransBindings _bindings;

  Inditrans._(DynamicLibrary lib) : _bindings = InditransBindings(lib);

  static Future<Inditrans> _init() async {
    final lib = await FfiFacade.lib;
    return Inditrans._(lib);
  }

  static get instance => _instance;

  String transliterate(String text, String from, String to, [TranslitOptions options = TranslitOptions.None]) {
    final cText = _toCString(text);
    final cFrom = _toCString(from);
    final cTo = _toCString(to);
    final buffer = _bindings.transliterate(cText, cFrom, cTo, options.value);

    final result = _fromCString(buffer);
    _bindings.releaseBuffer(buffer);

    FfiFacade.allocator.free(cText);
    FfiFacade.allocator.free(cFrom);
    FfiFacade.allocator.free(cTo);
    return result;
  }

  String _fromCString(Pointer<Uint8> cString) {
    int len = 0;
    while (cString[len] != 0) {
      len++;
    }
    return len > 0 ? utf8.decode(cString.asTypedList(len)) : '';
  }

  /// Don't forget to free the c string using the same allocator if your are done with it!
  Pointer<Uint8> _toCString(String dartString) {
    List<int> bytes = utf8.encode(dartString);
    Pointer<Uint8> cString = FfiFacade.allocator.allocate(bytes.length);
    cString.asTypedList(bytes.length).setAll(0, bytes);
    return cString;
  }

  static registerWith(registrar) {
    // TODO
  }
}
