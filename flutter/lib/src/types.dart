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

  /// Converts a [String] to [Script] if valid
  static Script? fromString(String name) {
    try {
      return Script.values.firstWhere((entry) => entry.toString() == name);
    } catch (_) {
      return null;
    }
  }
}

/// Extension methods for [Script]
/// Converts a [String] to [Script] if valid
extension InditransScriptExtension on String {
  Script? toScript() => Script.fromString(this);
}

/// Transliteration options
/// Flags to control transliteration
class Option {
  final int _value;

  const Option._(this._value);

  /// Returns the int value of the option flag
  int get value => _value;

  /// No options (default)
  static const None = Option._(0);

  /// Ignore Vedic accents in transliterated text
  static const IgnoreVedicAccents = Option._(1);

  /// Ignore quoted markers in transliterated text
  static const IgnoreQuotedMarkers = Option._(2);

  /// Use traditional Tamil consonants
  static const TamilTraditional = Option._(4);

  /// Use superscripted Tamil consonants
  static const TamilSuperscripted = Option._(8);

  /// Retain zero-width characters in transliterated text
  static const RetainZeroWidthChars = Option._(16);

  /// Force ASCII numerals in transliterated text
  static const ASCIINumerals = Option._(32);

  /// Joins two options
  Option operator +(Option other) {
    return Option._(_value | other._value);
  }

  static const Map<String, Option> _values = {
    'None': None,
    'IgnoreVedicAccents': IgnoreVedicAccents,
    'IgnoreQuotedMarkers': IgnoreQuotedMarkers,
    'TamilTraditional': TamilTraditional,
    'TamilSuperscripted': TamilSuperscripted,
    'RetainZeroWidthChars': RetainZeroWidthChars,
    'ASCIINumerals': ASCIINumerals,
  };
  static Option fromString(String name) {
    return _values[name] ?? None;
  }
}
