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
            'DisableInference': 64,
            'DevanagariTraditional': 128,
        },
        'Scripts': [
            'assamese',
            'bangla',
            'bengali',
            'burmese',
            'devanagari',
            'grantha',
            'gujarati',
            'gurmukhi',
            'hindi',
            'hk',
            'iast',
            'indic',
            'indlish',
            'ipa',
            'iso',
            'itrans',
            'kannada',
            'khmer',
            'malayalam',
            'nastaliq',
            'odia',
            'oriya',
            'punjabi',
            'sanskrit',
            'sinhala',
            'slp1',
            'tamil',
            'telugu',
            'thai',
            'tibetan',
            'titus',
            'urdu',
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
            const cwrap = Module['cwrap'] || (typeof cwrap !== 'undefined' ? cwrap : null);
            const utf8ToString = Module['UTF8ToString'] || (typeof UTF8ToString !== 'undefined' ? UTF8ToString : null);
            const transliterate = cwrap('transliterate', 'number', ['string', 'string', 'string', 'number', 'string', 'string']);
            const releaseBuffer = cwrap('releaseBuffer', 'void', ['number']);
            const stringPtr = transliterate(text, from, to, options, skipStart, skipEnd);
            if (stringPtr == 0) {
                return '';
            }
            const result = utf8ToString(stringPtr);
            releaseBuffer(stringPtr);
            return result;
        },
        'transliterateUrduWithLexicon': (text, to, options, data, skipStart = "##", skipEnd = "##") => {
            const cwrap = Module['cwrap'] || (typeof cwrap !== 'undefined' ? cwrap : null);
            const utf8ToString = Module['UTF8ToString'] || (typeof UTF8ToString !== 'undefined' ? UTF8ToString : null);
            const convert = cwrap('transliterateUrduWithLexiconData', 'number',
                ['string', 'string', 'number', 'number', 'number', 'string', 'string']);
            const releaseBuffer = cwrap('releaseBuffer', 'void', ['number']);
            const bytes = data || new Uint8Array();
            const ptr = bytes.length ? Module['_malloc'](bytes.length) : 0;
            if (bytes.length && !ptr) return '';
            try {
                if (ptr) HEAPU8.set(bytes, ptr);
                const resultPtr = convert(text, to, options, ptr, bytes.length, skipStart, skipEnd);
                if (!resultPtr) return '';
                try { return utf8ToString(resultPtr); } finally { releaseBuffer(resultPtr); }
            } finally {
                if (ptr) Module['_free'](ptr);
            }
        },

        'loadUrduLexicon': (data) => {
            const bytes = data || new Uint8Array();
            if (!bytes.length) return 0;
            const create = Module['cwrap']('createUrduLexiconHandle', 'number', ['number', 'number']);
            const ptr = Module['_malloc'](bytes.length);
            if (!ptr) return 0;
            try {
                HEAPU8.set(bytes, ptr);
                return create(ptr, bytes.length);
            } finally {
                Module['_free'](ptr);
            }
        },
        'transliterateUrduWithLoadedLexicon': (text, to, options, handle, skipStart = "##", skipEnd = "##") => {
            const convert = Module['cwrap']('transliterateUrduWithLexiconHandle', 'number',
                ['string', 'string', 'number', 'number', 'string', 'string']);
            const release = Module['cwrap']('releaseBuffer', 'void', ['number']);
            const resultPtr = convert(text, to, options, handle, skipStart, skipEnd);
            if (!resultPtr) return '';
            try {
                return Module['UTF8ToString'](resultPtr);
            } finally {
                release(resultPtr);
            }
        },
        'releaseUrduLexicon': (handle) => {
            if (handle) Module['cwrap']('releaseUrduLexiconHandle', 'void', ['number'])(handle);
        },
        'isScriptSupported': (script) => {
            const cwrap = Module['cwrap'] || (typeof cwrap !== 'undefined' ? cwrap : null);
            const isScriptSupported = cwrap('isScriptSupported', 'number', ['string']);
            return isScriptSupported(script) == 1;
        },
        'supportedScripts': () => {
            return Module['inditrans']['Scripts'];
        }
    }
}
