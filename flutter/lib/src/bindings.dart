// ignore_for_file: always_specify_types
// ignore_for_file: camel_case_types
// ignore_for_file: constant_identifier_names
// ignore_for_file: non_constant_identifier_names

// AUTO GENERATED FILE, DO NOT EDIT.
//
// Generated by `package:ffigen`.
import 'ffi_proxy.dart' as ffi;

/// Bindings for `Inditrans`.
///
/// Regenerate bindings with `dart run ffigen --config ffigen.yaml`.
///
class InditransBindings {
  /// Holds the symbol lookup function.
  final ffi.Pointer<T> Function<T extends ffi.NativeType>(String symbolName) _lookup;

  /// The symbols are looked up in [dynamicLibrary].
  InditransBindings(ffi.DynamicLibrary dynamicLibrary) : _lookup = dynamicLibrary.lookup;

  /// The symbols are looked up with [lookup].
  InditransBindings.fromLookup(ffi.Pointer<T> Function<T extends ffi.NativeType>(String symbolName) lookup) : _lookup = lookup;

  int translitOptionsToInt(
    ffi.Pointer<ffi.Char> optionStr,
  ) {
    return _translitOptionsToInt(
      optionStr,
    );
  }

  late final _translitOptionsToIntPtr = _lookup<ffi.NativeFunction<ffi.UnsignedLong Function(ffi.Pointer<ffi.Char>)>>('translitOptionsToInt');
  late final _translitOptionsToInt = _translitOptionsToIntPtr.asFunction<int Function(ffi.Pointer<ffi.Char>)>();

  /// transliterate
  ffi.Pointer<ffi.Char> transliterate(
    ffi.Pointer<ffi.Char> text,
    ffi.Pointer<ffi.Char> from,
    ffi.Pointer<ffi.Char> to,
    int options,
  ) {
    return _transliterate(
      text,
      from,
      to,
      options,
    );
  }

  late final _transliteratePtr = _lookup<ffi.NativeFunction<ffi.Pointer<ffi.Char> Function(ffi.Pointer<ffi.Char>, ffi.Pointer<ffi.Char>, ffi.Pointer<ffi.Char>, ffi.UnsignedLong)>>('transliterate');
  late final _transliterate = _transliteratePtr.asFunction<ffi.Pointer<ffi.Char> Function(ffi.Pointer<ffi.Char>, ffi.Pointer<ffi.Char>, ffi.Pointer<ffi.Char>, int)>();

  /// transliterate2
  ffi.Pointer<ffi.Char> transliterate2(
    ffi.Pointer<ffi.Char> text,
    ffi.Pointer<ffi.Char> from,
    ffi.Pointer<ffi.Char> to,
    ffi.Pointer<ffi.Char> optionStr,
  ) {
    return _transliterate2(
      text,
      from,
      to,
      optionStr,
    );
  }

  late final _transliterate2Ptr =
      _lookup<ffi.NativeFunction<ffi.Pointer<ffi.Char> Function(ffi.Pointer<ffi.Char>, ffi.Pointer<ffi.Char>, ffi.Pointer<ffi.Char>, ffi.Pointer<ffi.Char>)>>('transliterate2');
  late final _transliterate2 = _transliterate2Ptr.asFunction<ffi.Pointer<ffi.Char> Function(ffi.Pointer<ffi.Char>, ffi.Pointer<ffi.Char>, ffi.Pointer<ffi.Char>, ffi.Pointer<ffi.Char>)>();

  /// release
  void releaseBuffer(
    ffi.Pointer<ffi.Char> buffer,
  ) {
    return _releaseBuffer(
      buffer,
    );
  }

  late final _releaseBufferPtr = _lookup<ffi.NativeFunction<ffi.Void Function(ffi.Pointer<ffi.Char>)>>('releaseBuffer');
  late final _releaseBuffer = _releaseBufferPtr.asFunction<void Function(ffi.Pointer<ffi.Char>)>();
}
