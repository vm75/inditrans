library inditrans;

import 'dart:convert';

import 'src/bindings.dart';
import 'src/utils.dart';
import 'src/ffi_proxy.dart';

export 'src/utils.dart' show TranslitOptions;

class StagingMemory {
  final Allocator _allocator;
  final List<Pointer<NativeType>> _toFree = [];

  StagingMemory(this._allocator);

  Pointer<Uint8> toNativeString(String dartString) {
    List<int> bytes = utf8.encode(dartString);
    Pointer<Uint8> nativeString = _allocator.allocate(bytes.length);
    _toFree.add(nativeString);
    nativeString.asTypedList(bytes.length).setAll(0, bytes);
    return nativeString;
  }

  String fromNativeString(Pointer<Uint8> nativeString) {
    _toFree.add(nativeString);
    int len = 0;
    while (nativeString[len] != 0) {
      len++;
    }
    return len > 0 ? utf8.decode(nativeString.asTypedList(len)) : '';
  }

  void freeAll() {
    for (final entry in _toFree) {
      _allocator.free(entry);
    }
    _toFree.clear();
  }
}

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
    // TODO
  }
}
