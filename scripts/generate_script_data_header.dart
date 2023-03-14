// ignore_for_file: avoid_print

import 'dart:convert';
import 'dart:io';

const headerPrefix = '''#pragma once

#define Z "\\u0000"
#define E "\\u0001"

#define INDIC "i\\u0000"
#define TAMIL "t\\u0000"
#define LATIN "l\\u0000"

#define LANGUAGES "l\\u0000"
#define VOWELS "v\\u0000"
#define VOWELDIACRITICS "V\\u0000"
#define CONSONANTS "c\\u0000"
#define CONSONANTDIACRITICS "C\\u0000"
#define SYMBOLS "s\\u0000"
#define ALTERNATES "a\\u0000"

// clang-format off

const char scriptData[] =
''';

const headerSuffix = ''';

// clang-format on
''';

class ScriptInfo {
  static const Map<String, int?> arrayTypes = {
    'languages': null,
    'vowels': 19,
    'vowelDiacritics': 19,
    'consonants': 50,
    'consonantDiacritics': 4,
    'symbols': 19
  };
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

  void writeAlternates(StringBuffer buffer, List<dynamic> alts) {
    if (alts.isEmpty) {
      return;
    }
    buffer.write('    ALTERNATES\n');
    for (final entry in alts) {
      final alt = entry as Map<String, dynamic>;
      final type = alt['type'] as String;
      final idx = alt['idx'] as int;
      final value = alt['alt'] as String;
      buffer.write('      ${type.toUpperCase()} "$idx" Z "$value" Z E\n');
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
    if (info['alternates'] != null) {
      writeAlternates(buffer, info['alternates']);
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

  final inditransScripts =
      jsonDecode(File('scripts/script_data.json').readAsStringSync())
          as Map<String, dynamic>;

  final buffer = StringBuffer();
  buffer.write(headerPrefix);

  for (final entry in parseScriptInfo(inditransScripts)) {
    entry.write(buffer);
  }

  buffer.write(headerSuffix);

  File genFile = File('native/src/script_data.h');
  genFile.writeAsStringSync(buffer.toString());
}
