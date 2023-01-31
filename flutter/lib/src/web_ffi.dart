import 'package:web_ffi/web_ffi.dart';
import 'package:web_ffi/web_ffi_modules.dart';

import 'wasm_module.dart';

typedef Char = Uint8;
typedef UnsignedLong = Uint64;

class FfiFacade {
  static Future<DynamicLibrary> lib = _initLib();
  static final Allocator allocator = _memory();

  static _memory() {
    Memory.init();
    return Memory.global!;
  }

  static Future<DynamicLibrary> _initLib() async {
    // After we loaded the wasm binaries and injected the js code
    // into our webpage, we obtain a module
    final module = await WasmModule.initFromAsset('packages/inditrans/assets/wasm/inditrans.wasm');
    return DynamicLibrary.fromModule(module);
  }
}
