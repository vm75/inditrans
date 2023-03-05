import 'dart:convert';
import 'dart:io';

const HEADER = '''#pragma once

#include <array>
#include <string>

enum class ScriptType : uint8_t { Tamil, Brahmi, Roman, Others };

struct ScriptData {
  ScriptType type {};
  std::array<std::string_view, 19> vowels;
  std::array<std::string_view, 19> vowelDiacritics;
  std::array<std::string_view, 50> consonants;
  std::array<std::string_view, 5> consonantDiacritic;
  std::array<std::string_view, 19> symbols;
};

// clang-format off
constexpr std::array<std::pair<std::string_view, ScriptData>, 118> ScriptDataMap {{
''';

const FOOTER = '''}};
// clang-format on
''';

class ScriptInfo {
  final String type;
  final String name;
  final List<dynamic> info;

  ScriptInfo(this.type, this.name, this.info);

  void writeArray(StringBuffer buffer, List<dynamic> arr) {
    buffer.write('    { ');
    bool first = true;
    for (final entry in arr) {
      if (!first) {
        buffer.write(', ');
      }
      first = false;
      buffer.write('"');
      if (entry is String) {
        buffer.write(entry.replaceAll('"', '\\"'));
      } else {
        buffer.write(entry);
      }
      buffer.write('"');
    }
    buffer.write(' }');
  }

  void write(StringBuffer buffer, bool first) {
    if (!first) {
      buffer.write(',\n');
    }
    first = false;
    buffer.write('  { "$name", {\n    ScriptType::$type,\n');
    bool firstArray = true;
    for (final arr in info) {
      if (!firstArray) {
        buffer.write(',\n');
      }
      firstArray = false;
      writeArray(buffer, arr);
    }
    buffer.write('\n  }}');
  }
}

List<ScriptInfo> parseScriptInfo(Map<String, dynamic> map) {
  final List<ScriptInfo> list = [];
  for (final entries in map.entries) {
    final type = entries.key;
    final scripts = entries.value as Map<String, dynamic>;
    for (final entries in scripts.entries) {
      list.add(ScriptInfo(type, entries.key, entries.value as List<dynamic>));
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
      jsonDecode(File('nodejs/assets/scripts.json').readAsStringSync())
          as Map<String, dynamic>;

  final List<ScriptInfo> primary = parseScriptInfo(inditransScripts['Primary']);
  final List<ScriptInfo> secondary =
      parseScriptInfo(inditransScripts['Secondary']);

  final buffer = StringBuffer();
  buffer.write(HEADER);

  var first = true;
  for (final entry in primary) {
    entry.write(buffer, first);
    first = false;
  }

  buffer.write(''',
#ifndef __EMSCRIPTEN__
''');
  first = true;
  for (final entry in secondary) {
    entry.write(buffer, first);
    first = false;
  }

  buffer.write('''

#endif
''');

  buffer.write(FOOTER);

  File genFile = File('flutter/native/src/scripts-gen.h');
  genFile.writeAsStringSync(buffer.toString());
}
