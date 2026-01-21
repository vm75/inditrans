// ignore_for_file: non_constant_identifier_names

/// Transliteration options
/// Flags to control transliteration
class Option {
  /// No options (default)
  static final None = Option('None');

  /// Use traditional Tamil consonants only
  static final TamilTraditional = Option('TamilTraditional');

  /// Use superscripted Tamil consonants
  static final TamilSuperscripted = Option('TamilSuperscripted');

  /// Force ASCII numerals in transliterated text
  static final ASCIINumerals = Option('ASCIINumerals');

  /// Ignore Vedic accents in transliterated text
  static final IgnoreVedicAccents = Option('IgnoreVedicAccents');

  /// Retain special markers which are used to identify non-standard chars
  static final RetainSpecialMarkers = Option('RetainSpecialMarkers');

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
