// ignore_for_file: non_constant_identifier_names

/// Enum with value
enum _Options {
  none._(0),
  tamiltraditional._(1),
  tamilsuperscripted._(2),
  asciinumerals._(4),
  ignorevedicaccents._(8),
  retainspecialmarkers._(16);

  final int value;
  const _Options._(this.value);
}

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

  /// Gets the transliteration option from a string
  factory Option(String optionsString) {
    final options = optionsString.toLowerCase().split(' ').map((e) => e.trim());
    int value = _Options.none.value;

    for (final option in options) {
      try {
        value |=
            _Options.values.firstWhere((entry) => entry.name == option).value;
      } catch (_) {}
    }

    return Option._(value);
  }
}
