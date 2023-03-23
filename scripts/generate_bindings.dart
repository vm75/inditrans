// ignore_for_file: avoid_print

import 'dart:io';

void main(List<String> args) async {
  final rootDir = Directory.current;

  final versionFile = File("${rootDir.path}/.version");
  if (!versionFile.existsSync()) {
    print("Run from root folder");
    return;
  }

  Directory.current = Directory('flutter');
  final result =
      await Process.run('dart', ['run', 'ffigen', '--config', 'ffigen.yaml']);

  stdout.write(result.stdout);
  if (result.exitCode != 0) {
    stderr.write(result.stderr);
    return;
  }

  final file = File('lib/src/bindings.dart');
  final contents = file.readAsStringSync();
  file.writeAsStringSync(
    contents.replaceAll(
      "import 'dart:ffi' as ffi",
      "import 'ffi_proxy.dart' as ffi",
    ),
  );
}
