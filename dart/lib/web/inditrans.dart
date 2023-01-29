@JS()
library inditrans;

import 'package:js/js.dart';

import '../common/const_utils.dart';

@JS()
class InditransBinding {
  external static int translitOptionsToInt(String option);
  external static String transliterate(String text, String from, String to, int option);
  external static String transliterate2(String text, String from, String to, String option);
}

class IndiTrans {
  static String transliterate(String text, String from, String to, [TranslitOptions options = TranslitOptions.None]) {
    return InditransBinding.transliterate(text, from, to, options.value);
  }
}

// class Inditrans {
//   static final js.JsObject _module = context['Module'];
//   static final _HEAP8 = _module['HEAP8'];

//   static final js.JsFunction _transliterate = _module.callMethod('cwrap', [
//     'transliterate',
//     'number',
//     ['string', 'string', 'string', 'number']
//   ]);
//   static final js.JsFunction _releaseBuffer = _module.callMethod('cwrap', [
//     'releaseBuffer',
//     'void',
//     ['number']
//   ]);

//   static final js.JsFunction _to_uint8list = js.context['to_uint8list']; // from util.js

//   typed.Uint8List toUint8List(int buffer, int length) {
//     return _to_uint8list.apply([buffer, length]);
//   }

//   static String transliterate(String text, String from, String to, [options = TranslitOptions.None]) {
//     final buffer = _transliterate.apply([text, from, to, options.value]);

//     final result = String.fromCharCodes(uint8list);

//     _releaseBuffer.apply(buffer);
//   }
// }
