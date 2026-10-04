import 'dart:convert';
import 'dart:io';

import 'script_data.dart';

/// Resolves the legacy insertion/expansion rules at generation time. C++23
/// compiles the resulting sorted typed entries into exact-sized flat tries.
class StaticScripts {
  StaticScripts(this.scripts, String constantsPath)
      : constants = (jsonDecode(File(constantsPath).readAsStringSync())
                as Map<String, dynamic>)
            .map((key, value) => MapEntry(key, (value as List).cast<String>()));

  final List<ScriptInfo> scripts;
  final Map<String, List<String>> constants;
  final List<Map<String, Object>> audit = [];
  final List<List<_Token>> _sequences = [[]]; // ID zero means no match.
  static const sequenceOffsetBits = 12;
  int _sequenceOffset = 1; // Reserve the absent-value sentinel.
  final Map<String, int> sequenceIds = {};

  static const classes = [
    'vowels',
    'vowelMarks',
    'consonants',
    'otherDiacritics',
    'accents',
    'symbols',
    'vedicSymbols',
    'exclusiveSymbols',
  ];
  static const tokenTypes = [
    'Vowel',
    'VowelMark',
    'Consonant',
    'OtherDiacritic',
    'Accent',
    'Symbol',
    'VedicSymbol',
    'ExclusiveSymbol',
  ];

  String scriptType(ScriptInfo script) => script.type == 'latin'
      ? 'Latin'
      : script.type == 'tamil'
          ? 'Tamil'
          : 'Indic';

  List<String> chars(ScriptInfo script, String type) {
    if (type == 'accents') {
      return constants[
          script.type == 'latin' ? 'LatinAccents' : 'VedicAccents']!;
    }
    if (type == 'exclusiveSymbols') {
      return script.type == 'latin' ? [] : constants['ExclusiveSymbols']!;
    }
    return ((script.info[type] as List?) ?? []).cast<String>();
  }

  void _addBase(Map<String, _ReaderToken> map, ScriptInfo script, String mode) {
    for (var type = 0; type < classes.length; type++) {
      final array = chars(script, classes[type]);
      for (var index = 0; index < array.length; index++) {
        if (script.type == 'latin' && type == 1 && index != 0) continue;
        _add(map, array[index],
            _ReaderToken(_Token(type, index, scriptType(script))), mode);
      }
    }
  }

  void _add(Map<String, _ReaderToken> map, String key, _ReaderToken value,
      String mode) {
    if (key.isEmpty) return;
    final old = map[key];
    if (old == null) {
      map[key] = value;
    } else if (old.emittedKey != value.emittedKey) {
      audit.add({
        'mode': mode,
        'spelling': key,
        'winner': old.emittedKey,
        'ignored': value.emittedKey,
      });
    }
  }

  String foldAscii(String text) => String.fromCharCodes(
      text.runes.map((ch) => ch >= 65 && ch <= 90 ? ch + 32 : ch));

  _Match _match(Map<String, _ReaderToken> map, String text, bool fold) {
    // Code-unit slices are only used for generated valid Unicode. Compare the
    // same ASCII-folded spelling as the old UTF-32 runtime lookup.
    for (var length = text.length; length > 0; length--) {
      final key = text.substring(0, length);
      final token = map[fold ? foldAscii(key) : key];
      if (token != null) return _Match(token, length);
    }
    return const _Match(null, 0);
  }

  Map<String, _ReaderToken> _reader(ScriptInfo script, {bool fold = false}) {
    final map = <String, _ReaderToken>{};
    final mode = '${script.name}${fold ? ':folded' : ''}';
    _addBase(map, script, mode);
    final equivalents = (script.info['equivalents'] as Map?) ?? {};
    final targets = equivalents.keys.cast<String>().toList()..sort(compareUtf8);
    for (final target in targets) {
      _ReaderToken? token;
      if (target.length >= 3 && target[1] == ':') {
        final type = 'vmcoasSx'.indexOf(target[0]);
        if (type < 0) continue;
        // Corresponds to the old getTokenType, including its case-sensitive S.
        final index = int.parse(target.substring(2)) & 255;
        token = _ReaderToken(_Token(type, index, scriptType(script)));
      } else {
        var rest = target;
        final lead = _match(map, rest, fold);
        if (lead.token != null) {
          token = lead.token;
          rest = rest.substring(lead.length);
          final extra = <_Token>[];
          while (rest.isNotEmpty) {
            final next = _match(map, rest, fold);
            if (next.token == null) break;
            // InputReader expands once: nested extras of subsequent matches
            // were not recursively emitted by the old engine.
            extra.add(next.token!.lead);
            rest = rest.substring(next.length);
          }
          if (extra.isNotEmpty) token = _ReaderToken(token!.lead, extra);
          if (rest.isNotEmpty) {
            audit.add(
                {'mode': mode, 'partialTarget': target, 'remainder': rest});
          }
        }
      }
      if (token == null) {
        audit.add({'mode': mode, 'unresolvedTarget': target});
        continue;
      }
      for (final alias in (equivalents[target] as List).cast<String>()) {
        _add(map, alias, token, mode);
      }
    }
    return map;
  }

