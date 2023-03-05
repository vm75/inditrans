// ignore_for_file: constant_identifier_names, non_constant_identifier_names

/// Supported scripts
enum Script {
  assamese,
  bengali,
  devanagari,
  easyroman,
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
class Options {
  final int _value;

  const Options._(this._value);

  int get value => _value;

  static const None = Options._(0);
  static const IgnoreVedicAccents = Options._(1);
  static const IgnoreQuotedMarkers = Options._(2);
  static const TamilTraditional = Options._(4);
  static const TamilSuperscripted = Options._(8);
  static const RetainZeroWidthChars = Options._(16);
  static const ASCIINumerals = Options._(32);

  Options operator +(Options other) {
    return Options._(_value | other._value);
  }
}
