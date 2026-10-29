@TestOn('browser')
library;

import 'dart:convert';

import 'package:inditrans/inditrans.dart' as inditrans;
import 'package:test/test.dart';

// The shared test pack, encoded here because browser tests cannot use dart:io.
const _pack = 'SVVMMr5SuFGUAAAAeyJmb3JtYXQiOjIsInNjcmlwdCI6InVyZHUiLCJ3b3JkbGlzdCI6eyJuYW1lIjoidXJkdS10ZXN0LWxleGljb24udHN2Iiwic2hhMjU2IjoiZDU2MGIzY2Y1ZTMzNzA0MTc0YjQ1MGFlMzVlNDRjYWJjOWZhZjU1MGM5NTdhZmZmYjY3ZjBhYTNjMTA4YTgyZiJ9fVBMWDEEAAEAAAAC2KcC2KgC2KoC2qkAAAQACgAAAAMCAAEBAAMAFoGlgCyA';

void main() {
  // tests/integration/dart_web_smoke.py serves the current build with CORS.
  setUpAll(() async => inditrans.init('http://127.0.0.1:18773/inditrans.wasm'));

  test('Urdu pronunciation pack and fallback on web', () {
    final pack = base64Decode(_pack);
    expect(inditrans.transliterateUrduWithLexicon('کتاب', inditrans.Script.iast, pack), 'kitāb');
    final reused = inditrans.UrduPronunciationLexicon.load(pack);
    expect(reused, isNotNull);
    try {
      final punctuation = inditrans.transliterate('۔', inditrans.Script.urdu, inditrans.Script.iast);
      expect(reused!.transliterate('کتاب۔کتاب', inditrans.Script.iast), 'kitāb${punctuation}kitāb');
    } finally {
      reused?.dispose();
    }
    expect(inditrans.transliterateUrduWithLexicon('##کتاب## کتاب', inditrans.Script.iast, pack), 'کتاب kitāb');
    expect(inditrans.transliterateUrduWithLexicon('کتاب', inditrans.Script.iast, pack.sublist(1)),
        inditrans.transliterate('کتاب', inditrans.Script.urdu, inditrans.Script.iast));
    expect(inditrans.transliterateUrduWithLexicon('کتِاب', inditrans.Script.iast, pack),
        inditrans.transliterate('کتِاب', inditrans.Script.urdu, inditrans.Script.iast));
    expect(inditrans.transliterateUrduWithLexicon('کتاب', inditrans.Script.iast, pack, inditrans.Option.DisableInference),
        inditrans.transliterate('کتاب', inditrans.Script.urdu, inditrans.Script.iast, inditrans.Option.DisableInference));
    expect(inditrans.transliterateUrduWithLexicon('کتاب', inditrans.Script.urdu, pack), 'کتاب');
    expect(inditrans.transliterateUrduWithLexicon('کتاب', inditrans.Script.tamil, pack),
        inditrans.transliterate('किताब्', inditrans.Script.devanagari, inditrans.Script.tamil));
  });
}
