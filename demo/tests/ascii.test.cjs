// Run with: node --test demo/tests/ascii.test.cjs
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const { runInNewContext } = require('node:vm');
const { test } = require('node:test');
const assert = require('node:assert/strict');

// Exercise the buildless demo's actual tables and composer without a browser dependency.
const html = readFileSync(join(__dirname, '../index.html'), 'utf8');
const data = readFileSync(join(__dirname, '../keyboard-data.js'), 'utf8');
const catalog = html.slice(html.indexOf('function keyboardAccents'), html.indexOf('// Font dev tools:'));
const composer = html.slice(html.indexOf('function createAsciiComposer'), html.indexOf('function createKeyboard'));
const { KEYBOARD_MAPS, ASCII_KEYBOARD_TOKENS, createAsciiComposer, keyboardAccents, SCRIPT_CATALOG,
  NATIVE_KEYBOARD_LAYOUTS, nativeKeyboardValues, nativeKeyboardExtras } = runInNewContext(
  data + '\n' + catalog + '\n' + composer + ';({ KEYBOARD_MAPS, ASCII_KEYBOARD_TOKENS, createAsciiComposer, keyboardAccents, SCRIPT_CATALOG, NATIVE_KEYBOARD_LAYOUTS, nativeKeyboardValues, nativeKeyboardExtras })'
);
function layoutFor(script) {
  const layout = KEYBOARD_MAPS[script];
  return {
    ...layout,
    inherentVowel: SCRIPT_CATALOG[script].kind === 'roman' ? layout.vowels[0] : layout.inherentVowel,
    symbols: [...layout.symbols, ...layout.vedicSymbols], accents: keyboardAccents(script)
  };
}
const compose = createAsciiComposer(layoutFor('devanagari'));

test('native layouts retain original positions and hide foreign equivalents and duplicates', () => {
  for (const [script, entry] of Object.entries(SCRIPT_CATALOG)) {
    if (entry.kind !== 'indic' && entry.kind !== 'persoArabic') continue;
    assert.ok(NATIVE_KEYBOARD_LAYOUTS[script], script);
    const layout = KEYBOARD_MAPS[script];
    for (const group of ['vowels', 'vowelMarks', 'consonants', 'otherDiacritics', 'symbols']) {
      const values = group === 'symbols' ? [...layout.symbols, ...layout.vedicSymbols] : layout[group];
      const visible = nativeKeyboardValues(script, group, values);
      assert.equal(visible.length, values.length, script + ': ' + group);
      const keys = Array.from(visible).filter(Boolean);
      assert.equal(new Set(keys).size, keys.length, script + ': duplicate ' + group);
    }
  }
  const values = script => nativeKeyboardValues(script, 'consonants', KEYBOARD_MAPS[script].consonants);
  assert.equal(values('devanagari')[45], '');
  assert.equal(values('sinhala')[45], 'ඟ');
  assert.equal(values('tamil')[1], 'க²');
  assert.equal(values('tamil')[7], 'ச³');
  assert.equal(values('tamil')[8], 'ச⁴');
  assert.equal(values('tamil')[37], 'ஜ');
  assert.equal(nativeKeyboardExtras('tamil').length, 0);
  assert.equal(values('tamil')[45], '');
});

test('Tamil J composes as a consonant', () => {
  const layout = layoutFor('tamil');
  const consonants = Array.from(layout.consonants, (value, index) =>
    index === 7 ? 'ச³' : index === 8 ? 'ச⁴' : index === 37 ? 'ஜ' : value);
  const convert = createAsciiComposer({ ...layout, consonants, asciiTokens: { consonants: { 37: 'J' } } });
  assert.equal(convert('J'), 'ஜ');
  assert.equal(convert('Ji'), 'ஜி');
  assert.equal(convert('i', 'ஜ'), 'ி');
});

test('script-specific shortcuts compose their displayed keys without changing existing mappings', () => {
  const extras = nativeKeyboardExtras('burmese');
  assert.deepEqual(Array.from(extras, entry => entry.value), ['ျ', 'ြ', 'ွ', 'ှ', '္']);
  const convert = createAsciiComposer({ ...layoutFor('burmese'), nativeExtras: extras });
  assert.equal(convert('m.x1'), 'မြ');
  assert.equal(convert('m.x1i'), 'မြိ');
  assert.equal(convert('k'), 'က');
  assert.equal(createAsciiComposer({ ...layoutFor('malayalam'), nativeExtras: nativeKeyboardExtras('malayalam') })('.x0'), 'ൺ');
});

