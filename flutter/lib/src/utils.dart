// ignore_for_file: hash_and_equals, constant_identifier_names

import 'dart:convert';

import 'package:wasm_ffi/ffi_proxy.dart';

class StagingMemory {
  final Allocator _allocator;
  final List<Pointer<NativeType>> _toFree = [];

  StagingMemory(this._allocator);

  Pointer<Uint8> toNativeString(String dartString) {
    List<int> bytes = utf8.encode(dartString);
    Pointer<Uint8> nativeString = _allocator.allocate(bytes.length + 1);
    _toFree.add(nativeString);
    final charList = nativeString.asTypedList(bytes.length + 1);
    charList.setAll(0, bytes);
    charList[bytes.length] = 0;
    return nativeString;
  }

  String fromNativeString(Pointer<Uint8> nativeString) {
    if (nativeString == nullptr) {
      return '';
    }
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
