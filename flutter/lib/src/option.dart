// ignore_for_file: non_constant_identifier_names

/// Transliteration options
/// Flags to control transliteration
class Option {
  static const _strIntMap = {
    'none': 0,
    'tamiltraditional': 1,
    'tamilsuperscripted': 2,
    'asciinumerals': 4,
    'ignorevedicaccents': 8,
    'retainspecialmarkers': 16,
  };

  /// No options (default)
  static final None = Option._(0);

  /// Use traditional Tamil consonants only
  static final TamilTraditional = Option._(1);

  /// Use superscripted Tamil consonants
  static final TamilSuperscripted = Option._(2);

  /// Force ASCII numerals in transliterated text
  static final ASCIINumerals = Option._(4);

  /// Ignore Vedic accents in transliterated text
  static final IgnoreVedicAccents = Option._(8);

  /// Retain special markers which are used to identify non-standard chars
  static final RetainSpecialMarkers = Option._(16);

  /// Returns the int value of the option flag
  int get value => _value;

  /// Joins two options
  Option operator +(Option other) {
    return Option._(_value | other._value);
  }

  /// Returns the int value of the option flag
  final int _value;

  const Option._(this._value);

  static final _optionSplitterRe = RegExp(r'[\s,]+');

  /// Gets the transliteration option from a string
  factory Option(String optionsString) {
    final options = optionsString.toLowerCase().split(_optionSplitterRe);
    int value = None.value;

    for (final option in options) {
      if (_strIntMap.containsKey(option)) {
        value |= _strIntMap[option]!;
      }
    }

    return Option._(value);
  }

  @override
  String toString() {
    final options = <String>[];
    for (final entry in _strIntMap.entries) {
      if (value & entry.value != 0) {
        options.add(entry.key);
      }
    }
    return options.join(', ');
  }
}