test('every character slot has a unique ASCII shortcut', () => {
  const tokens = Object.values(ASCII_KEYBOARD_TOKENS).flat();
  assert.equal(new Set(tokens).size, tokens.length);
  assert.ok(tokens.every(token => /^[\x21-\x7e]+$/.test(token)));
  for (const script of Object.keys(KEYBOARD_MAPS)) {
    const layout = layoutFor(script);
    const convert = createAsciiComposer(layout);
    for (const [group, shortcuts] of Object.entries(ASCII_KEYBOARD_TOKENS)) {
      if (group === 'accents' && layout.accents.length === 0) { assert.equal(layout.accents.length, 0); continue; }
      assert.equal(shortcuts.length, layout[group].length, script + ': ' + group);
      shortcuts.forEach((token, index) => {
        if (layout[group][index]) assert.equal(convert(token), layout[group][index], script + ': ' + token);
      });
    }
  }
});

test('Roman source schemes support ASCII typing with explicit vowels', () => {
  for (const script of ['hk', 'iast', 'ipa', 'iso', 'itrans', 'slp1', 'titus', 'velthuis']) {
    const layout = layoutFor(script);
    const convert = createAsciiComposer(layout);
    assert.equal(convert('ka'), 'k' + layout.vowels[0], script + ' keeps explicit inherent vowel');
    assert.equal(convert('kA'), 'k' + layout.vowels[1], script + ' composes long vowel');
    assert.equal(convert('k:i'), 'k' + layout.vowels[2], script + ' forces an independent vowel');
  }
  assert.equal(createAsciiComposer(layoutFor('iso'))('kA'), 'kā');
  assert.equal(createAsciiComposer(layoutFor('iast'))('kA'), 'kā');
  assert.equal(createAsciiComposer(layoutFor('itrans'))('kA'), 'kaa');
});

test('vowels become signs only immediately after a consonant', () => {
  for (const [ascii, expected] of Object.entries({
    k: 'क', i: 'इ', ki: 'कि', kI: 'की', ka: 'क', kai: 'कै', kri: 'करि',
    'k:i': 'कइ', 'k;h': 'कह', kh: 'ख', 'kh;i': 'खइ', 'k`Sh.i': 'क्षि',
    'k.i': 'कि', 'R^i': 'ऋ', 'kR^i': 'कृ', 'ki:i': 'किइ'
  })) assert.equal(compose(ascii), expected, ascii);
  assert.equal(compose('i', 'क'), 'ि');
  assert.equal(compose('i', 'का'), 'इ');
  assert.equal(compose('i', 'क्'), 'इ');
  assert.equal(createAsciiComposer(layoutFor('telugu'))('ki'), 'కి');
  assert.equal(createAsciiComposer(layoutFor('kannada'))('ki'), 'ಕಿ');
});

test('Grantha keyboard composes supplementary letters, signs and conjuncts', () => {
  const convert = createAsciiComposer(layoutFor('grantha'));
  assert.equal(KEYBOARD_MAPS.tamilExtended, undefined);
  for (const [ascii, expected] of Object.entries({
    'k`Sh.i': '𑌕𑍍𑌷𑌿', kO: '𑌕𑍋', 'kR^I': '𑌕𑍄', OM: '𑍐'
  })) assert.equal(convert(ascii), expected, ascii);
});

test('Indlish collisions, symbols and accents remain distinct', () => {
  for (const [ascii, expected] of Object.entries({
    n: 'न', ng: 'ङ', ny: 'ञ', nh: 'ऩ', sh: 'श', Sh: 'ष', gh: 'घ', G: 'ग़',
    D: 'ड', Rd: 'ड़', DH: 'ढ', Rdh: 'ढ़',
    M: 'ं', H: 'ः', '~M': 'ँ', '`': '्', OM: 'ॐ', '!': '।', '!!': '॥',
    '~g': 'ꣳ', '~G': 'ꣴ', '^1': '॒', '^2': '॑', '^3': '᳚'
  })) assert.equal(compose(ascii), expected, ascii);
  assert.equal(createAsciiComposer(layoutFor('sinhala'))('~ng'), 'ඟ');
});

