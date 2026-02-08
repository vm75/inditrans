// ignore_for_file: non_constant_identifier_names

/// Transliteration options
/// Flags to control transliteration
class Option {
  /// No options (default)
  static final None = Option._strVal('None', 0);

  /// Use traditional Tamil consonants only
  static final TamilTraditional = Option._strVal('TamilTraditional', 1);

  /// Use superscripted Tamil consonants
  static final TamilSuperscripted = Option._strVal('TamilSuperscripted', 2);

  /// Force ASCII numerals in transliterated text
  static final ASCIINumerals = Option._strVal('ASCIINumerals', 4);

  /// Ignore Vedic accents in transliterated text
  static final IgnoreVedicAccents = Option._strVal('IgnoreVedicAccents', 8);

  /// Retain special markers which are used to identify non-standard chars
  static final RetainSpecialMarkers = Option._strVal('RetainSpecialMarkers', 16);

  /// Do not check for xml/html tags, and treat them as normal text
  static final NoXMLTagHandling = Option._strVal('NoXMLTagHandling', 32);

  /// Returns the int value of the option flag
  int get value => _value;

  /// Joins two options
  Option operator +(Option other) {
    return Option._val(_value | other._value);
  }

  /// Returns the int value of the option flag
  final int _value;

  const Option._val(this._value);

  static final _optionSplitterRe = RegExp(r'[\s,]+');
  static final _valueMap = <String, int>{};

  Option._strVal(String optionsString, this._value) {
    _valueMap[optionsString.toLowerCase()] = _value;
  }

  /// Gets the transliteration option from a string
  factory Option(String optionsString) {
    final options = optionsString.toLowerCase().split(_optionSplitterRe);
    int value = None.value;

    for (final option in options) {
      if (_valueMap.containsKey(option)) {
        value |= _valueMap[option]!;
      }
    }

    return Option._val(value);
  }

  @override
  String toString() {
    final options = <String>[];
    for (final entry in _valueMap.entries) {
      if (value & entry.value != 0) {
        options.add(entry.key);
      }
    }
    return options.join(', ');
  }
}
