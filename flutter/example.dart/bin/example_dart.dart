import 'package:example_dart/example.dart';
import 'package:inditrans/inditrans.dart' as i;

void printValue(String id, String value) {
  print('$id: $value');
}

Future<void> main(List<String> arguments) async {
  await init();
  final text = 'श्री॒ गु॒रु॒भ्यो नमः॒ । ह॒रिः॒ ॐ ॥';
  final res = transliterate(text, i.Script.devanagari, i.Script.tamil);

  printValue('sanskrit', text);
  printValue('tamil', res);
}
