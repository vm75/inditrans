// ignore_for_file: avoid_print

import 'dart:io';

import 'utils/latin_equivalents.dart';
import 'utils/options_headers.dart';
import 'utils/script_data.dart';
import 'utils/scripts_headers.dart';

void main(List<String> args) async {
  final rootDir = Directory.current;

  final versionFile = File('${rootDir.path}/.version');
  if (!versionFile.existsSync()) {
    print('Run from root folder');
    return;
  }

  final latinEquivalents = LatinEquivalents('docs/extended-latin.txt');
  final scriptData = ScriptData('tool/script_data.json', latinEquivalents);

  scriptData.writeScriptDataHeader('native/src/script_data.h');

  final scriptsHeaders = ScriptsHeaders(scriptData.scriptList);
  scriptsHeaders.updateDart('flutter/lib/src/script.dart');
  scriptsHeaders.updateTypescript('nodejs/src/Script.ts');
  scriptsHeaders.updateJavascript('js/src/inditrans.post.js');

  final optionHeaders = OptionHeaders('tool/options.json');
  optionHeaders.updateNative('flutter/native/src/exports.h');
  optionHeaders.updateDart('flutter/lib/src/option.dart');
  optionHeaders.updateTypescript('nodejs/src/Option.ts');
  optionHeaders.updateJavascript('js/src/inditrans.post.js');
}
