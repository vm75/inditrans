import 'dart:convert';
import 'dart:io';

import 'latin_equivalents.dart';

// ignore: leading_newlines_in_multiline_strings
const headerPrefix = '''#pragma once

#define Z "\\u0000"
#define E "\\u0001"

#define VEDIC "v\\u0000"
#define INDIC "i\\u0000"
#define TAMIL "t\\u0000"
#define LATIN "l\\u0000"

#define VOWELS "v\\u0000"
#define VOWELMARKS "m\\u0000"
#define CONSONANTS "c\\u0000"
#define OTHERDIACRITICS "o\\u0000"
#define SYMBOLS "s\\u0000"
#define VEDICSYMBOLS "S\\u0000"
#define EQUIVALENTS "E\\u0000"
#define ALIASES "A\\u0000"
#define LANGUAGES "l\\u0000"

// clang-format off

''';

const headerSuffix = '''
// clang-format on
''';

const Map<String, int?> arrayTypes = {
  'aliases': null,
  'vowels': 19,
  'vowelMarks': 19,
  'consonants': 50,
  'otherDiacritics': 4,
  'symbols': 13,
  'vedicSymbols': 3,
};
const List<String> arrayGroupTypes = [
  'languages',
];

class ScriptInfo {
  final String type;
  final String name;
  final Map<String, dynamic> info;

  ScriptInfo(this.type, this.name, this.info);
}

class BinaryBuffer {
  BinaryBuffer() : buffer = StringBuffer();

  void write(String str) {
    buffer.write(str);
  }

  void writeString(String str) {
    buffer.write('"$str" Z ');
  }

  List<String> writeArray(
    String type,
    List<dynamic> arr,
    int? len, {
    required bool needsEquivalent,
  }) {
    if (len != null && arr.length != len) {
      assert(arr.length == len);
    }
    final List<String> equivalentList = [];
    buffer.write('    ${type.toUpperCase()} ');
    for (final entry in arr) {
      writeString(entry as String);
      if (needsEquivalent) {
        equivalentList.add(entry);
      }
    }
    buffer.write('E\n');
    return equivalentList;
  }

  void writeArrayGroup(
    String type,
    Map<dynamic, dynamic> equivalents,
  ) {
    if (equivalents.isEmpty) {
      return;
    }

    buffer.write('    ${type.toUpperCase()}\n');
    for (final entry in equivalents.entries) {
      buffer.write('      "${entry.key}" Z ');
      for (final equivalent in entry.value) {
        writeString(equivalent as String);
      }
      buffer.write('E\n');
    }
    buffer.write('    E\n');
  }

  StringBuffer buffer;
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

  void writeScriptInfo(ScriptInfo scriptInfo, BinaryBuffer buffer) {
    buffer.write('  "${scriptInfo.name}" Z ${scriptInfo.type.toUpperCase()}\n');
    final Map<dynamic, dynamic> equivalents =
        (scriptInfo.info['equivalents'] ?? {}) as Map<dynamic, dynamic>;
    for (final entry in arrayTypes.entries) {
      if (scriptInfo.info[entry.key] != null) {
        final needsEquivalent = scriptInfo.type == 'latin';
        final upperEquivalents = scriptInfo.name == 'iast' ||
            scriptInfo.name == 'ipa' ||
            scriptInfo.name == 'iso';
        final equivalentList = buffer.writeArray(
          entry.key,
          scriptInfo.info[entry.key] as List<dynamic>,
          entry.value,
          needsEquivalent: needsEquivalent,
        );
        for (final equivalent in equivalentList) {
          addEquivalents(
            equivalent,
            equivalents,
            upperEquivalent: upperEquivalents,
          );
        }
      }
    }
    for (final entry in arrayGroupTypes) {
      if (scriptInfo.info[entry] != null) {
        buffer.writeArrayGroup(
          entry,
          scriptInfo.info[entry] as Map<String, dynamic>,
        );
      }
    }
    if (equivalents.isNotEmpty) {
      buffer.writeArrayGroup('equivalents', equivalents);
    }

    buffer.write('  E\n');
  }

  void writeScriptDataHeader(String path) {
    final buffer = BinaryBuffer();
    buffer.write(headerPrefix);

    // scripts
    buffer.write("const char scriptData[] =\n");
    for (final entry in scriptInfoList) {
      writeScriptInfo(entry, buffer);
    }
    buffer.write(";\n\n");

    buffer.write(headerSuffix);

    final File genFile = File(path);
    genFile.writeAsStringSync(buffer.buffer.toString());
  }

  List<ScriptInfo> scriptInfoList = [];
  List<String> scriptList = [];
  final LatinEquivalents latinEquivalents;
}
