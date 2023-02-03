// ignore_for_file: hash_and_equals, constant_identifier_names

import 'dart:convert';

import 'ffi_proxy.dart';

N valueOf<T, N extends num>(dynamic val) {
  if (val is N) {
    return val;
  }
  if (val is AbstractVal<T, N>) {
    return val.value;
  }
  throw Exception('Invalid value');
}

abstract class AbstractVal<T, N extends num> {
  final N value;
  const AbstractVal(this.value);

  T create(N value);

  N _valueOf(dynamic other) {
    if (other is N) {
      return other;
    } else if (other is AbstractVal<T, N>) {
      return other.value;
    }
    throw Exception('Invalid value');
  }

  @override
  bool operator ==(dynamic other) {
    return value == _valueOf(other);
  }
}

abstract class AbstractEnum<T> extends AbstractVal<T, int> {
  const AbstractEnum(int value) : super(value);
}

abstract class AbstractConst<T, N extends num> extends AbstractVal<T, N> {
  const AbstractConst(N value) : super(value);

  T operator +(dynamic other) {
    return create((value + _valueOf(other)) as N);
  }

  T operator -(dynamic other) {
    return create((value + _valueOf(other)) as N);
  }

  T operator *(dynamic other) {
    return create((value * _valueOf(other)) as N);
  }

  T operator /(dynamic other) {
    return create((value / _valueOf(other)) as N);
  }
}

abstract class AbstractFlag<T> extends AbstractVal<T, int> {
  const AbstractFlag(int value) : super(value);

  T operator |(dynamic other) {
    return create(value | _valueOf(other));
  }

  T operator &(dynamic other) {
    return create(value & _valueOf(other));
  }
}

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
