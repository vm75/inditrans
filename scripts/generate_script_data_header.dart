// ignore_for_file: avoid_print

import 'dart:convert';
import 'dart:io';

const headerPrefix = '''#pragma once

#define Z "\\u0000"
#define E "\\u0001"

#define INDIC "i\\u0000"
#define TAMIL "t\\u0000"
#define LATIN "l\\u0000"

#define VOWELS "v\\u0000"
#define VOWELDIACRITICS "V\\u0000"
#define CONSONANTS "c\\u0000"
#define COMMONDIACRITICS "C\\u0000"
#define SYMBOLS "s\\u0000"
#define VEDICSYMBOLS "S\\u0000"
#define ALTERNATES "a\\u0000"
#define ALIASES "A\\u0000"
#define LANGUAGES "l\\u0000"

// clang-format off

''';

const headerSuffix = '''
// clang-format on
''';

class ScriptInfo {
  static const Map<String, int?> arrayTypes = {
    'aliases': null,
    'vowels': 19,
    'vowelDiacritics': 19,
    'consonants': 50,
    'commonDiacritics': 4,
    'symbols': 13,
    'vedicSymbols': 3,
  };
  static const List<String> arrayGroupTypes = [
    'languages',
    'alternates',
  ];
  final String type;
  final String name;
  final Map<String, dynamic> info;

  ScriptInfo(this.type, this.name, this.info);

  void writeString(StringBuffer buffer, String str) {
    buffer.write('"${str}" Z ');
  }

  void writeArray(StringBuffer buffer, String type, List<dynamic> arr,
      [int? len]) {
    if (len != null) {
      assert(arr.length == len);
    }
    buffer.write('    ${type.toUpperCase()} ');
    for (final entry in arr) {
      writeString(buffer, entry);
    }
    buffer.write('E\n');
  }

  void writeArrayGroup(
      StringBuffer buffer, String type, Map<String, dynamic> alts) {
    if (alts.isEmpty) {
      return;
    }

    buffer.write('    ${type.toUpperCase()}\n');
    for (final entry in alts.entries) {
      buffer.write('      "${entry.key}" Z ');
      for (final alt in entry.value) {
        writeString(buffer, alt);
      }
      buffer.write('E\n');
    }
    buffer.write('    E\n');
  }

  void write(StringBuffer buffer) {
    buffer.write('  "$name" Z ${type.toUpperCase()}\n');
    for (final entry in arrayTypes.entries) {
      if (info[entry.key] != null) {
        writeArray(
            buffer, entry.key, info[entry.key] as List<dynamic>, entry.value);
      }
    }
    for (final entry in arrayGroupTypes) {
      if (info[entry] != null) {
        writeArrayGroup(buffer, entry, info[entry]);
      }
    }

    buffer.write('  E\n');
  }
}

List<ScriptInfo> parseScriptInfo(Map<String, dynamic> map) {
  final List<ScriptInfo> list = [];
  for (final entries in map.entries) {
    final type = entries.key;
    final scripts = entries.value as Map<String, dynamic>;
    for (final entries in scripts.entries) {
      list.add(
          ScriptInfo(type, entries.key, entries.value as Map<String, dynamic>));
    }
  }
  list.sort((a, b) => a.name.compareTo(b.name));
  return list;
}

void main(List<String> args) async {
  final rootDir = Directory.current;

  final versionFile = File("${rootDir.path}/.version");
  if (!versionFile.existsSync()) {
    print("Run from root folder");
    return;
  }

  final jsonData =
      jsonDecode(File('scripts/script_data.json').readAsStringSync())
          as Map<String, dynamic>;

  final buffer = StringBuffer();
  buffer.write(headerPrefix);

  // scripts
  buffer.write("const char scriptData[] =\n");
  for (final entry in parseScriptInfo(jsonData)) {
    entry.write(buffer);
  }
  buffer.write(";\n\n");

  buffer.write(headerSuffix);

  File genFile = File('native/src/script_data.h');
  genFile.writeAsStringSync(buffer.toString());
}
