// ignore_for_file: avoid_print

import 'dart:convert';
import 'dart:io';

import 'package:http/http.dart' as http;

class Script {
  final String name;
  List<String> vowels = [];
  List<String> vowelDiacritics = [];
  List<String> consonants = [];
  List<String> commonDiacritics = [];
  List<String> symbols = [];
  List<String> vedicSymbols = [];
  Script(this.name, Map<String, dynamic> json) {
    populateVowels(json);
    populateVowelDiacritics(json);
    populateConsonants(json);
    populateCommonDiacritics(json);
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
    final parts = path.split(".");
    late Map<String, dynamic> next = json;
    for (final part in parts) {
      next = next[part] as Map<String, dynamic>;
    }

    final src = next as List<dynamic>;
    for (int idx = idxStart; idx < idxStart + count; idx++) {
      dest.add(src[idx] as String);
    }
  }

  void populateVowels(Map<String, dynamic> json) {
    copy(vowels, json, "vowels.main", 0, 10);
    copy(vowels, json, "vowels.south", 0);
    copy(vowels, json, "vowels.main", 10, 2);
    copy(vowels, json, "vowels.south", 1);
    copy(vowels, json, "vowels.main", 12, 2);
    copy(vowels, json, "vowels.modern", 0);
    copy(vowels, json, "vowels.sinhala", 0);
    copy(vowels, json, "vowels.modern", 1);
  }

  void populateVowelDiacritics(Map<String, dynamic> json) {
    copy(vowelDiacritics, json, "vowelsigns.virama", 0);
    if (vowelDiacritics[0] == "×") {
      vowelDiacritics[0] = "";
    }
    copy(vowelDiacritics, json, "vowelsigns.main", 0, 9);
    copy(vowelDiacritics, json, "vowelsigns.south", 0);
    copy(vowelDiacritics, json, "vowelsigns.main", 9, 2);
    copy(vowelDiacritics, json, "vowelsigns.south", 1);
    copy(vowelDiacritics, json, "vowelsigns.main", 11, 2);
    copy(vowelDiacritics, json, "vowelsigns.modern", 0);
    copy(vowelDiacritics, json, "vowelsigns.sinhala", 0);
    copy(vowelDiacritics, json, "vowelsigns.modern", 1);
  }

  void populateConsonants(Map<String, dynamic> json) {
    copy(consonants, json, "consonants.main", 0, 33);
    copy(consonants, json, "consonants.south", 0, 4);
    copy(consonants, json, "consonants.persoarabic", 0, 8);
    copy(consonants, json, "consonants.sinhala", 0, 5);
  }

  void populateCommonDiacritics(Map<String, dynamic> json) {
    copy(commonDiacritics, json, "combiningsigns.ayogavaha", 0, 3);
    copy(commonDiacritics, json, "others.aytham", 0);
  }

  void populateSymbols(Map<String, dynamic> json) {
    copy(symbols, json, "numerals", 0, 10);
    copy(symbols, json, "others.om", 0);
    copy(symbols, json, "others.symbols", 1, 2);
  }

  void populateVedicSymbols(Map<String, dynamic> json) {
    copy(vedicSymbols, json, "others.symbols", 0);
    if (vowelDiacritics[0] == "") {
      vedicSymbols.addAll(["gͫ", "gͫ̄"]);
    } else {
      vedicSymbols.addAll(["ꣳ", "ꣴ"]);
    }
  }

  void show() {
    print('    "$name": {');
    print('      "vowels": [ "${vowels.join('", "')}" ],');
    print('      "vowelDiacritics": [ "${vowelDiacritics.join('", "')}" ],');
    print('      "consonants": [ "${consonants.join('", "')}" ],');
    print('      "commonDiacritics": [ "${commonDiacritics.join('", "')}" ],');
    print('      "symbols": [ "${symbols.join('", "')}" ],');
    print('      "vedicSymbols": [ "${vedicSymbols.join('", "')}" ],');
    print('      "alternates": {');
    print('        "z:0": [ "()", "^" ],');
    print('        "z:1": [ "{}", "^^" ]');
    print('      }');
    print('    },');
  }
}

// accept args
void main(List<String> args) async {
  final rootDir = Directory.current;

  final versionFile = File("${rootDir.path}/.version");
  if (!versionFile.existsSync()) {
    print("Run from root folder");
    return;
  }

  if (args.isEmpty) {
    print("Usage: dart import_script.dart <script>");
    return;
  }

  final url = Uri.parse(
    'https://raw.githubusercontent.com/virtualvinodh/aksharamukha/master/aksharamukha-back/resources/script_mapping/script_mapping.json',
  );
  final response = await http.get(url);
  final jsonResponse = json.decode(response.body) as Map<String, dynamic>;
  if (jsonResponse[args[0]] == null) {
    print("Script not found");
    return;
  }
  final scriptJson = jsonResponse[args[0]];
  final script = Script(args[0], scriptJson as Map<String, dynamic>);
  script.show();
}
