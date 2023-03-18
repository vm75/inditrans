import 'package:wasm_ffi/wasm_ffi.dart';
import 'package:wasm_ffi/wasm_ffi_modules.dart';

import 'wasm_module.dart';

typedef UnsignedLong = Uint64;
typedef Int = Int32;

class InditransDynamicLib {
  static late DynamicLibrary _lib;
  static late Allocator _allocator;

  static get lib => _lib;
  static get allocator => _allocator;

  static init() async {
    Memory.init();
    final module = await WasmModule.initFromAsset(
        'packages/inditrans/assets/inditrans.wasm');
    _lib = DynamicLibrary.fromModule(module);
    _allocator = _lib.boundMemory;
  }
}
