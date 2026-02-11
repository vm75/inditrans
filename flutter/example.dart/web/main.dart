import 'package:example_dart/example.dart';
import 'package:inditrans/inditrans.dart' as i;
import 'package:web/web.dart';

void setValue(String id, String value) {
  (document.querySelector('#$id') as HTMLElement).textContent = value;
}

void main() async {
  await i.init('assets/inditrans.wasm');
  final text = 'श्री॒ गु॒रु॒भ्यो नमः॒ । ह॒रिः॒ ॐ ॥';
  final res = transliterate(text, i.Script.devanagari, i.Script.tamil);

  setValue('sanskrit', text);
  setValue('tamil', res);
}
