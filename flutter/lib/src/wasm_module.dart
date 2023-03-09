import 'dart:typed_data';
import 'package:flutter/services.dart';
import 'package:wasm_ffi/wasm_ffi_modules.dart';
import 'package:wasm_ffi/wasm_ffi_meta.dart';
import 'package:wasm_interop/wasm_interop.dart' as interop;
import 'package:js/js_util.dart';

/// Documentation is in `emscripten_module_stub.dart`!
@extra
class WasmModule extends Module {
  final interop.Instance _instance;
  final List<WasmSymbol> _exports = [];

  @override
  List<WasmSymbol> get exports => _exports;

  static Future<WasmModule> initFromAsset(String path) async {
    final bytes = await rootBundle.load(path);
    return initFromBuffer(bytes.buffer);
  }

  static Future<WasmModule> initFromBuffer(ByteBuffer buffer) async {
    final wasmInstance = await interop.Instance.fromBufferAsync(buffer);
    return WasmModule._(wasmInstance);
  }

  FunctionDescription _fromWasmFunction(String name, Function func, int index) {
    String? s = getProperty(func, 'name');
    if (s != null) {
      int? length = getProperty(func, 'length');
      if (length != null) {
        return FunctionDescription(
            tableIndex: index,
            name: name,
            function: func,
            argumentCount: length);
      }
    }
    throw ArgumentError('$name does not seem to be a function symbol!');
  }

  WasmModule._(this._instance) {
    int index = 0;
    for (final e in _instance.functions.entries) {
      _exports.add(_fromWasmFunction(e.key, e.value, index++));
    }
  }

  @override
  void free(int pointer) {
    final func = _instance.functions['free'];
    func?.call(pointer);
  }

  @override
  ByteBuffer get heap => _instance.memories['memory']!.buffer;

  @override
  int malloc(int size) {
    final func = _instance.functions['malloc'];
    final resp = func?.call(size) as int;
    return resp;
  }
}
