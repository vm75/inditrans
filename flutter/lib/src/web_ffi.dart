import 'package:flutter/services.dart';
import 'package:inject_js/inject_js.dart';
import 'package:web_ffi/web_ffi.dart';
import 'package:web_ffi/web_ffi_modules.dart';

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
    // Inject the JavaScript into our page
    await importLibrary('packages/inditrans/assets/wasm/split/inditrans.js');

    // Load the WebAssembly binaries from assets
    final wasm = await rootBundle.load('packages/inditrans/assets/wasm/split/inditrans.wasm');
    Uint8List wasmBinaries = (wasm).buffer.asUint8List();

    // After we loaded the wasm binaries and injected the js code
    // into our webpage, we obtain a module
    final module = await EmscriptenModule.compile(wasmBinaries, 'inditrans');
    return DynamicLibrary.fromModule(module);
  }
}