test('incomplete shortcuts, separators and literal escapes are deterministic', () => {
  assert.equal(compose('k.'), 'क.');
  assert.equal(compose('k.i'), 'कि');
  assert.equal(compose('k;h'), 'कह');
  assert.equal(compose('k h'), 'क ह');
  assert.equal(compose('\\k'), 'k');
  assert.equal(compose('k\\i'), 'कi');
  assert.equal(compose('\\;'), ';');
  assert.equal(compose('\\\\'), '\\');
  assert.equal(compose('\\`'), '`');
});

function inputHarness({ mobile = true, mode = mobile ? 'ascii' : 'direct', disabled = false, convert = compose } = {}) {
  const listeners = new Map();
  const sourceText = {
    value: '', selectionStart: 0, selectionEnd: 0,
    addEventListener(name, handler) {
      if (!listeners.has(name)) listeners.set(name, []);
      listeners.get(name).push(handler);
    }
  };
  const dispatch = (name, event = {}) => {
    event.preventDefault = () => { event.defaultPrevented = true; };
    for (const listener of listeners.get(name) || []) listener(event);
    return event;
  };
  const replace = (start, end, text) => {
    sourceText.value = sourceText.value.slice(0, start) + text + sourceText.value.slice(end);
    sourceText.selectionStart = sourceText.selectionEnd = start + text.length;
    dispatch('input');
  };
  const code = html.slice(html.indexOf("const inputMode = element('keyboard-mode')"), html.indexOf('// All groups checked by default'));
  runInNewContext(code + ';composeAscii = convert;', {
    sourceText, element: () => ({ value: mode, disabled, addEventListener() {} }),
    window: { matchMedia: () => ({ matches: mobile }) },
    editor: { replaceInputRange: replace }, convert
  });
  return {
    sourceText, dispatch, replace,
    type(text) { return dispatch('beforeinput', { cancelable: true, inputType: 'insertText', data: text }); },
    commit(text) {
      dispatch('compositionstart');
      replace(sourceText.selectionStart, sourceText.selectionEnd, text);
      dispatch('compositionend');
    }
  };
}

test('ASCII and Native modes work on desktop and mobile, even for Roman keyboards', () => {
  const mobile = inputHarness();
  mobile.type('k'); mobile.type('i');
  assert.equal(mobile.sourceText.value, 'कि');
  for (const settings of [{ mobile: false }, { mobile: true, mode: 'direct' }]) {
    const input = inputHarness(settings);
    assert.equal(input.type('ki').defaultPrevented, undefined);
  }
  const iso = inputHarness({ mobile: false, mode: 'ascii', disabled: true,
    convert: createAsciiComposer(layoutFor('iso')) });
  iso.type('k'); iso.type('a'); iso.type('A');
  assert.equal(iso.sourceText.value, 'kaā');
  assert.equal(iso.type('i').defaultPrevented, true);
});

test('phone composition commits convert once and extend previous ASCII shortcuts', () => {
  const input = inputHarness();
  input.commit('ki');
  assert.equal(input.sourceText.value, 'कि');
  input.type('k');
  input.commit('h');
  assert.equal(input.sourceText.value, 'किख');
  input.sourceText.selectionStart = 0;
  input.sourceText.selectionEnd = 2;
  input.commit('sh');
  assert.equal(input.sourceText.value, 'शख');
});

test('native-script composition and paste bypass ASCII conversion', () => {
  const input = inputHarness();
  input.commit('தமிழ்');
  assert.equal(input.sourceText.value, 'தமிழ்');
  input.dispatch('paste');
  input.replace(0, input.sourceText.value.length, 'ki');
  assert.equal(input.sourceText.value, 'ki');
});

test('Urdu ASCII vowels use explicit points and separate independent carriers', () => {
  const convert = createAsciiComposer(layoutFor('urdu'));
  assert.equal(convert('ka'), 'کَ');
  assert.equal(convert('ki'), 'کِ');
  assert.equal(convert('kI'), 'کِی');
  assert.equal(convert('i'), 'اِ');
});

