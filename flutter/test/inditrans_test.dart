// ignore_for_file: avoid_print

import 'dart:convert';
import 'dart:io';

import 'package:inditrans/inditrans.dart' as inditrans;
import 'package:inditrans/src/script.dart';
import 'package:test/test.dart';

// accept args
void main() async {
  await inditrans.init();

  test('optional Urdu pronunciation pack and fallback', () {
    final lexicon = File('../test-files/urdu-test-lexicon.iul').readAsBytesSync();
    expect(inditrans.transliterateUrduWithLexicon('کتاب', Script.iast, lexicon), 'kitāb');
    expect(inditrans.transliterateUrduWithLexicon('##کتاب## کتاب', Script.iast, lexicon), 'کتاب kitāb');
    final reused = inditrans.UrduPronunciationLexicon.load(lexicon);
    expect(reused, isNotNull);
    try {
      final loaded = reused!;
      expect(loaded.transliterate('کتاب', Script.iast), 'kitāb');
      final punctuation = inditrans.transliterate('۔', Script.urdu, Script.iast);
      expect(loaded.transliterate('کتاب۔کتاب', Script.iast), 'kitāb${punctuation}kitāb');
    } finally {
      reused?.dispose();
    }
    expect(() => reused.transliterate('کتاب', Script.iast), throwsStateError);
    expect(inditrans.UrduPronunciationLexicon.load(lexicon.sublist(1)), isNull);
    final fallback = inditrans.transliterate('کتاب', Script.urdu, Script.iast);
    expect(inditrans.transliterateUrduWithLexicon('کتاب', Script.iast, lexicon.sublist(1)), fallback);
  });

  // read json file
  final json = jsonDecode(File('../test-files/test-cases.json').readAsStringSync());

  // iterate over json which is in the form as in the file test-cases.json
  for (final input in (json as List).cast<Map<String, dynamic>>()) {
    final description = input['description'] as String;
    final inputText = input['text'] as String;
    final inputScript = (input['script'] as String).toScript();
    for (final target in (input['targets'] as List).cast<Map<String, dynamic>>()) {
      final targetScript = (target['script'] as String).toScript();
      final expected = target['text'];
      final options = inditrans.Option((target['options'] as String?) ?? '');

      if (inputScript == null || targetScript == null) {
        continue;
      }
      test(description, () {
        final actual = inditrans.transliterate(inputText, inputScript, targetScript, options);
        expect(actual, expected);
      });
    }
  }

  try {
    inditrans.transliterate('Dummy', Script.indlish, Script.indic);
  } catch (e) {
    print(e);
  }
}
