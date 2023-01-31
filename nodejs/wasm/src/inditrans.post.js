/// <reference types="emscripten" />
/** Above will import declarations from @types/emscripten, including Module etc. */
/** It is not .ts file but declaring reference will pass TypeScript Check. */

Module['onRuntimeInitialized'] = function () {
  Module['translitOptionsToInt'] = (optstr) => {
    const translitOptionsToInt = cwrap('translitOptionsToInt', 'number', ['string']);
    return translitOptionsToInt(optstr);
  }
  Module['transliterate'] = (text, from, to, options) => {
    const transliterate = cwrap('transliterate', 'number', ['string', 'string', 'string', 'number']);
    const releaseBuffer = cwrap('releaseBuffer', 'void', ['number']);
    const stringPtr = transliterate(text, from, to, options);
    if (stringPtr == 0) {
      return '';
    }
    const result = UTF8ToString(stringPtr);
    releaseBuffer(stringPtr);
    return result;
  }
  Module['transliterate2'] = (text, from, to, optstr) => {
    const transliterate = cwrap('transliterate2', 'number', ['string', 'string', 'string', 'string']);
    const releaseBuffer = cwrap('releaseBuffer', 'void', ['number']);
    const stringPtr = transliterate(text, from, to, optstr);
    if (stringPtr == 0) {
      return '';
    }
    const result = UTF8ToString(stringPtr);
    releaseBuffer(stringPtr);
    return result;
  }
}