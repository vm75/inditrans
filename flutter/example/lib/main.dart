import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import 'package:inditrans/inditrans.dart' as inditrans;
import 'package:google_fonts/google_fonts.dart';

class TestSpec {
  final String description;
  final String text;
  final inditrans.Script fromScript;
  final inditrans.Script toScript;
  final inditrans.Option options;
  final String expected;

  TestSpec(this.description, this.text, this.fromScript, this.toScript,
      this.options, this.expected);
}

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  final stopwatch = Stopwatch()..start();
  await inditrans.init();

  // load json asset
  final jsonString = await rootBundle.loadString('assets/test-cases.json');

  final List<TestSpec> testCases = [];
  for (final input in jsonDecode(jsonString)) {
    final description = input['description'];
    final inputText = input['text'];
    final inputScript =
        (input['script'] as String?)?.toScript() ?? inditrans.Script.devanagari;
    for (final target in input['targets']) {
      final targetScript = (target['script'] as String?)?.toScript() ??
          inditrans.Script.devanagari;
      final expected = target['text'];
      final options = inditrans.Option(target['options'] ?? '');
      testCases.add(TestSpec(description, inputText, inputScript, targetScript,
          options, expected));
    }
  }

  runApp(MyApp(
    timeToInit: stopwatch.elapsed,
    testCases: testCases,
  ));
}

class MyApp extends StatefulWidget {
  final Duration timeToInit;
  final List<TestSpec> testCases;
  const MyApp({super.key, required this.timeToInit, required this.testCases});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  List<Widget> _runTests() {
    final List<Widget> widgets = [];
    try {
      inditrans.transliterate(
          "Dummy", inditrans.Script.devanagari, inditrans.Script.indic);

      widgets.add(const Text(
        'to-indic PASSED',
        style: TextStyle(
            fontSize: 20, fontWeight: FontWeight.bold, color: Colors.green),
        textAlign: TextAlign.left,
      ));
    } catch (e) {
      widgets.add(const Text(
        'to-indic FAILED',
        style: TextStyle(
            fontSize: 20, fontWeight: FontWeight.bold, color: Colors.red),
        textAlign: TextAlign.left,
      ));
      // ignore
    }
    for (int testId = 0; testId < widget.testCases.length; testId++) {
      final testCase = widget.testCases[testId];
      final actual = inditrans.transliterate(testCase.text, testCase.fromScript,
          testCase.toScript, testCase.options);
      final name =
          'Test #${testId + 1}: ${testCase.description} (${testCase.fromScript} -> ${testCase.toScript})';
      if (actual == testCase.expected) {
        widgets.add(Text(
          '$name PASSED',
          style: const TextStyle(
              fontSize: 20, fontWeight: FontWeight.bold, color: Colors.green),
          textAlign: TextAlign.left,
        ));
      } else {
        widgets.add(Text(
          '$name FAILED',
          style: const TextStyle(
              fontSize: 20, fontWeight: FontWeight.bold, color: Colors.red),
          textAlign: TextAlign.left,
        ));
        widgets.add(Text(
          'Expected: ${testCase.expected}',
          style: GoogleFonts.notoSansTamil(),
          textAlign: TextAlign.left,
        ));
        widgets.add(Text(
          'Actual: $actual',
          style: GoogleFonts.notoSansTamil(),
          textAlign: TextAlign.left,
        ));
      }
      widgets.add(const SizedBox(height: 10));
    }
    return widgets;
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('Native Packages'),
        ),
        body: SingleChildScrollView(
          child: Container(
            padding: const EdgeInsets.all(10),
            child: Center(
                child: Column(
              children: _runTests(),
            )),
          ),
        ),
      ),
    );
  }
}
