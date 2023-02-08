library inditrans;

import 'src/bindings.dart';
import 'src/ffi_proxy.dart';
import 'src/types.dart';
import 'src/utils.dart';

export 'src/types.dart' show TranslitOptions, Script;

/// A transliterate utility for some common [indic](https://en.wikipedia.org/wiki/Brahmic_scripts) scripts
///
/// The follwing scripts are currently supported:
/// * Brahmi scripts:
///   * bengali
///   * devanagari
///   * gujarati
///   * gurmukhi
///   * kannada
///   * malayalam
///   * oriya
///   * telugu
///   * tamil
/// * Roman scripts
///   * iast (https://en.wikipedia.org/wiki/International_Alphabet_of_Sanskrit_Transliteration)
///   * ipa (https://en.wikipedia.org/wiki/International_Phonetic_Alphabet)
///   * iso 15919 (https://en.wikipedia.org/wiki/ISO_15919)
///   * itrans (https://en.wikipedia.org/wiki/ITRANS)
///   * romansimple (non-standard)
class Inditrans {
  static late dynamic _platformLib;
  static late InditransBindings _bindings;
  static late Allocator _allocator;

  /// Inditrans init. Init should be completed before any use is attempted.
  ///
  /// ```
  /// void main() async {
  ///   WidgetsFlutterBinding.ensureInitialized();
  ///   await Inditrans.init();
  ///   runApp(const MyApp());
  /// }
  /// ```
  static init() async {
    await InditransDynamicLib.init();
    _platformLib = InditransDynamicLib.lib;
    _bindings = InditransBindings(_platformLib);
    _allocator = InditransDynamicLib.allocator;
  }

  /// Transliterates [text] from [from] script to [to] script.
  /// [TranslitOptions] can be used for some specific config.
  /// Should be called only after [Inditrans.init] is completed.
  ///
  /// If the transliterate fails, returns an empty string.
  static String transliterate(String text, Script from, Script to,
      [TranslitOptions options = TranslitOptions.None]) {
    final staging = StagingMemory(_allocator);

    final cText = staging.toNativeString(text);
    final cFrom = staging.toNativeString(from.toString());
    final cTo = staging.toNativeString(to.toString());

    final buffer = _bindings.transliterate(cText, cFrom, cTo, options.value);
    final result = staging.fromNativeString(buffer);

    staging.freeAll();
    return result;
  }

  static registerWith(registrar) {
    // ignore
  }
}
