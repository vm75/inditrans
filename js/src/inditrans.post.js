Module['onRuntimeInitialized'] = function () {
    Module['inditrans'] = {
        'Options': {
            'None': 0,
            'TamilTraditional': 1,
            'TamilSuperscripted': 2,
            'ASCIINumerals': 4,
            'IgnoreVedicAccents': 8,
            'RetainSpecialMarkers': 16,
            'NoXMLTagHandling': 32,
        },
        'Scripts': [
            'assamese',
            'bangla',
            'bengali',
            'burmese',
            'devanagari',
            'gujarati',
            'gurmukhi',
            'hindi',
            'hk',
            'iast',
            'indic',
            'ipa',
            'iso',
            'itrans',
            'kannada',
            'khmer',
            'malayalam',
            'odia',
            'oriya',
            'punjabi',
            'readableLatin',
            'sanskrit',
            'sinhala',
            'slp1',
            'tamil',
            'tamilExtended',
            'telugu',
            'thai',
            'tibetan',
            'titus',
            'velthuis',
            'wx',
        ],
        'optionsToInt': (optionsString) => {
            const options = optionsString.split(' ');
            let result = 0;
            for (const option of options) {
                result |= Module['inditrans']['Options'][option];
            }
            return result;
        },
        'transliterate': (text, from, to, options, skipStart = "##", skipEnd = "##") => {
            const transliterate = cwrap('transliterate', 'number', ['string', 'string', 'string', 'number', 'string', 'string']);
            const releaseBuffer = cwrap('releaseBuffer', 'void', ['number']);
            const stringPtr = transliterate(text, from, to, options, skipStart, skipEnd);
            if (stringPtr == 0) {
                return '';
            }
            const result = UTF8ToString(stringPtr);
            releaseBuffer(stringPtr);
            return result;
        },
        'isScriptSupported': (script) => {
            const isScriptSupported = cwrap('isScriptSupported', 'number', ['string']);
            return isScriptSupported(script) == 1;
        },
        'supportedScripts': () => {
            return Module['inditrans']['Scripts'];
        }
    }
}