library inditrans;

import 'dart:convert';

import 'src/bindings.dart';
import 'src/utils.dart';
import 'src/proxy_ffi.dart';

class Inditrans {
  late InditransBindings _bindings;

  factory Inditrans() {
    return Inditrans._();
  }

  Inditrans._();

  init() async {
    _bindings = InditransBindings(await FfiFacade.lib);
  }

  int translitOptionsToInt(String option) {
    return _bindings.translitOptionsToInt(_toCString(option));
  }

  String transliterate(String text, String from, String to, [TranslitOptions options = TranslitOptions.None]) {
    final buffer = _bindings.transliterate(_toCString(text), _toCString(from), _toCString(to), options.value);

    final result = _fromCString(buffer);
    _bindings.releaseBuffer(buffer);

    return result;
  }

  String transliterate2(String text, String from, String to, [String options = ""]) {
    final buffer = _bindings.transliterate2(_toCString(text), _toCString(from), _toCString(to), _toCString(options));

    final result = _fromCString(buffer);
    _bindings.releaseBuffer(buffer);

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

  static registerWith(registrar) {}
}
