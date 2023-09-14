import 'dart:convert';
import 'dart:io';

import 'string_utils.dart';

class OptionInfo {
  String name;
  String comment;
  int value;
  OptionInfo(this.name, this.comment, this.value);
}

class OptionHeaders {
  OptionHeaders(String path) {
    final jsonData = jsonDecode(File(path).readAsStringSync()) as List<dynamic>;

    for (final entry in jsonData) {
      final optInfo = entry as Map<String, dynamic>;
      final name = optInfo['name'] as String;
      final comment = optInfo['comment'] as String;
      final value = optInfo['value'] as int;
      options.add(OptionInfo(name, comment, value));
    }
  }

  void updateDart(String path) {
    final StringBuffer buffer = StringBuffer();
    for (final option in options) {
      buffer.writeln('  /// ${option.comment}');
      buffer.writeln(
        "  static final ${option.name} = Option('${option.name}');",
      );
      buffer.writeln();
    }
    replaceByDelimiters(
      path,
      "class Option {",
      "  /// Returns the int value of the option flag",
      buffer,
    );
  }

  void updateTypescript(String path) {
    final StringBuffer buffer = StringBuffer();
    buffer.writeln('// GENERATED CODE - DO NOT MODIFY BY HAND');
    buffer.writeln();
    buffer.writeln('/// Transliteration options');
    buffer.writeln('/// Flags to control transliteration');
    buffer.write('export enum Option {');
    for (final option in options) {
      buffer.writeln();
      buffer.writeln('  /// ${option.comment}');
      buffer.writeln(
        '  ${option.name} = ${option.value},',
      );
    }
    buffer.writeln('}');

    final File genFile = File(path);
    genFile.writeAsStringSync(buffer.toString());
  }

  void updateNative(String path) {
    final genFile = File(path);
    final contents = genFile.readAsStringSync();
    final enumStart = contents.indexOf('enum TranslitOptions {');
    final enumEnd = contents.indexOf('};', enumStart);

    final prefix =
        contents.substring(0, enumStart + 'enum TranslitOptions {'.length);
    final suffix = contents.substring(enumEnd);

    final StringBuffer buffer = StringBuffer();
    buffer.writeln(prefix);
    for (final option in options) {
      buffer.writeln('  /// ${option.comment}');
      buffer.writeln(
        '  ${option.name} = ${option.value},',
      );
    }
    buffer.write(suffix);

    genFile.writeAsStringSync(buffer.toString());
  }

  void updateJavascript(String path) {
    final StringBuffer buffer = StringBuffer();
    for (final option in options) {
      buffer.writeln(
        "            '${option.name}': ${option.value},",
      );
    }

    replaceByDelimiters(
      path,
      "'Options': {\n",
      "        },",
      buffer,
    );
  }

  List<OptionInfo> options = [];
}
