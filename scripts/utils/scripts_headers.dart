import 'dart:io';

import 'string_utils.dart';

class ScriptsHeaders {
  ScriptsHeaders(this.scripts) {
    scripts.sort();
  }

  void updateDart(String path) {
    final StringBuffer buffer = StringBuffer();
    for (final String script in scripts) {
      buffer.writeln('  $script,');
    }
    replaceByDelimiters(
      path,
      "enum Script {",
      "}",
      buffer,
    );
  }

  void updateTypescript(String path) {
    final StringBuffer buffer = StringBuffer();
    buffer.writeln('// GENERATED CODE - DO NOT MODIFY BY HAND');
    buffer.writeln();
    buffer.writeln('/// Supported scripts');
    buffer.writeln('export enum Script {');
    for (final String script in scripts) {
      buffer.writeln("  $script = '$script',");
    }
    buffer.writeln('}');

    final File genFile = File(path);
    genFile.writeAsStringSync(buffer.toString());
  }

  void updateJavascript(String path) {
    final StringBuffer buffer = StringBuffer();
    for (final script in scripts) {
      buffer.writeln(
        "            '$script',",
      );
    }

    replaceByDelimiters(
      path,
      "'Scripts': [\n",
      "        ],",
      buffer,
    );
  }

  final List<String> scripts;
}
