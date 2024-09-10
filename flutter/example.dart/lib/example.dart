import 'package:inditrans/inditrans.dart' as inditrans;

const bool kIsWeb = bool.fromEnvironment('dart.library.js_interop');

Future<void> init() async {
  await inditrans.init(kIsWeb ? 'assets/inditrans.wasm' : 'inditrans');
}

String transliterate(String text, inditrans.Script from, inditrans.Script to) {
  return inditrans.transliterate(text, from, to);
}
