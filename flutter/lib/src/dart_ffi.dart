// Notice that in this file, we import dart:ffi and not proxy_ffi.dart
import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';

const String _libName = 'inditrans';

typedef Char = Uint8;

class FfiFacade {
  static final Future<DynamicLibrary> lib = _initBindings();
  static final Allocator allocator = Arena();

  static Future<DynamicLibrary> _initBindings() async {
    if (Platform.isMacOS || Platform.isIOS) {
      return DynamicLibrary.open('$_libName.framework/$_libName');
    } else if (Platform.isAndroid || Platform.isLinux) {
      return DynamicLibrary.open('lib$_libName.so');
    } else if (Platform.isWindows) {
      return DynamicLibrary.open('$_libName.dll');
    } else {
      throw UnsupportedError('Unknown platform: ${Platform.operatingSystem}');
    }
  }

  Future<DynamicLibrary> getBindings() async {
    return lib;
  }
}
