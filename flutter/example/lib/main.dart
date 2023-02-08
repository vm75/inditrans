import 'package:flutter/material.dart';

import 'package:inditrans/inditrans.dart' as inditrans;
import 'package:google_fonts/google_fonts.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  final stopwatch = Stopwatch()..start();
  await inditrans.init();

  runApp(MyApp(timeToInit: stopwatch.elapsed));
}

class MyApp extends StatefulWidget {
  final Duration timeToInit;
  const MyApp({super.key, required this.timeToInit});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  @override
  Widget build(BuildContext context) {
    const spacerSmall = SizedBox(height: 10);

    const from = inditrans.Script.devanagari;
    const to = inditrans.Script.tamil;
    const original = "श्री॒ गु॒रु॒भ्यो नमः॒ । ह॒रिः॒ ओ३म् ॥";
    final transliterated = inditrans.transliterate(
        original,
        from,
        to,
        inditrans.TranslitOptions.IgnoreVedicAccents |
            inditrans.TranslitOptions.TamilSuperscripted);

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
              children: [
                Text(
                  'inditrans test (init completed in ${widget.timeToInit.inMilliseconds} ms)',
                  style: const TextStyle(
                      fontSize: 40, fontWeight: FontWeight.bold),
                  textAlign: TextAlign.center,
                ),
                spacerSmall,
                Text(
                  'Original text in $from: $original',
                  style: GoogleFonts.notoSansDevanagari(),
                  textAlign: TextAlign.left,
                ),
                spacerSmall,
                Text(
                  'Transliterated text in $to: $transliterated',
                  style: GoogleFonts.notoSansTamil(),
                  textAlign: TextAlign.left,
                ),
              ],
            )),
          ),
        ),
      ),
    );
  }
}
