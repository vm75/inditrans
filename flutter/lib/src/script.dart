/// Supported scripts
enum Script {
  assamese,
  bangla,
  bengali,
  burmese,
  devanagari,
  gujarati,
  gurmukhi,
  hindi,
  iast,
  indic,
  ipa,
  iso,
  itrans,
  kannada,
  khmer,
  malayalam,
  odia,
  oriya,
  punjabi,
  readableLatin,
  sanskrit,
  sinhala,
  tamil,
  tamilExtended,
  telugu,
  thai,
  tibetan,
  titus,
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
      return Script.values.firstWhere(
          (e) => e.name.toLowerCase() == name || e.toString() == this);
    } catch (e) {
      return null;
    }
  }
}
