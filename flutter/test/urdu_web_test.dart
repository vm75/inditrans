@TestOn('browser')
library;

import 'dart:convert';

import 'package:inditrans/inditrans.dart' as inditrans;
import 'package:test/test.dart';

// The shared test pack, encoded here because browser tests cannot use dart:io.
const _pack = 'SVVMMfBUn2i1AAAAeyJ2ZXJzaW9uIjoidGVzdC0xIiwicHJvdmVuYW5jZSI6IkluZGl0cmFucyBwcm9qZWN0IHJlZ3Jlc3Npb24gZml4dHVyZTsgbm90IGEgcHJvZHVjdGlvbiBsZXhpY29uIiwic291cmNlX3NoYTI1NiI6ImM1M2E4MzhlY2ZhOTY1NTg2ZjFiZDU3MDE4YzdkZmJhMWQxNjUwY2M4YzUzMzJhNmJlNzQ3OTQxMDY1ZDczNDUifQEAAAAIAAEA2qnYqtin2KgDAAEAAv8BDwH/ARYA/w==';

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
      expect(reused!.transliterate('کتاب۔کتاب', inditrans.Script.iast), 'kitāb' + punctuation + 'kitāb');
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
