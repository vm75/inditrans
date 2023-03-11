// ignore_for_file: constant_identifier_names, non_constant_identifier_names

/// Supported scripts
enum Script {
  assamese,
  bengali,
  devanagari,
  readablelatin,
  gujarati,
  gurmukhi,
  iast,
  indic,
  ipa,
  iso,
  itrans,
  kannada,
  malayalam,
  oriya,
  sinhala,
  tamil,
  telugu;

  @override
  String toString() {
    return super.toString().replaceFirst('Script.', '');
  }

  static Script? fromString(String name) {
    try {
      return Script.values.firstWhere((entry) => entry.toString() == name);
    } catch (_) {
      return null;
    }
  }
}

extension InditransScriptExtension on String {
  Script? toScript() => Script.fromString(this);
}

/// Transliteration options
class Option {
  final int _value;

  const Option._(this._value);

  int get value => _value;

  static const None = Option._(0);
  static const IgnoreVedicAccents = Option._(1);
  static const IgnoreQuotedMarkers = Option._(2);
  static const TamilTraditional = Option._(4);
  static const TamilSuperscripted = Option._(8);
  static const RetainZeroWidthChars = Option._(16);
  static const ASCIINumerals = Option._(32);

  Option operator +(Option other) {
    return Option._(_value | other._value);
  }

  static const Map<String, Option> values = {
    'None': None,
    'IgnoreVedicAccents': IgnoreVedicAccents,
    'IgnoreQuotedMarkers': IgnoreQuotedMarkers,
    'TamilTraditional': TamilTraditional,
    'TamilSuperscripted': TamilSuperscripted,
    'RetainZeroWidthChars': RetainZeroWidthChars,
    'ASCIINumerals': ASCIINumerals,
  };
  static Option fromString(String name) {
    return values[name] ?? None;
  }
}
