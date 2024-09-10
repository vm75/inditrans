import 'dart:io';

enum Case { capital, small }

class LatinLetter {
  final Case letterCase;
  final String letter;
  List<String> diacriticNames = [];
  List<String> diacritics = [];
  String? equivalent;

  LatinLetter(this.letterCase, this.letter);
}

class LatinEquivalents {
  LatinEquivalents(String path, {bool includeModifiers = false}) {
    final lines = File(path).readAsLinesSync();

    final Map<String, String> diacritics = {};
    final Map<String, String> modifiers = {};
    final Map<String, LatinLetter> latinExt = {};

    final diacriticRE = RegExp(r'^.*?\t(.*?)\tCombining (.*)');
    final modifierRE = RegExp(r'^.*?\t(.*?)\tModifier Letter (.*)');
    final latinExtRE = RegExp(
      r'^.*?\t(.*?)\tLatin (Small|Capital) Letter ([A-Za-z])(?: with (.*?))?(?: and (.*?))?(?: \(=(.*)\))?$',
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
          match!.group(2)!.toLowerCase() == 'small' ? Case.small : Case.capital,
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

    for (final entry in latinExt.entries) {
      if (entry.value.diacriticNames.isEmpty) {
        continue;
      }
      final d1 = diacritics[entry.value.diacriticNames[0]] ?? '';
      final m1 = includeModifiers
          ? modifiers[entry.value.diacriticNames[0]] ?? ''
          : '';
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
  }

  Map<int, List<String>> equivalents = {};
}
