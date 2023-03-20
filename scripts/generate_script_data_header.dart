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

enum Case { Capital, Small }

class LatinLetter {
  final Case letterCase;
  final String letter;
  List<String> diacriticNames = [];
  List<String> diacritics = [];
  String? equivalent;

  LatinLetter(this.letterCase, this.letter);
}

Map<int, List<String>> loadLatinAlternates([bool includeModifiers = false]) {
  final lines = File('docs/extended-latin.txt').readAsLinesSync();

  Map<String, String> diacritics = {};
  Map<String, String> modifiers = {};
  Map<String, LatinLetter> latinExt = {};

  final diacriticRE = RegExp(r"^.*?\t(.*?)\tCombining (.*)");
  final modifierRE = RegExp(r"^.*?\t(.*?)\tModifier Letter (.*)");
  final latinExtRE = RegExp(
      r"^.*?\t(.*?)\tLatin (Small|Capital) Letter ([A-Za-z])(?: with (.*?))?(?: and (.*?))?(?: \(=(.*)\))?$");

  for (final line in lines) {
    RegExpMatch? match;
    if ((match = diacriticRE.firstMatch(line)) != null) {
      diacritics[match!.group(2)!.toLowerCase()] = match.group(1)!;
    } else if (includeModifiers &&
        (match = modifierRE.firstMatch(line)) != null) {
      modifiers[match!.group(2)!.toLowerCase()] = match.group(1)!;
    } else if ((match = latinExtRE.firstMatch(line)) != null) {
      final letter = LatinLetter(
          match!.group(2)!.toLowerCase() == "small" ? Case.Small : Case.Capital,
          match.group(3)!.toLowerCase());
      if (match.group(4) != null) {
        letter.diacriticNames.add(match.group(4)!.toLowerCase());
      }
      if (match.group(5) != null) {
        letter.diacriticNames.add(match.group(5)!.toLowerCase());
      }
      if (match.group(6) != null) {
        letter.equivalent = match.group(6)!;
      }
      latinExt[match.group(1)!] = letter;
    }
  }

  Map<int, List<String>> alts = {};
  for (final entry in latinExt.entries) {
    if (entry.value.diacriticNames.isEmpty) {
      continue;
    }
    final d1 = diacritics[entry.value.diacriticNames[0]] ?? '';
    final m1 =
        includeModifiers ? modifiers[entry.value.diacriticNames[0]] ?? '' : '';
    String d2 = '';
    String m2 = '';
    if (entry.value.diacriticNames.length == 2) {
      d2 = diacritics[entry.value.diacriticNames[1]] ?? '';
      if (includeModifiers) {
        m2 = modifiers[entry.value.diacriticNames[1]] ?? '';
      }
    }
    if ('$d1$d2' == '') {
      continue;
    }
    entry.value.diacritics.add(entry.value.letter + d1 + d2);
    if (includeModifiers && '$m1$m2' != '') {
      entry.value.diacritics.add(entry.value.letter + m1 + m2);
    }
    if (entry.value.equivalent != null) {
      entry.value.diacritics.add(entry.value.letter + d2 + d1);
      if (includeModifiers && '$m1$m2' != '') {
        entry.value.diacritics.add(entry.value.letter + m2 + m1);
      }
    }
    assert(entry.key.codeUnits.length == 1);
    alts[entry.key.codeUnits[0]] = entry.value.diacritics;
  }

  return alts;
}

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
  ];
  final String type;
  final String name;
  final Map<String, dynamic> info;

  ScriptInfo(this.type, this.name, this.info);

  void writeString(StringBuffer buffer, String str) {
    buffer.write('"${str}" Z ');
  }

  void addAlternates(String str, Map<String, dynamic> alternates,
      Map<int, List<String>> altMap, bool upperAlt) {
    final altList = (alternates[str] ?? []) as List<dynamic>;
    bool needsAlt = false;

    if (upperAlt) {
      final altUpper = str.toUpperCase();
      if (altUpper != str && !altList.contains(altUpper)) {
        altList.add(altUpper);
        needsAlt = true;
      }
    }

    final codeUnits = str.codeUnits;
    List<String> parts = [];
    for (final unit in codeUnits) {
      if (altMap.containsKey(unit)) {
        parts.add(altMap[unit]![0]);
        needsAlt = true;
      } else {
        parts.add(String.fromCharCode(unit));
      }
    }
    if (!needsAlt) {
      return;
    }
    // join parts and add to alternates
    final altStr = parts.join();
    if (str != altStr && !altList.contains(altStr)) {
      altList.add(altStr);
      needsAlt = true;
      if (upperAlt) {
        final altUpper = altStr.toUpperCase();
        if (altUpper != altStr && !altList.contains(altUpper)) {
          altList.add(altUpper);
          needsAlt = true;
        }
      }
    }
    if (needsAlt) {
      alternates[str] = altList;
    }
  }

  void writeArray(
      StringBuffer buffer,
      String type,
      List<dynamic> arr,
      int? len,
      Map<String, dynamic> alternates,
      Map<int, List<String>> altMap,
      bool needsAlt,
      bool upperAlt) {
    if (len != null && arr.length != len) {
      assert(arr.length == len);
    }
    buffer.write('    ${type.toUpperCase()} ');
    for (final entry in arr) {
      writeString(buffer, entry);
      if (needsAlt) {
        addAlternates(entry, alternates, altMap, upperAlt);
      }
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

  void write(StringBuffer buffer, Map<int, List<String>> altMap) {
    buffer.write('  "$name" Z ${type.toUpperCase()}\n');
    final Map<String, dynamic> alternates = info['alternates'] ?? {};
    for (final entry in arrayTypes.entries) {
      if (info[entry.key] != null) {
        bool needsAlt = type == 'latin';
        bool upperAlts = name == 'iast' || name == 'ipa' || name == 'iso';
        writeArray(buffer, entry.key, info[entry.key] as List<dynamic>,
            entry.value, alternates, altMap, needsAlt, upperAlts);
      }
    }
    for (final entry in arrayGroupTypes) {
      if (info[entry] != null) {
        writeArrayGroup(buffer, entry, info[entry]);
      }
    }
    if (alternates.isNotEmpty) {
      writeArrayGroup(buffer, 'alternates', alternates);
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

  final altMap = loadLatinAlternates();

  // scripts
  buffer.write("const char scriptData[] =\n");
  for (final entry in parseScriptInfo(jsonData)) {
    entry.write(buffer, altMap);
  }
  buffer.write(";\n\n");

  // // latin alternates
  // buffer.write("const char latinAlternates[] =\n");
  // for (final entry in altMap.entries) {
  //   writeAlternate(entry, buffer);
  // }
  // buffer.write(";\n\n");

  buffer.write(headerSuffix);

  File genFile = File('native/src/script_data.h');
  genFile.writeAsStringSync(buffer.toString());
}

void writeAlternate(MapEntry<int, List<String>> entry, StringBuffer buffer) {
  buffer.write('  "${String.fromCharCode(entry.key)}" Z ');
  for (final alt in entry.value) {
    buffer.write('"${alt}" Z ');
  }
  buffer.write('E\n');
}
