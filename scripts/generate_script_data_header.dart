// ignore_for_file: avoid_print

import 'dart:convert';
import 'dart:io';

// ignore: leading_newlines_in_multiline_strings
const headerPrefix = '''#pragma once

#define Z "\\u0000"
#define E "\\u0001"

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

enum Case { capital, small }

class LatinLetter {
  final Case letterCase;
  final String letter;
  List<String> diacriticNames = [];
  List<String> diacritics = [];
  String? equivalent;

  LatinLetter(this.letterCase, this.letter);
}

Map<int, List<String>> loadLatinEquivalents({bool includeModifiers = false}) {
  final lines = File('docs/extended-latin.txt').readAsLinesSync();

  final Map<String, String> diacritics = {};
  final Map<String, String> modifiers = {};
  final Map<String, LatinLetter> latinExt = {};

  final diacriticRE = RegExp(r"^.*?\t(.*?)\tCombining (.*)");
  final modifierRE = RegExp(r"^.*?\t(.*?)\tModifier Letter (.*)");
  final latinExtRE = RegExp(
    r"^.*?\t(.*?)\tLatin (Small|Capital) Letter ([A-Za-z])(?: with (.*?))?(?: and (.*?))?(?: \(=(.*)\))?$",
  );

  for (final line in lines) {
    RegExpMatch? match;
    if ((match = diacriticRE.firstMatch(line)) != null) {
      diacritics[match!.group(2)!.toLowerCase()] = match.group(1)!;
    } else if (includeModifiers &&
        (match = modifierRE.firstMatch(line)) != null) {
      modifiers[match!.group(2)!.toLowerCase()] = match.group(1)!;
    } else if ((match = latinExtRE.firstMatch(line)) != null) {
      final letter = LatinLetter(
        match!.group(2)!.toLowerCase() == "small" ? Case.small : Case.capital,
        match.group(3)!.toLowerCase(),
      );
      if (match.group(4) != null) {
        letter.diacriticNames.add(match.group(4)!.toLowerCase());
      }
      if (match.group(5) != null) {
        letter.diacriticNames.add(match.group(5)!.toLowerCase());
      }
      if (match.group(6) != null) {
        letter.equivalent = match.group(6);
      }
      latinExt[match.group(1)!] = letter;
    }
  }

  final Map<int, List<String>> equivalents = {};
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
    equivalents[entry.key.codeUnits[0]] = entry.value.diacritics;
  }

  return equivalents;
}

class ScriptInfo {
  static const Map<String, int?> arrayTypes = {
    'aliases': null,
    'vowels': 19,
    'vowelMarks': 19,
    'consonants': 50,
    'otherDiacritics': 4,
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
    buffer.write('"$str" Z ');
  }

  void addEquivalents(
    String str,
    Map<dynamic, dynamic> equivalents,
    Map<int, List<String>> equivalentMap, {
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
      if (equivalentMap.containsKey(unit)) {
        parts.add(equivalentMap[unit]![0]);
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

  void writeArray(
    StringBuffer buffer,
    String type,
    List<dynamic> arr,
    int? len,
    Map<dynamic, dynamic> equivalents,
    Map<int, List<String>> equivalentMap, {
    required bool needsEquivalent,
    required bool upperEquivalent,
  }) {
    if (len != null && arr.length != len) {
      assert(arr.length == len);
    }
    buffer.write('    ${type.toUpperCase()} ');
    for (final entry in arr) {
      writeString(buffer, entry as String);
      if (needsEquivalent) {
        addEquivalents(entry, equivalents, equivalentMap,
            upperEquivalent: upperEquivalent);
      }
    }
    buffer.write('E\n');
  }

  void writeArrayGroup(
    StringBuffer buffer,
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
        writeString(buffer, equivalent as String);
      }
      buffer.write('E\n');
    }
    buffer.write('    E\n');
  }

  void write(StringBuffer buffer, Map<int, List<String>> equivalentMap) {
    buffer.write('  "$name" Z ${type.toUpperCase()}\n');
    final Map<dynamic, dynamic> equivalents =
        (info['equivalents'] ?? {}) as Map<dynamic, dynamic>;
    for (final entry in arrayTypes.entries) {
      if (info[entry.key] != null) {
        final needsEquivalent = type == 'latin';
        final upperEquivalents =
            name == 'iast' || name == 'ipa' || name == 'iso';
        writeArray(
          buffer,
          entry.key,
          info[entry.key] as List<dynamic>,
          entry.value,
          equivalents,
          equivalentMap,
          needsEquivalent: needsEquivalent,
          upperEquivalent: upperEquivalents,
        );
      }
    }
    for (final entry in arrayGroupTypes) {
      if (info[entry] != null) {
        writeArrayGroup(buffer, entry, info[entry] as Map<String, dynamic>);
      }
    }
    if (equivalents.isNotEmpty) {
      writeArrayGroup(buffer, 'equivalents', equivalents);
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
        ScriptInfo(type, entries.key, entries.value as Map<String, dynamic>),
      );
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

  final equivalentMap = loadLatinEquivalents();

  // scripts
  buffer.write("const char scriptData[] =\n");
  for (final entry in parseScriptInfo(jsonData)) {
    entry.write(buffer, equivalentMap);
  }
  buffer.write(";\n\n");

  buffer.write(headerSuffix);

  final File genFile = File('native/src/script_data.h');
  genFile.writeAsStringSync(buffer.toString());
}
