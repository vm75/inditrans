Module['onRuntimeInitialized'] = function () {
    Module['inditrans'] = {
        'Options': {
            'None': 0,
            'TamilTraditional': 1,
            'TamilSuperscripted': 2,
            'ASCIINumerals': 4,
            'IgnoreVedicAccents': 8,
            'ShowQuotedMarkers': 16,
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