  int _sequence(_ReaderToken token) {
    return sequenceIds.putIfAbsent(token.emittedKey, () {
      final tokens = [token.lead, ...token.extra];
      if (_sequenceOffset + tokens.length > (1 << sequenceOffsetBits) ||
          tokens.length >= (1 << (16 - sequenceOffsetBits))) {
        throw StateError('Token sequence exceeds the 16-bit span capacity');
      }
      final id = _sequenceOffset | (tokens.length << sequenceOffsetBits);
      _sequenceOffset += tokens.length;
      _sequences.add(tokens);
      return id;
    });
  }

  static int compareUtf8(String a, String b) =>
      compareLists(utf8.encode(a), utf8.encode(b));
  static int compareLists(List<int> a, List<int> b) {
    for (var i = 0; i < a.length && i < b.length; i++) {
      if (a[i] != b[i]) return a[i].compareTo(b[i]);
    }
    return a.length.compareTo(b.length);
  }

  static void check16(int value, String what) {
    if (value > 65535) throw StateError('$what exceeds 16-bit capacity');
  }

  // Emit byte escapes, so pooled offsets and lengths are independent of the
  // host source encoding, signed char, and Dart UTF-16 string lengths.
  static String literal(String value) =>
      '"${utf8.encode(value).map((b) => '\\x${b.toRadixString(16).padLeft(2, '0')}').join()}"';

