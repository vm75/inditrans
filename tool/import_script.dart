// ignore_for_file: avoid_print, unreachable_from_main

import 'dart:convert';
import 'dart:io';

import 'package:http/http.dart' as http;

class Script {
  final String name;
  List<String> vowels = [];
  List<String> vowelMarks = [];
  List<String> consonants = [];
  List<String> otherDiacritics = [];
  List<String> symbols = [];
  List<String> vedicSymbols = [];
  Script(this.name, Map<String, dynamic> json) {
    populateVowels(json);
    populateVowelMarks(json);
    populateConsonants(json);
    populateOtherDiacritics(json);
    populateSymbols(json);
    populateVedicSymbols(json);
  }

  void copy(
    List<String> dest,
    Map<String, dynamic> json,
    String path,
    int idxStart, [
    int count = 1,
  ]) {
    final parts = path.split('.');
    dynamic current = json;
    for (final part in parts) {
      if (current is Map<String, dynamic>) {
        current = current[part];
      }
    }

    final src = current as List<dynamic>;
    for (int idx = idxStart; idx < idxStart + count; idx++) {
      dest.add(src[idx] as String);
    }
  }

  void populateVowels(Map<String, dynamic> json) {
    copy(vowels, json, 'vowels.main', 0, 10);
    copy(vowels, json, 'vowels.south', 0);
    copy(vowels, json, 'vowels.main', 10, 2);
    copy(vowels, json, 'vowels.south', 1);
    copy(vowels, json, 'vowels.main', 12, 2);
    copy(vowels, json, 'vowels.modern', 0);
    copy(vowels, json, 'vowels.sinhala', 0);
    copy(vowels, json, 'vowels.modern', 1);
  }

  void populateVowelMarks(Map<String, dynamic> json) {
    copy(vowelMarks, json, 'vowelsigns.virama', 0);
    if (vowelMarks[0] == '×') {
      vowelMarks[0] = '';
    }
    copy(vowelMarks, json, 'vowelsigns.main', 0, 9);
    copy(vowelMarks, json, 'vowelsigns.south', 0);
    copy(vowelMarks, json, 'vowelsigns.main', 9, 2);
    copy(vowelMarks, json, 'vowelsigns.south', 1);
    copy(vowelMarks, json, 'vowelsigns.main', 11, 2);
    copy(vowelMarks, json, 'vowelsigns.modern', 0);
    copy(vowelMarks, json, 'vowelsigns.sinhala', 0);
    copy(vowelMarks, json, 'vowelsigns.modern', 1);
  }

  void populateConsonants(Map<String, dynamic> json) {
    copy(consonants, json, 'consonants.main', 0, 33);
    copy(consonants, json, 'consonants.south', 0, 4);
    copy(consonants, json, 'consonants.persoarabic', 0, 8);
    copy(consonants, json, 'consonants.sinhala', 0, 5);
  }

  void populateOtherDiacritics(Map<String, dynamic> json) {
    copy(otherDiacritics, json, 'combiningsigns.ayogavaha', 0, 3);
    copy(otherDiacritics, json, 'others.aytham', 0);
  }

  void populateSymbols(Map<String, dynamic> json) {
    copy(symbols, json, 'numerals', 0, 10);
    copy(symbols, json, 'others.om', 0);
    copy(symbols, json, 'others.symbols', 1, 2);
  }

  void populateVedicSymbols(Map<String, dynamic> json) {
    copy(vedicSymbols, json, 'others.symbols', 0);
    if (vowelMarks[0] == '') {
      vedicSymbols.addAll(['gͫ', 'gͫ̄']);
    } else {
      vedicSymbols.addAll(['ꣳ', 'ꣴ']);
    }
  }

  Map<String, dynamic> toJson() {
    return {
      'vowels': vowels,
      'vowelMarks': vowelMarks,
      'consonants': consonants,
      'otherDiacritics': otherDiacritics,
      'symbols': symbols,
      'vedicSymbols': vedicSymbols,
      'equivalents': {
        'x:0': ['()', '^'],
        'x:1': ['{}', '^^'],
      },
    };
  }
}

// Custom JSON formatter to keep arrays on a single line
String formatJson(dynamic object, [String indent = '']) {
  if (object is Map) {
    if (object.isEmpty) return '{}';
    final buffer = StringBuffer('{\n');
    final keys = object.keys.toList();
    for (int i = 0; i < keys.length; i++) {
      final key = keys[i];
      final value = object[key];
      buffer.write('$indent  "$key": ${formatJson(value, '$indent  ')}');
      if (i < keys.length - 1) buffer.write(',');
      buffer.write('\n');
    }
    buffer.write('$indent}');
    return buffer.toString();
  } else if (object is List) {
    if (object.isEmpty) return '[]';
    final content = object.map((e) => json.encode(e)).join(', ');
    return '[ $content ]';
  } else {
    return json.encode(object);
  }
}

void main(List<String> args) async {
  final rootDir = Directory.current;
  final validCategories = ['vedic', 'indic', 'latin', 'tamil'];

  if (args.length < 2) {
    print('Usage: dart import_script.dart <category> <script1> [script2] ...');
    print('Categories: ${validCategories.join(', ')}');
    return;
  }

  final category = args[0].toLowerCase();
  if (!validCategories.contains(category)) {
    print('❌ Invalid category: $category');
    print('Valid categories: ${validCategories.join(', ')}');
    return;
  }

  final scriptsToImport = args.sublist(1);

  // Load existing script_data.json
  final scriptDataFile = File('${rootDir.path}/tool/script_data.json');
  Map<String, dynamic> scriptData = {};

  if (scriptDataFile.existsSync()) {
    final content = await scriptDataFile.readAsString();
    scriptData = json.decode(content) as Map<String, dynamic>;
    print('Loaded existing script_data.json');
  } else {
    scriptData = {for (var c in validCategories) c: {}};
    print('Creating new script_data.json');
  }

  // Fetch remote script mapping
  final url = Uri.parse(
    'https://raw.githubusercontent.com/virtualvinodh/aksharamukha/master/aksharamukha-back/resources/script_mapping/script_mapping.json',
  );
  print('Fetching script mapping from remote...');
  final response = await http.get(url);
  final jsonResponse = json.decode(response.body) as Map<String, dynamic>;

  int addedCount = 0;
  int skippedCount = 0;

  for (final scriptName in scriptsToImport) {
    if (jsonResponse[scriptName] == null) {
      print('❌ Script "$scriptName" not found in remote mapping');
      continue;
    }

    // Check if script already exists in any category
    bool exists = false;
    for (final cat in scriptData.values) {
      if (cat is Map<String, dynamic> && cat.containsKey(scriptName)) {
        exists = true;
        break;
      }
    }

    if (exists) {
      print('⏭️  Script "$scriptName" already exists, skipping');
      skippedCount++;
      continue;
    }

    try {
      final scriptJson = jsonResponse[scriptName] as Map<String, dynamic>;
      final script = Script(scriptName, scriptJson);

      // Add script to specified category
      (scriptData[category] as Map<String, dynamic>)[scriptName] =
          script.toJson();

      print('✅ Added "$scriptName" to category "$category"');
      addedCount++;
    } catch (e) {
      print('❌ Error importing "$scriptName": $e');
    }
  }

  if (addedCount > 0) {
    final jsonString = formatJson(scriptData);
    await scriptDataFile.writeAsString('$jsonString\n');
    print('\n✨ Successfully saved script_data.json');
    print('   Added: $addedCount scripts to "$category"');
  } else {
    print('\n⚠️  No new scripts were added');
  }
}
