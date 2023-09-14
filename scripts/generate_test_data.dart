// ignore_for_file: avoid_print

import 'dart:convert';
import 'dart:io';

import 'package:http/http.dart' as http;

void main(List<String> args) async {
  final rootDir = Directory.current;

  final versionFile = File("${rootDir.path}/.version");
  if (!versionFile.existsSync()) {
    print("Run from root folder");
    return;
  }

  final url = Uri.parse(
    'https://raw.githubusercontent.com/indic-transliteration/common_tests/master/transliterationTests.json',
  );
  final response = await http.get(url);
  final jsonData = jsonDecode(response.body) as Map<String, dynamic>;

  final tests = [];

  final types = {};
  // basic_all_to_all
  final basicAllToAll = jsonData["basic_all_to_all"] as Map<String, dynamic>;
  for (final entry in basicAllToAll.entries) {
    final script = entry.key;
    final testCases = entry.value as Map<String, dynamic>;
    for (final test in testCases.entries) {
      final type = "any-to-any: ${test.key}";
      final str = test.value as String;
      if (types[type] == null) {
        types[type] = {
          "description": type,
          "type": "any-to-any",
          "targets": [],
        };
      }
      ((types[type] as Map<String, dynamic>)["targets"] as List<dynamic>).add({
        "script": script,
        "text": str,
      });
    }
  }
  for (final test in types.values) {
    tests.add(test);
  }

  // devanaagarii_round_trip
  final devanaagariiRoundTrip =
      jsonData["devanaagarii_round_trip"] as List<dynamic>;

  for (final entry in devanaagariiRoundTrip) {
    final testCases = entry as Map<String, dynamic>;
    final description = "to-and-fro: ${testCases["description"]}";

    final testCase = {
      "description": description,
      "type": "to-and-fro",
      "script": "devanagari",
      "text": entry["dev"],
    };

    final List<dynamic> targets = [];

    for (final test in testCases.entries) {
      final script = test.key;
      final str = test.value;
      if (!["description", "nonSupportingPrograms", "comments", "TODO", "dev"]
          .contains(script)) {
        targets.add({
          "script": script,
          "text": str,
        });
      }
    }

    testCase["targets"] = targets;

    tests.add(testCase);
  }

  // to_devanaagarii
  final toDevanaagarii = jsonData["to_devanaagarii"] as List<dynamic>;
  for (final entry in toDevanaagarii) {
    final testCases = entry as Map<String, dynamic>;
    final description = entry["description"];

    for (final test in testCases.entries) {
      final script = test.key;
      final text = test.value;
      if (!["description", "nonSupportingPrograms", "comments", "TODO", "dev"]
          .contains(script)) {
        tests.add({
          "description": description,
          "script": script,
          "text": text,
          "targets": [
            {
              "script": "devanagari",
              "text": entry["dev"],
            },
          ],
        });
        break;
      }
    }
  }

  // from_devanaagarii
  final fromDevanaagarii = jsonData["from_devanaagarii"] as List<dynamic>;
  for (final entry in fromDevanaagarii) {
    final testCases = entry as Map<String, dynamic>;
    final description = entry["description"];

    final List<dynamic> targets = [];

    for (final test in testCases.entries) {
      final script = test.key;
      final str = test.value;
      if (!["description", "nonSupportingPrograms", "comments", "TODO", "dev"]
          .contains(script)) {
        targets.add({
          "script": script,
          "text": str,
        });
      }
    }

    tests.add({
      "description": description,
      "script": "devanagari",
      "text": entry["dev"],
      "targets": targets,
    });
  }

  const encoder = JsonEncoder.withIndent("  ");

  final File genFile = File('test-files/sanscript-tests.json');
  genFile.writeAsStringSync(encoder.convert(tests));
}
