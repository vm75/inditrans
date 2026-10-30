/**
 * Inditrans Standalone Wrapper
 *
 * A minimal wrapper for using inditrans.js without adding it as a dependency.
 *
 * Usage:
 * 1. Copy this file and inditrans.js to your project
 * 2. Load inditrans.js first, then this wrapper:
 *    <script src="inditrans.js"></script>
 *    <script src="inditrans-standalone.js"></script>
 *
 * 3. Use the API:
 *    Inditrans.ready().then(() => {
 *      const result = Inditrans.transliterate(
 *        'नमस्ते',
 *        Inditrans.Script.devanagari,
 *        Inditrans.Script.iast,
 *        Inditrans.Option.None
 *      );
 *      console.log(result); // 'namaste'
 *    });
 */

(function (global) {
  'use strict';

  // Supported scripts
  const Script = {
    assamese: 'assamese',
    bangla: 'bangla',
    bengali: 'bengali',
    burmese: 'burmese',
    devanagari: 'devanagari',
    grantha: 'grantha',
    gujarati: 'gujarati',
    gurmukhi: 'gurmukhi',
    hindi: 'hindi',
    hk: 'hk',
    iast: 'iast',
    indic: 'indic',
    indlish: 'indlish',
    ipa: 'ipa',
    iso: 'iso',
    itrans: 'itrans',
    kannada: 'kannada',
    khmer: 'khmer',
    malayalam: 'malayalam',
    odia: 'odia',
    oriya: 'oriya',
    punjabi: 'punjabi',
    sanskrit: 'sanskrit',
    sinhala: 'sinhala',
    slp1: 'slp1',
    tamil: 'tamil',
    telugu: 'telugu',
    thai: 'thai',
    tibetan: 'tibetan',
    titus: 'titus',
    velthuis: 'velthuis',
    wx: 'wx',
    urdu: 'urdu',
    nastaliq: 'nastaliq'
  };

  // Transliteration options (flags that can be combined)
  const Option = {
    None: 0,
    TamilTraditional: 1,
    TamilSuperscripted: 2,
    ASCIINumerals: 4,
    IgnoreVedicAccents: 8,
    RetainSpecialMarkers: 16,
    NoXMLTagHandling: 32,
    DisableInference: 64,
    DevanagariTraditional: 128
  };

  let moduleReady = false;
  let readyPromise = null;

  /**
   * Wait for the inditrans WASM module to be ready
   * @returns {Promise<void>}
   */
  function ready() {
    if (readyPromise) {
      return readyPromise;
    }

    readyPromise = new Promise((resolve, reject) => {
      if (typeof Module === 'undefined') {
        reject(new Error('inditrans.js not loaded. Make sure to load inditrans.js before this wrapper.'));
        return;
      }

      if (Module.inditrans) {
        moduleReady = true;
        resolve();
        return;
      }

      // Wait for the module to be ready
      if (Module.onRuntimeInitialized) {
        const originalCallback = Module.onRuntimeInitialized;
        Module.onRuntimeInitialized = function () {
          if (originalCallback) originalCallback();
          moduleReady = true;
          resolve();
        };
      } else {
        // Set up callback if it doesn't exist yet
        Module.onRuntimeInitialized = function () {
          moduleReady = true;
          resolve();
        };
      }

      // Timeout after 10 seconds
      setTimeout(() => {
        if (!moduleReady) {
          reject(new Error('inditrans module initialization timeout'));
        }
      }, 10000);
    });

    return readyPromise;
  }

  /**
   * Transliterate text from one script to another
   * @param {string} text - The text to transliterate
   * @param {string} fromScript - Source script (use Script enum)
   * @param {string} toScript - Target script (use Script enum)
   * @param {number} options - Transliteration options (use Option enum, can be combined with |)
   * @param {string} skipStart - Text to skip at the start of the input
   * @param {string} skipEnd - Text to skip at the end of the input
   * @returns {string} The transliterated text
   */
  function transliterate(text, fromScript, toScript, options = Option.None, skipStart = '##', skipEnd = '##') {
    if (!moduleReady || !Module.inditrans) {
      throw new Error('Inditrans not initialized. Call ready() and wait for it to resolve first.');
    }
    return Module.inditrans.transliterate(text, fromScript, toScript, options, skipStart, skipEnd);
  }

  /**
   * Load a caller-provided IUL1 Urdu pronunciation pack once. Returns an opaque
   * pointer (0 for invalid packs), and does not bundle a production dictionary.
   */
  function loadUrduLexicon(bytes) {
    if (!moduleReady || !Module.inditrans) throw new Error('Inditrans not initialized.');
    return Module.inditrans.loadUrduLexicon(bytes);
  }

  /** Convert Urdu text using a reusable lexicon handle. */
  function transliterateUrduWithLoadedLexicon(text, to, options, handle, skipStart = '##', skipEnd = '##') {
    if (!moduleReady || !Module.inditrans) throw new Error('Inditrans not initialized.');
    return Module.inditrans.transliterateUrduWithLoadedLexicon(text, to, options, handle, skipStart, skipEnd);
  }

  /** Release a caller-owned Urdu lexicon handle. */
  function releaseUrduLexicon(handle) {
    if (!moduleReady || !Module.inditrans) throw new Error('Inditrans not initialized.');
    Module.inditrans.releaseUrduLexicon(handle);
  }

  /**
   * Check if a script is supported
   * @param {string} script - The script name to check
   * @returns {boolean} True if the script is supported
   */
  function isScriptSupported(script) {
    if (!moduleReady || !Module.inditrans) {
      throw new Error('Inditrans not initialized. Call ready() and wait for it to resolve first.');
    }
    return Module.inditrans.Scripts && Module.inditrans.Scripts.hasOwnProperty(script);
  }

  /**
   * Get all supported scripts
   * @returns {string[]} Array of supported script names
   */
  function getSupportedScripts() {
    if (!moduleReady || !Module.inditrans) {
      throw new Error('Inditrans not initialized. Call ready() and wait for it to resolve first.');
    }
    return Module.inditrans.Scripts ? Object.values(Module.inditrans.Scripts) : [];
  }

  // Export the API
  const Inditrans = {
    Script,
    Option,
    ready,
    transliterate,
    loadUrduLexicon,
    transliterateUrduWithLoadedLexicon,
    releaseUrduLexicon,
    isScriptSupported,
    getSupportedScripts
  };

  // Export for different module systems
  if (typeof module !== 'undefined' && module.exports) {
    // CommonJS
    module.exports = Inditrans;
  } else if (typeof define === 'function' && define.amd) {
    // AMD
    define([], function () {
      return Inditrans;
    });
  } else {
    // Browser global
    global.Inditrans = Inditrans;
  }

})(typeof window !== 'undefined' ? window : this);
