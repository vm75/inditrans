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
class Options {
  final int _value;

  const Options._(this._value);

  int get value => _value;

  static const None = Options._(0);
  static const IgnoreVedicAccents = Options._(1);
  static const IgnoreQuotedMarkers = Options._(2);
  static const TamilTraditional = Options._(4);
  static const TamilSuperscripted = Options._(8);
  static const InferAnuswara = Options._(16);
  static const RetainZeroWidthChars = Options._(32);
  static const ASCIINumerals = Options._(64);

  Options operator +(Options other) {
    return Options._(_value | other._value);
  }
}