  void writeHeader(String path) {
    if (scripts.length > 32) throw StateError('source mask capacity exceeded');
    final explicit = [for (final script in scripts) _reader(script)];
    final folded = [
      for (var i = 0; i < scripts.length; i++)
        ['iast', 'iso'].contains(scripts[i].name)
            ? _reader(scripts[i], fold: true)
            : explicit[i],
    ];
    final devanagari = scripts.indexWhere((s) => s.name == 'devanagari');
    final indic = <String, _ReaderToken>{...explicit[devanagari]};
    final ordered = scripts.toList()
      ..sort((a, b) => a.name.toLowerCase().compareTo(b.name.toLowerCase()));
    for (final script in ordered) {
      if (script.name != 'devanagari' && script.type != 'latin') {
        _addBase(indic, script, 'indic');
      }
    }

    // A spelling has one topology node. Source-specific payloads are merged
    // only when their full emitted token _sequences are identical.
    final nonRoman = <String, Map<int, int>>{};
    for (var source = 0; source < scripts.length; source++) {
      if (scripts[source].type == 'latin') continue;
      for (final entry in explicit[source].entries) {
        final id = _sequence(entry.value);
        final variants = nonRoman.putIfAbsent(entry.key, () => {});
        variants[id] = (variants[id] ?? 0) | (1 << source);
      }
    }
    for (final key in indic.keys) {
      nonRoman.putIfAbsent(key, () => {});
    }
    final keys = nonRoman.keys.toList()..sort(compareUtf8);
    final terminals = <String>[];
    final alternatives = <String>[];
    final union = <String, int>{};
    for (final key in keys) {
      final variants = nonRoman[key]!.entries.toList();
      final primary = variants.isEmpty ? const MapEntry(0, 0) : variants.first;
      final begin = alternatives.length;
      for (final entry in variants.skip(1)) {
        alternatives.add('{${entry.value}u, ${entry.key}}');
      }
      final unrestricted = indic[key] == null ? 0 : _sequence(indic[key]!);
      terminals.add(
          '{${primary.value}u, ${primary.key}, $unrestricted, {$begin, ${variants.length > 1 ? variants.length - 1 : 0}}}');
      union[key] = terminals.length;
    }
    check16(terminals.length, 'terminal IDs');
    check16(alternatives.length, 'source variants');

    final graphs = <Map<String, int>>[union];
    final readers = <String>[];
    for (var i = 0; i < scripts.length; i++) {
      if (scripts[i].type != 'latin') {
        readers.add('{0, 0, ${1 << i}u}');
      } else {
        Map<String, int> values(Map<String, _ReaderToken> map) =>
            map.map((k, v) => MapEntry(k, _sequence(v)));
        final normal = values(explicit[i]);
        final fold = values(folded[i]);
        final normalId = graphs.length;
        graphs.add(normal);
        var foldedId = normalId;
        if (normal.length != fold.length ||
            normal.entries.any((e) => fold[e.key] != e.value)) {
          foldedId = graphs.length;
          graphs.add(fold);
        }
        readers.add('{$normalId, $foldedId, 0}');
      }
    }

    final pool = StringBuffer();
    final poolRefs = <String, _Range>{};
    var poolBytes = 0;
    _Range text(String value) => poolRefs.putIfAbsent(value, () {
          final ref = _Range(poolBytes, utf8.encode(value).length);
          pool.writeln('    ${literal(value)}');
          poolBytes += ref.count;
          check16(poolBytes, 'writer string pool');
          return ref;
        });
    final charEntries = <String>[];
    final ranges = <String, _Range>{};
    final writers = <String>[];
    for (final script in scripts) {
      final scriptRanges = <String>[];
      for (final type in classes) {
        final array = chars(script, type);
        final key = jsonEncode(array);
        final range = ranges.putIfAbsent(key, () {
          final begin = charEntries.length;
          for (final value in array) {
            final ref = text(value);
            assert(ref.count <= 255, 'writer char length exceeds uint8_t');
            charEntries.add('{${ref.begin}, ${ref.count}}');
          }
          check16(charEntries.length, 'writer entries');
          return _Range(begin, array.length);
        });
        scriptRanges.add(
            'std::span{writerChars}.subspan(${range.begin}, ${range.count})');
      }
      writers.add(
          '{ScriptType::${scriptType(script)}, ${script.type == 'vedic'}, {{${scriptRanges.join(', ')}}}}');
    }

    final names = <String, int>{};
    for (var i = 0; i < scripts.length; i++) {
      names[scripts[i].name.toLowerCase()] = i;
    }
    for (var i = 0; i < scripts.length; i++) {
      for (final alias
          in ((scripts[i].info['aliases'] as List?) ?? []).cast<String>()) {
        names.putIfAbsent(alias.toLowerCase(), () => i);
      }
    }
    final sortedNames = names.keys.toList()..sort();

    final buffer = StringBuffer(
        '// GENERATED by tool/generate_headers.dart. Do not edit.\n#pragma once\n\n#include "static_script_types.h"\n\nnamespace inditrans::static_data {\n');
    void array(String type, String name, List<String> values) {
      buffer.writeln(
          'inline constexpr std::array<$type, ${values.length}> $name {{');
      for (final value in values) {
        buffer.writeln('    $value,');
      }
      buffer.writeln('}};\n');
    }

    buffer.writeln('inline constexpr char writerText[] =\n$pool;\n');
    array('WriterChar', 'writerChars', charEntries);
    array('ScriptWriterMap', 'writers', writers);
    array('ScriptName', 'names',
        [for (final name in sortedNames) '{"$name", ${names[name]}}']);
    array('ReaderInfo', 'readerInfo', readers);
    buffer.writeln('inline constexpr size_t devanagariId = $devanagari;');
    buffer.writeln(
        'inline constexpr size_t tamilId = ${scripts.indexWhere((s) => s.name == 'tamil')};\n');
    buffer.writeln(
        'inline constexpr unsigned sequenceOffsetBits = $sequenceOffsetBits;\n');
    final flatTokens = <String>['{TokenType::Ignore, 255, ScriptType::Others}'];
    for (final tokens in _sequences) {
      for (final token in tokens) {
        flatTokens.add(token.cpp);
      }
    }
    check16(flatTokens.length, 'sequence token offsets');
    array('ScriptToken', 'sequenceTokens', flatTokens);
    array('SourceTerminal', 'sourceTerminals', terminals);
    array('SourceVariant', 'sourceAlternatives', alternatives);

    var capacity = 1;
    for (final graph in graphs) {
      final bound =
          1 + graph.keys.fold<int>(0, (n, key) => n + utf8.encode(key).length);
      if (bound > capacity) capacity = bound;
    }
    buffer.writeln('using ReaderIndex = SmallestIndex<${capacity * 2}>;\n');
    for (var i = 0; i < graphs.length; i++) {
      final graph = graphs[i];
      final keys = graph.keys.toList()..sort(compareUtf8);
      array('TrieEntry<uint8_t>', 'readerEntries$i',
          [for (final key in keys) '{${literal(key)}, ${graph[key]}}']);
      buffer.writeln(
          'inline constexpr auto readerTrie$i = makeStaticTrie<readerEntries$i, ReaderIndex>();\n');
    }
    array('TrieView<uint8_t, ReaderIndex>', 'readerTries',
        [for (var i = 0; i < graphs.length; i++) 'readerTrie$i.view()']);

    // Tokenize and group prefixes exactly as the former Tamil constructor did.
    final tamil = explicit[scripts.indexWhere((s) => s.name == 'tamil')];
    final prefixKeys = <List<int>>[];
    for (final prefix in constants['TamilPrefixes']!) {
      var rest = prefix;
      final units = <List<_Token>>[];
      while (rest.isNotEmpty) {
        final found = _match(tamil, rest, false);
        if (found.token == null) {
          throw StateError('Unrecognized Tamil prefix $prefix');
        }
        final token = found.token!.lead;
        if ([0, 2, 5, 7].contains(token.type)) {
          units.add([
            token,
            const _Token(8, 255, 'Others'),
            const _Token(8, 255, 'Others'),
            const _Token(8, 255, 'Others')
          ]);
        } else {
          final slot = token.type == 1
              ? 1
              : token.type == 3
                  ? 2
                  : token.type == 4
                      ? 3
                      : -1;
          if (slot > 0) units.last[slot] = token;
        }
        rest = rest.substring(found.length);
      }
      prefixKeys.add([for (final unit in units) _unitKey(unit)]);
    }
    prefixKeys.sort(compareLists);
    for (var i = 0; i < prefixKeys.length; i++) {
      array('uint64_t', 'tamilPrefix$i',
          [for (final key in prefixKeys[i]) '${key}ull']);
    }
    array('TrieEntry<uint64_t>', 'tamilEntries',
        [for (var i = 0; i < prefixKeys.length; i++) '{tamilPrefix$i, 1}']);
    buffer.writeln(
        'inline constexpr auto tamilTrie = makeStaticTrie<tamilEntries>();\n');
    buffer.writeln('} // namespace inditrans::static_data');
    File(path).writeAsStringSync(buffer.toString());
    Directory('out').createSync(recursive: true);
    File('out/static-lookup-collisions.json').writeAsStringSync(
        '${const JsonEncoder.withIndent('  ').convert(audit)}\n');
  }

  int _unitKey(List<_Token> unit) {
    final lead = unit.first;
    var key = lead.type |
        (lead.index << 4) |
        (['Indic', 'Tamil', 'Latin', 'Others'].indexOf(lead.script) << 12);
    for (var i = 1; i < unit.length; i++) {
      key |= (unit[i].type | (unit[i].index << 4)) << (14 + (i - 1) * 12);
    }
    return key;
  }
}

class _Token {
  const _Token(this.type, this.index, this.script);
  final int type;
  final int index;
  final String script;
  String get key => '$type:$index:$script';
  String get cpp =>
      '{TokenType::${StaticScripts.tokenTypes[type]}, $index, ScriptType::$script}';
}

class _ReaderToken {
  const _ReaderToken(this.lead, [this.extra = const []]);
  final _Token lead;
  final List<_Token> extra;
  String get emittedKey => [lead, ...extra].map((t) => t.key).join('|');
}

class _Match {
  const _Match(this.token, this.length);
  final _ReaderToken? token;
  final int length;
}

class _Range {
  const _Range(this.begin, this.count);
  final int begin;
  final int count;
}
