/// Supported scripts
enum Script {
  assamese,
  bangla,
  bengali,
  burmese,
  devanagari,
  grantha,
  gujarati,
  gurmukhi,
  hindi,
  hk,
  iast,
  indic,
  indlish,
  ipa,
  iso,
  itrans,
  kannada,
  khmer,
  malayalam,
  nastaliq,
  odia,
  oriya,
  punjabi,
  sanskrit,
  sinhala,
  slp1,
  tamil,
  telugu,
  thai,
  tibetan,
  titus,
  urdu,
  velthuis,
  wx,
}

/// Gets the name of the script
extension ScriptNameExtension on Script {
  String get name => toString().split('.').last;
}

/// Gets the script from a string
extension ToScriptExtension on String {
  Script? toScript() {
    try {
      final name = toLowerCase().split('.').last;
      return Script.values.firstWhere((e) => e.name.toLowerCase() == name || e.toString() == this);
    } catch (e) {
      return null;
    }
  }
}
