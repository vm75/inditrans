// ignore_for_file: avoid_print

import 'dart:convert';
import 'dart:io';

import 'package:inditrans/inditrans.dart' as inditrans;
import 'package:inditrans/src/script.dart';
import 'package:test/test.dart';

// accept args
void main() async {
  await inditrans.init();

  // read json file
  final json =
      jsonDecode(File('../test-files/test-cases.json').readAsStringSync());

  // iterate over json which is in the form as in the file test-cases.json
  for (final input in json as List<Map<String, dynamic>>) {
    final description = input['description'] as String;
    final inputText = input['text'] as String;
    final inputScript = (input['script'] as String).toScript();
    for (final target in input['targets'] as List<Map<String, dynamic>>) {
      final targetScript = (target['script'] as String).toScript();
      final expected = target['text'];

      if (inputScript == null || targetScript == null) {
        continue;
      }
      test(description, () {
        final actual =
            inditrans.transliterate(inputText, inputScript, targetScript);
        expect(actual, expected);
      });
    }
  }

  try {
    inditrans.transliterate('Dummy', Script.readableLatin, Script.indic);
  } catch (e) {
    print(e);
  }
}
