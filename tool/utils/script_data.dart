import 'dart:convert';
import 'dart:io';

import 'latin_equivalents.dart';

const Map<String, int?> arrayTypes = {
  'aliases': null,
  'vowels': 19,
  'vowelMarks': 19,
  'consonants': 50,
  'otherDiacritics': 4,
  'symbols': 13,
  'vedicSymbols': 3,
};

class ScriptInfo {
  final String type;
  final String name;
  final Map<String, dynamic> info;

  ScriptInfo(this.type, this.name, this.info);
}

class ScriptData {
  ScriptData(String path, this.latinEquivalents) {
    final jsonData =
        jsonDecode(File(path).readAsStringSync()) as Map<String, dynamic>;

    parseScriptInfo(jsonData);
  }

  void parseScriptInfo(Map<String, dynamic> map) {
    for (final entries in map.entries) {
      final type = entries.key;
      final scripts = entries.value as Map<String, dynamic>;
      for (final entries in scripts.entries) {
        scriptInfoList.add(
          ScriptInfo(type, entries.key, entries.value as Map<String, dynamic>),
        );
      }
    }
    scriptInfoList.sort((a, b) => a.name.compareTo(b.name));

    for (final entry in scriptInfoList) {
      scriptList.add(entry.name);
      if (entry.info['aliases'] != null) {
        for (final alias in entry.info['aliases'] as List<dynamic>) {
          scriptList.add(alias as String);
        }
      }
    }
    scriptList.add('indic');
  }

  void addEquivalents(
    String str,
    Map<dynamic, dynamic> equivalents, {
    required bool upperEquivalent,
  }) {
    final equivalentList = (equivalents[str] ?? []) as List<dynamic>;
    bool needsEquivalent = false;

    if (upperEquivalent) {
      final equivalentUpper = str.toUpperCase();
      if (equivalentUpper != str && !equivalentList.contains(equivalentUpper)) {
        equivalentList.add(equivalentUpper);
        needsEquivalent = true;
      }
    }

    final codeUnits = str.codeUnits;
    final List<String> parts = [];
    for (final unit in codeUnits) {
      if (latinEquivalents.equivalents.containsKey(unit)) {
        parts.add(latinEquivalents.equivalents[unit]![0]);
        needsEquivalent = true;
      } else {
        parts.add(String.fromCharCode(unit));
      }
    }
    if (!needsEquivalent) {
      return;
    }
    // join parts and add to equivalents
    final equivalentStr = parts.join();
    if (str != equivalentStr && !equivalentList.contains(equivalentStr)) {
      equivalentList.add(equivalentStr);
      needsEquivalent = true;
      if (upperEquivalent) {
        final equivalentUpper = equivalentStr.toUpperCase();
        if (equivalentUpper != equivalentStr &&
            !equivalentList.contains(equivalentUpper)) {
          equivalentList.add(equivalentUpper);
          needsEquivalent = true;
        }
      }
    }
    if (needsEquivalent) {
      equivalents[str] = equivalentList;
    }
  }

  void prepareEquivalents() {
    for (final script in scriptInfoList) {
      if (script.type != 'latin') continue;
      final equivalents =
          (script.info['equivalents'] ??= {}) as Map<dynamic, dynamic>;
      final upper =
          script.name == 'iast' || script.name == 'ipa' || script.name == 'iso';
      for (final category in arrayTypes.keys) {
        for (final value
            in ((script.info[category] as List?) ?? []).cast<String>()) {
          addEquivalents(value, equivalents, upperEquivalent: upper);
        }
      }
    }
  }

  List<ScriptInfo> scriptInfoList = [];
  List<String> scriptList = [];
  final LatinEquivalents latinEquivalents;
}
