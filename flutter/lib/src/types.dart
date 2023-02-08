// ignore_for_file: constant_identifier_names

import 'package:inditrans/src/utils.dart';

/// Supported scripts
enum Script {
  bengali,
  devanagari,
  gujarati,
  gurmukhi,
  kannada,
  malayalam,
  oriya,
  telugu,
  tamil,
  iast,
  ipa,
  iso,
  itrans,
  romansimple;

  @override
  String toString() {
    return super.toString().replaceFirst('Script.', '');
  }
}

/// Transliteration options
class TranslitOptions extends AbstractFlag<TranslitOptions> {
  const TranslitOptions(int value) : super(value);
  static const None = TranslitOptions(0);
  static const IgnoreVedicAccents = TranslitOptions(1);
  static const IgnoreQuotedMarkers = TranslitOptions(2);
  static const TamilTraditional = TranslitOptions(4);
  static const TamilSuperscripted = TranslitOptions(8);
  static const InferAnuswara = TranslitOptions(16);
  static const RetainZeroWidthChars = TranslitOptions(32);

  @override
  TranslitOptions create(int value) {
    return TranslitOptions(value);
  }
}
