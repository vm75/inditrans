import 'package:flutter/services.dart';
import 'package:wasm_ffi/wasm_ffi_core.dart';

class InditransDynamicLib {
  static late DynamicLibrary _lib;
  static late Allocator _allocator;

  static get lib => _lib;
  static get allocator => _allocator;

  static init() async {
    final bytes =
        await rootBundle.load('packages/inditrans/assets/inditrans.wasm');

    _lib = await DynamicLibrary.open(WasmType.standalone,
        wasmBinary: bytes.buffer.asUint8List());

    _allocator = _lib.memory;
  }
}
