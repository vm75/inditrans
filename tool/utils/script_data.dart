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

class MetadataRange {
  final int offset;
  final int count;

  MetadataRange(this.offset, this.count);
}

class MetadataGroup {
  final String key;
  final MetadataRange values;

  MetadataGroup(this.key, this.values);
}

class GeneratedScriptMetadata {
  final String name;
  final String type;
  final bool isVedic;
  final MetadataRange vowels;
  final MetadataRange vowelMarks;
  final MetadataRange consonants;
  final MetadataRange otherDiacritics;
  final MetadataRange symbols;
  final MetadataRange vedicSymbols;
  final MetadataRange aliases;
  final MetadataRange equivalents;
  final MetadataRange languages;

  GeneratedScriptMetadata({
    required this.name,
    required this.type,
    required this.isVedic,
    required this.vowels,
    required this.vowelMarks,
    required this.consonants,
    required this.otherDiacritics,
    required this.symbols,
    required this.vedicSymbols,
    required this.aliases,
    required this.equivalents,
    required this.languages,
  });
}

class BinaryBuffer {
  BinaryBuffer() : buffer = StringBuffer();

  void write(String str) {
    buffer.write(str);
  }

  void writeString(String str) {
    final escapedStr = str.replaceAll('"', '\\"');
    buffer.write('"$escapedStr" Z ');
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
      for (final equivalent in entry.value as List<dynamic>) {
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
    buffer.write('const char scriptData[] =\n');
    for (final entry in scriptInfoList) {
      writeScriptInfo(entry, buffer);
    }
    buffer.write(';\n\n');

    buffer.write(headerSuffix);

    final File genFile = File(path);
    genFile.writeAsStringSync(buffer.buffer.toString());

    writeScriptMetadataHeader('native/src/script_metadata.h');
  }

  void writeScriptMetadataHeader(String path) {
    final vowels = <String>[];
    final vowelMarks = <String>[];
    final consonants = <String>[];
    final otherDiacritics = <String>[];
    final symbols = <String>[];
    final vedicSymbols = <String>[];
    final aliases = <String>[];
    final equivalentGroups = <MetadataGroup>[];
    final equivalentValues = <String>[];
    final languageGroups = <MetadataGroup>[];
    final languageValues = <String>[];
    final scripts = <GeneratedScriptMetadata>[];

    MetadataRange appendValues(List<String> target, Iterable<dynamic>? source) {
      final offset = target.length;
      if (source != null) {
        target.addAll(source.cast<String>());
      }
      return MetadataRange(offset, target.length - offset);
    }

    MetadataRange appendGroups(
      List<MetadataGroup> groups,
      List<String> values,
      Map<dynamic, dynamic>? source,
    ) {
      final offset = groups.length;
      if (source != null) {
        final entries = source.entries.toList()
          ..sort((a, b) => (a.key as String).compareTo(b.key as String));
        for (final entry in entries) {
          final range = appendValues(values, (entry.value as List<dynamic>));
          groups.add(MetadataGroup(entry.key as String, range));
        }
      }
      return MetadataRange(offset, groups.length - offset);
    }

    for (final entry in scriptInfoList) {
      final info = entry.info;
      final String typeCode;
      if (entry.type == 'vedic') {
        typeCode = 'v';
      } else if (entry.type == 'indic') {
        typeCode = 'i';
      } else if (entry.type == 'tamil') {
        typeCode = 't';
      } else if (entry.type == 'latin') {
        typeCode = 'l';
      } else {
        throw StateError('Unknown script type: ${entry.type}');
      }
      scripts.add(GeneratedScriptMetadata(
        name: entry.name,
        type: typeCode,
        isVedic: entry.type == 'vedic',
        vowels: appendValues(vowels, info['vowels'] as List<dynamic>?),
        vowelMarks:
            appendValues(vowelMarks, info['vowelMarks'] as List<dynamic>?),
        consonants:
            appendValues(consonants, info['consonants'] as List<dynamic>?),
        otherDiacritics: appendValues(
          otherDiacritics,
          info['otherDiacritics'] as List<dynamic>?,
        ),
        symbols: appendValues(symbols, info['symbols'] as List<dynamic>?),
        vedicSymbols:
            appendValues(vedicSymbols, info['vedicSymbols'] as List<dynamic>?),
        aliases: appendValues(aliases, info['aliases'] as List<dynamic>?),
        equivalents: appendGroups(
          equivalentGroups,
          equivalentValues,
          info['equivalents'] as Map<dynamic, dynamic>?,
        ),
        languages: appendGroups(
          languageGroups,
          languageValues,
          info['languages'] as Map<dynamic, dynamic>?,
        ),
      ));
    }

    final buffer = StringBuffer('''#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace inditrans::generated {

struct ScriptMetadataRange {
  uint32_t offset;
  uint32_t count;
};

struct ScriptMetadataGroup {
  std::string_view key;
  ScriptMetadataRange values;
};

struct ScriptMetadataRecord {
  std::string_view name;
  char type;
  bool isVedic;
  ScriptMetadataRange vowels;
  ScriptMetadataRange vowelMarks;
  ScriptMetadataRange consonants;
  ScriptMetadataRange otherDiacritics;
  ScriptMetadataRange symbols;
  ScriptMetadataRange vedicSymbols;
  ScriptMetadataRange aliases;
  ScriptMetadataRange equivalents;
  ScriptMetadataRange languages;
};

''');

    void writeStringArray(String name, List<String> values) {
      buffer.writeln(
        'inline constexpr std::array<std::string_view, ${values.length}> $name = {',
      );
      for (final value in values) {
        final escaped = value
            .replaceAll(r'\', r'\\')
            .replaceAll('"', r'\"')
            .replaceAll('\n', r'\n')
            .replaceAll('\r', r'\r')
            .replaceAll('\t', r'\t');
        buffer.writeln('  "$escaped",');
      }
      buffer.writeln('};\n');
    }

    void writeGroupArray(String name, List<MetadataGroup> groups) {
      buffer.writeln(
        'inline constexpr std::array<ScriptMetadataGroup, ${groups.length}> $name = {',
      );
      for (final group in groups) {
        final escaped = group.key
            .replaceAll(r'\', r'\\')
            .replaceAll('"', r'\"')
            .replaceAll('\n', r'\n')
            .replaceAll('\r', r'\r')
            .replaceAll('\t', r'\t');
        buffer.writeln(
          '  ScriptMetadataGroup{"$escaped", '
          '{${group.values.offset}, ${group.values.count}}},',
        );
      }
      buffer.writeln('};\n');
    }

    writeStringArray('scriptVowels', vowels);
    writeStringArray('scriptVowelMarks', vowelMarks);
    writeStringArray('scriptConsonants', consonants);
    writeStringArray('scriptOtherDiacritics', otherDiacritics);
    writeStringArray('scriptSymbols', symbols);
    writeStringArray('scriptVedicSymbols', vedicSymbols);
    writeStringArray('scriptAliases', aliases);
    writeStringArray('scriptEquivalentValues', equivalentValues);
    writeStringArray('scriptLanguageValues', languageValues);
    writeGroupArray('scriptEquivalentGroups', equivalentGroups);
    writeGroupArray('scriptLanguageGroups', languageGroups);

    buffer.writeln(
      'inline constexpr std::array<ScriptMetadataRecord, ${scripts.length}> scriptMetadataScripts = {',
    );
    for (final script in scripts) {
      String range(MetadataRange value) => '{${value.offset}, ${value.count}}';
      buffer.writeln(
        '  ScriptMetadataRecord{"${script.name}", \'${script.type}\', '
        '${script.isVedic ? 'true' : 'false'}, ${range(script.vowels)}, '
        '${range(script.vowelMarks)}, ${range(script.consonants)}, '
        '${range(script.otherDiacritics)}, ${range(script.symbols)}, '
        '${range(script.vedicSymbols)}, ${range(script.aliases)}, '
        '${range(script.equivalents)}, ${range(script.languages)}},',
      );
    }
    buffer.writeln('};\n\n} // namespace inditrans::generated\n');

    File(path).writeAsStringSync(buffer.toString());
  }

  List<ScriptInfo> scriptInfoList = [];
  List<String> scriptList = [];
  final LatinEquivalents latinEquivalents;
}
