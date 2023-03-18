Module['onRuntimeInitialized'] = function () {
    Module['inditrans'] = {
        'Options': {
            'None': 0,
            'IgnoreVedicAccents': 1,
            'IgnoreQuotedMarkers': 2,
            'TamilTraditional': 4,
            'TamilSuperscripted': 8,
            'RetainZeroWidthChars': 16,
            'ASCIINumerals': 32,
        },
        'optionsToInt': (optionsString) => {
            const options = optionsString.split(' ');
            let result = 0;
            for (const option of options) {
                result |= Module['inditrans']['Options'][option];
            }
            return result;
        },
        'transliterate': (text, from, to, options) => {
            const transliterate = cwrap('transliterate', 'number', ['string', 'string', 'string', 'number']);
            const releaseBuffer = cwrap('releaseBuffer', 'void', ['number']);
            const stringPtr = transliterate(text, from, to, options);
            if (stringPtr == 0) {
                return '';
            }
            const result = UTF8ToString(stringPtr);
            releaseBuffer(stringPtr);
            return result;
        },
        'isScriptSupported' : (script) => {
            const isScriptSupported = cwrap('isScriptSupported', 'number', ['string']);
            return isScriptSupported(script) == 1;
        }
    }
}