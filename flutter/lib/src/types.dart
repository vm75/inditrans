// ignore_for_file: constant_identifier_names, non_constant_identifier_names

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
class TranslitOptions {
  final int _value;

  const TranslitOptions._(this._value);

  int get value => _value;

  static const None = TranslitOptions._(0);
  static const IgnoreVedicAccents = TranslitOptions._(1);
  static const IgnoreQuotedMarkers = TranslitOptions._(2);
  static const TamilTraditional = TranslitOptions._(4);
  static const TamilSuperscripted = TranslitOptions._(8);
  static const InferAnuswara = TranslitOptions._(16);
  static const RetainZeroWidthChars = TranslitOptions._(32);

  TranslitOptions operator |(TranslitOptions other) {
    return TranslitOptions._(_value | other._value);
  }

  TranslitOptions operator &(TranslitOptions other) {
    return TranslitOptions._(_value & other._value);
  }
}
