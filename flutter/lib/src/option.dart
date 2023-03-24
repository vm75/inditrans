// ignore_for_file: non_constant_identifier_names

/// Transliteration options
/// Flags to control transliteration
class Option {
  /// No options (default)
  static final None = Option._(0, 'None');

  /// Use traditional Tamil consonants only
  static final TamilTraditional = Option._(1, 'TamilTraditional');

  /// Use superscripted Tamil consonants
  static final TamilSuperscripted = Option._(2, 'TamilSuperscripted');

  /// Force ASCII numerals in transliterated text
  static final ASCIINumerals = Option._(4, 'ASCIINumerals');

  /// Ignore Vedic accents in transliterated text
  static final IgnoreVedicAccents = Option._(8, 'IgnoreVedicAccents');

  /// Retain special markers which are used to identify non-standard chars
  static final RetainSpecialMarkers = Option._(16, 'RetainSpecialMarkers');

  /// Returns the int value of the option flag
  int get value => _value;

  /// Joins two options
  Option operator +(Option other) {
    return Option._(_value | other._value, '$_name ${other._name}');
  }

  /// Gets the transliteration option from a string
  static Option fromString(String optionsString) {
    Option options = Option.None;
    final optionString = optionsString.split(RegExp(r'[^a-zA-Z]+'));
    for (final entry in optionString) {
      final name = entry.toLowerCase();
      if (_values[name] == null) {
        continue;
      }
      final other = _values[name]!;
      if (options._value & other._value == 0) {
        options += other;
      }
    }
    return options;
  }

  static final Map<String, Option> _values = {};

  final int _value;
  final String _name;

  Option._(this._value, this._name) {
    _values[_name.toLowerCase()] = this;
  }
}
