// ignore_for_file: avoid_print

import 'dart:convert';
import 'dart:io';

import 'package:inditrans/inditrans.dart' as inditrans;
import 'package:test/test.dart';

// accept args
void main() async {
  await inditrans.init();

  // read json file
  var json =
      jsonDecode(File("../test-files/test-cases.json").readAsStringSync());

  // iterate over json which is in the form as in the file test-cases.json
  for (final input in json) {
    final description = input['description'];
    final inputText = input['text'];
    final inputScript = input['script'];
    for (final target in input['targets']) {
      final targetScript = target['script'];
      final expected = target['text'];
      test(description, () {
        final actual =
            inditrans.transliterate(inputText, inputScript, targetScript);
        expect(actual, expected);
      });
    }
  }
}
