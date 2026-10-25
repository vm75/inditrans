// Run with: node --test demo/tests/ascii.test.cjs
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const { runInNewContext } = require('node:vm');
const { test } = require('node:test');
const assert = require('node:assert/strict');

// Exercise the buildless demo's actual tables and composer without a browser dependency.
const html = readFileSync(join(__dirname, '../index.html'), 'utf8');
const tables = html.slice(html.indexOf('const KEYBOARD_MAPS'), html.indexOf('const KEYBOARD_REFERENCE_MAP'));
const composer = html.slice(html.indexOf('function createAsciiComposer'), html.indexOf('function createKeyboard'));
const { KEYBOARD_MAPS, ASCII_KEYBOARD_TOKENS, createAsciiComposer, VEDIC_KEYBOARD_ACCENTS } = runInNewContext(
  tables + '\n' + composer + ';({ KEYBOARD_MAPS, ASCII_KEYBOARD_TOKENS, createAsciiComposer, VEDIC_KEYBOARD_ACCENTS })'
);
function layoutFor(script) {
  const layout = KEYBOARD_MAPS[script];
  return { ...layout, symbols: [...layout.symbols, ...layout.vedicSymbols], accents: VEDIC_KEYBOARD_ACCENTS };
}
const compose = createAsciiComposer(layoutFor('devanagari'));

test('every character slot has a unique ASCII shortcut', () => {
  const tokens = Object.values(ASCII_KEYBOARD_TOKENS).flat();
  assert.equal(new Set(tokens).size, tokens.length);
  assert.ok(tokens.every(token => /^[\x21-\x7e]+$/.test(token)));
  for (const script of Object.keys(KEYBOARD_MAPS)) {
    const layout = layoutFor(script);
    const convert = createAsciiComposer(layout);
    for (const [group, shortcuts] of Object.entries(ASCII_KEYBOARD_TOKENS)) {
      assert.equal(shortcuts.length, layout[group].length, script + ': ' + group);
      shortcuts.forEach((token, index) => {
        if (layout[group][index]) assert.equal(convert(token), layout[group][index], script + ': ' + token);
      });
    }
  }
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

function inputHarness({ mobile = true, mode = 'direct', disabled = false } = {}) {
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
  const code = html.slice(html.indexOf("const inputMode = element('keyboard-mode')"), html.indexOf('// Groups mapped to keyboard rows:'));
  runInNewContext(code + ';composeAscii = convert;', {
    sourceText, element: () => ({ value: mode, disabled, addEventListener() {} }),
    window: { matchMedia: () => ({ matches: mobile }) },
    editor: { replaceInputRange: replace }, convert: compose
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

test('mobile enables ASCII while preserving desktop direct mode and Roman input', () => {
  const mobile = inputHarness();
  mobile.type('k'); mobile.type('i');
  assert.equal(mobile.sourceText.value, 'कि');
  for (const settings of [{ mobile: false }, { disabled: true }]) {
    const input = inputHarness(settings);
    assert.equal(input.type('ki').defaultPrevented, undefined);
  }
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