test('script families and accent keyboards match the canonical data', () => {
  const data = JSON.parse(readFileSync(join(__dirname, '../../scripts/script_data.json'), 'utf8'));
  const groups = Object.values(SCRIPT_CATALOG).filter(entry => entry.separator).map(entry => entry.label);
  assert.deepEqual(Array.from(groups), ['── Indic ──', '── Roman ──', '── Perso-Arabic ──']);
  let family;
  const families = ['indic', 'roman', 'persoArabic'];
  let index = 0;
  for (const [id, entry] of Object.entries(SCRIPT_CATALOG)) {
    if (entry.separator) { family = families[index++]; continue; }
    if (id === 'indic') { assert.equal(entry.target, false); continue; }
    assert.ok(Object.hasOwn(data[family], id), id + ': wrong dropdown group');
  }
  for (const [kind, scripts] of Object.entries(data)) {
    for (const [id, info] of Object.entries(scripts)) {
      const entry = SCRIPT_CATALOG[id];
      assert.equal(entry.kind, kind, id);
      assert.equal(entry.supportsVedicAccents, info.supportsVedicAccents, id);
      assert.equal(keyboardAccents(id).length, info.supportsVedicAccents ? 3 : 0, id);
    }
  }
  assert.equal(SCRIPT_CATALOG.tamil.kind, 'indic');
  assert.equal(keyboardAccents('tamil')[0], '॒');
  assert.equal(keyboardAccents('iso')[0], '̱');
});

test('native keyboard keys still display the ASCII shortcuts and accessible labels', () => {
  const code = html.slice(html.indexOf('function createKeyButton('), html.indexOf('function renderKeyboardKeys()'));
  function node() {
    return {
      dataset: {}, children: [], classList: { add() {} },
      append(child) { this.children.push(child); },
      setAttribute(name, value) { this[name] = value; },
      getAttribute(name) { return this[name]; },
      addEventListener() {}
    };
  }
  const createKeyButton = runInNewContext(code + ';createKeyButton', {
    document: { createElement: node }, keyboardIsRoman: () => true,
    keyCategory: () => ({ color: 'vowel', label: 'Vowel' }),
    KEYBOARD_COLORS: { vowel: 'purple' },
    keyboardKeyDisplay: value => value,
    keyboardShortcut: () => 'A',
    asciiEnabled: () => false
  });
  const button = createKeyButton('ā', '', { id: 'vowels', label: 'Vowels' });
  assert.equal(button.children[0].textContent, 'ā');
  assert.equal(button.children[1].className, 'keyboard-key-shortcut');
  assert.equal(button.children[1].textContent, 'A');
  assert.match(button.title, /ASCII: A/);
  assert.match(button['aria-label'], /ASCII A/);
});

test('Mixed Indic typing label follows the active layout and hides on other source scripts', () => {
  const code = html.slice(html.indexOf('function renderKeyboard()'), html.indexOf('function populateKeyboardControls()'));
  const sourceScript = { value: 'indic' };
  const typing = { hidden: true };
  const keyboardScript = {
    disabled: true, options: [],
    replaceChildren() { this.options = []; },
    add(value) { this.options.push(value); }
  };
  const mode = { value: 'direct', disabled: false };
  const nodes = { 'keyboard-script': keyboardScript, 'mobile-typing-script': typing,
    'ascii-keyboard-help': { hidden: true } };
  const ctx = {
    sourceScript, inputMode: mode, element: id => nodes[id], resetAscii() {}, asciiEnabled: () => false,
    keyboardIsRoman: () => sourceScript.value === 'iso',
    keyboardLayout: () => ({ vowels: ['a'], consonants: [], symbols: [], vedicSymbols: [] }),
    createAsciiComposer: () => () => '',
    keyboardAccents: () => [], nativeKeyboardExtras: () => [],
    renderKeyboardKeys() {}, renderKeyboardToggles() {}, renderMobileReference() {},
    SCRIPT_CATALOG: {
      devanagari: { label: 'Devanagari' },
      tamil: { label: 'Tamil' }, iso: { label: 'ISO 15919' }
    },
    INDIC_SCRIPTS: ['devanagari', 'tamil'],
    Option: class { constructor(text, value) { this.text = text; this.value = value; } }
  };
  const controller = runInNewContext('let activeIndicKeyboardScript = "devanagari"; let composeAscii; '
    + 'function activeKeyboardScript() { return sourceScript.value === "indic" ? activeIndicKeyboardScript : sourceScript.value; }'
    + code + ';({ renderKeyboard, select: value => { activeIndicKeyboardScript = value; } })', ctx);
  controller.renderKeyboard();
  assert.equal(typing.textContent, 'Typing: Devanagari');
  assert.equal(typing.hidden, false);
  assert.equal(keyboardScript.disabled, false);
  controller.select('tamil');
  controller.renderKeyboard();
  assert.equal(typing.textContent, 'Typing: Tamil');
  sourceScript.value = 'iso';
  controller.renderKeyboard();
  assert.equal(typing.hidden, true);
  assert.equal(mode.disabled, false);
});
