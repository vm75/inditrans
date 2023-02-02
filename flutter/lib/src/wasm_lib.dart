import 'package:web_ffi/web_ffi.dart';
import 'package:web_ffi/web_ffi_modules.dart';

import 'wasm_module.dart';

typedef Char = Uint8;
typedef UnsignedLong = Uint64;

class InditransDynamicLib {
  static final Future<DynamicLibrary> _lib = _initLib();
  static final Allocator _allocator = _memory();

  static get lib => _lib;
  static get allocator => _allocator;

  static _memory() {
    Memory.init();
    return Memory.global!;
  }

  static Future<DynamicLibrary> _initLib() async {
    final module = await WasmModule.initFromAsset('packages/inditrans/assets/wasm/inditrans.wasm');
    return DynamicLibrary.fromModule(module);
  }
}
