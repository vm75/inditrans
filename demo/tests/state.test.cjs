const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const { runInNewContext } = require('node:vm');
const { test } = require('node:test');
const assert = require('node:assert/strict');

const html = readFileSync(join(__dirname, '../index.html'), 'utf8');
const defaultsCode = readFileSync(join(__dirname, '../demo-data.js'), 'utf8');
const defaults = runInNewContext(defaultsCode + ';DEMO_DEFAULTS');
const defaultText = runInNewContext(defaultsCode + ';DEMO_SAMPLE');
test('default sample is literal mixed-script text with Indlish output', () => {
  assert.match(defaultText, /श्री/);
  assert.match(defaultText, /தமிழ்/);
  assert.match(defaultText, /##.*မြန်မာ.*##/);
  assert.equal(defaults.sourceScript, 'indic');
  assert.equal(defaults.targetScript, 'indlish');
  const populate = html.slice(html.indexOf('function populateScripts()'), html.indexOf('function populateFonts('));
  const sourceScript = { add() {} };
  const targetScript = { add() {} };
  runInNewContext(populate + ';populateScripts()', {
    sourceScript, targetScript, SCRIPT_CATALOG: {}, DEMO_DEFAULTS: defaults
  });
  assert.equal(sourceScript.value, 'indic');
  assert.equal(targetScript.value, 'indlish');
});

function conversionOptions() {
  const nodes = {};
  function node() {
    return {
      value: '##', checked: false, style: {}, events: {}, children: [],
      set id(value) { nodes[value] = this; },
      addEventListener(name, callback) { this.events[name] = callback; },
      setAttribute() {},
      append(...children) { for (const child of children) { child.parentElement = this; this.children.push(child); } },
      replaceChildren() { this.children = []; }
    };
  }
  for (const id of ['option-badge', 'option-summary', 'target-option-controls', 'skip-start', 'skip-end']) nodes[id] = node();
  const targetScript = { value: 'tamil' };
  let conversions = 0;
  const create = runInNewContext(
    defaultsCode
    + html.slice(html.indexOf('function createTargetOptions'), html.indexOf('function bindScrollSync'))
    + ';createTargetOptions', {
      element: id => nodes[id], document: { createElement: node, createTextNode: text => ({ textContent: text }) }
    });
  const api = create(targetScript, () => conversions++);
  api.populateTargetOptions();
  return { api, nodes, targetScript, conversions: () => conversions };
}

test('conversion options restore custom skip delimiters and reset all flags and delimiters', () => {
  const { api, nodes, targetScript, conversions } = conversionOptions();
  api.restoreTargetOptions({ options: ['TamilSuperscripted', 'NoXMLTagHandling'], skipStart: '<keep>', skipEnd: '</keep>' });
  assert.deepEqual(Array.from(api.selectedTargetOptions()), ['TamilSuperscripted', 'NoXMLTagHandling']);
  assert.equal(api.selectedSkipDelimiters().skipStart, '<keep>');
  assert.equal(api.selectedSkipDelimiters().skipEnd, '</keep>');
  assert.equal(nodes['option-badge'].textContent, 4);
  nodes['skip-start'].value = ' [ ';
  nodes['skip-start'].events.input();
  assert.equal(conversions(), 1);
  assert.equal(api.selectedSkipDelimiters().skipStart, ' [ ');
  targetScript.value = 'indlish';
  api.restoreTargetOptions();
  assert.deepEqual(Array.from(api.selectedTargetOptions()), []);
  assert.equal(nodes['option-TamilSuperscripted'].checked, false);
  assert.equal(nodes['option-TamilSuperscripted'].disabled, true);
  assert.equal(nodes['skip-start'].value, '##');
  assert.equal(nodes['skip-end'].value, '##');
  assert.equal(nodes['option-badge'].style.display, 'none');
});

test('missing, invalid, or empty delimiters use ## and Tamil styles stay mutually exclusive', () => {
  const { api, nodes } = conversionOptions();
  api.restoreTargetOptions({ options: ['TamilTraditional', 'TamilSuperscripted'], skipStart: null, skipEnd: 123 });
  assert.deepEqual(Array.from(api.selectedTargetOptions()), ['TamilTraditional']);
  assert.equal(api.selectedSkipDelimiters().skipStart, '##');
  nodes['skip-start'].value = nodes['skip-end'].value = '';
  assert.equal(api.selectedSkipDelimiters().skipStart, '##');
  assert.equal(api.selectedSkipDelimiters().skipEnd, '##');
});

test('reset restores the complete conversion state before replacing and converting the input', () => {
  const { api, nodes } = conversionOptions();
  api.restoreTargetOptions({ options: ['NoXMLTagHandling'], skipStart: '[', skipEnd: ']' });
  const sourceScript = { value: 'tamil' };
  const targetScript = { value: 'telugu' };
  const sourceText = { value: 'edited text', defaultValue: defaultText };
  let saved;
  const updatedFonts = [];
  let keyboardChanges = 0;
  const code = html.slice(html.indexOf('function resetDemo()'), html.indexOf('function transliterate()'));
  runInNewContext(code + ';resetDemo()', {
    DEMO_DEFAULTS: defaults, sourceScript, targetScript, sourceText, sourceFont: 'input font', targetFont: 'output font',
    populateFonts: (font, script) => updatedFonts.push([font, script]), updateLanguage() {},
    keyboard: { sourceChanged() { keyboardChanges++; } }, restoreTargetOptions: api.restoreTargetOptions,
    editor: { replaceInputRange(start, end, replacement) {
      assert.equal(start, 0);
      assert.equal(end, 'edited text'.length);
      assert.equal(sourceScript.value, 'indic');
      assert.equal(targetScript.value, 'indlish');
      assert.deepEqual(Array.from(api.selectedTargetOptions()), []);
      assert.equal(nodes['skip-start'].value, '##');
      assert.equal(nodes['skip-end'].value, '##');
      sourceText.value = replacement;
    } },
    saveSession() { saved = sourceText.value; }
  });
  assert.equal(saved, defaultText);
  assert.equal(keyboardChanges, 1);
  assert.deepEqual(updatedFonts, [['input font', 'indic'], ['output font', 'indlish']]);
  assert.match(html, /element\('sample-button'\)\.addEventListener\('click', resetDemo\)/);
});

test('demo badge matches the package version', () => {
  const { version } = JSON.parse(readFileSync(join(__dirname, '../../nodejs/package.json'), 'utf8'));
  assert.equal(html.match(/const ENGINE_VERSION = '([^']+)'/)[1], `v${version}`);
});

test('engine adapter and wrapper forward custom skip delimiters with conversion flags', async () => {
  let received;
  const loadedScripts = [];
  const api = {
    Option: { None: 0, NoXMLTagHandling: 32 }, ready: async () => {}, getSupportedScripts: () => ['indic', 'indlish'],
    transliterate(...args) { received = args; return 'converted'; }
  };
  const code = html.slice(html.indexOf('function createEngineProfile()'), html.indexOf('const element ='));
  const { createEngine, createEngineProfile } = runInNewContext(code + ';({createEngine, createEngineProfile})', {
    ENGINE_VERSION: 'v0.15.0', KEYBOARD_MAPS: {}, window: { Inditrans: api }, URL,
    document: { baseURI: 'https://example.test/inditrans/demo/', createElement: () => ({}), head: { append: script => { loadedScripts.push(script.src); script.onload(); } } }
  });
  const engine = createEngine(createEngineProfile());
  await engine.load();
  assert.deepEqual(loadedScripts, [
    'https://example.test/inditrans/js/public/inditrans.js?v=v0.15.0',
    'https://example.test/inditrans/js/inditrans-standalone.js?v=v0.15.0'
  ]);
  assert.equal(engine.transliterate(defaultText, 'indic', 'indlish', ['NoXMLTagHandling'], '[', ']'), 'converted');
  assert.deepEqual(received, [defaultText, 'indic', 'indlish', 32, '[', ']']);
});

const storageCode = html.slice(html.indexOf('const DEMO_STORAGE_KEY'), html.indexOf('const SAVED_DEMO_STATE'));
function storageApi(localStorage) {
  return runInNewContext(storageCode + `;({
    readDemoState, writeDemoState
  })`, { localStorage });
}
function memoryStorage() {
  let value = null;
  return {
    getItem: () => value,
    setItem: (_key, next) => { value = next; },
    removeItem: () => { value = null; }
  };
}

test('session storage round-trips settings and both panes, including empty text', () => {
  const api = storageApi(memoryStorage());
  const state = { engineVersion: 'v0.13.0', sourceText: '', targetText: 'कि', inputMode: 'ascii',
    skipStart: '<keep>', skipEnd: '</keep>', keyboard: { groups: [] } };
  api.writeDemoState(state);
  assert.deepEqual(JSON.parse(JSON.stringify(api.readDemoState())), state);
});

test('corrupt or obsolete storage falls back to defaults', () => {
  const storage = memoryStorage();
  const api = storageApi(storage);
  for (const value of [null, '{broken', 'null', '[]', 'false', '42', '"old state"']) {
    storage.setItem('', value);
    assert.equal(JSON.stringify(api.readDemoState()), '{}');
  }
});

test('blocked reads and quota failures do not break the demo', () => {
  const api = storageApi({
    getItem() { throw new Error('Storage blocked'); },
    setItem() { throw new Error('Quota exceeded'); }
  });
  assert.equal(JSON.stringify(api.readDemoState()), '{}');
  assert.doesNotThrow(() => api.writeDemoState({ sourceText: 'test' }));
});

function themePage({ saved = null, dark = false, blocked = false } = {}) {
  const events = {};
  const select = { addEventListener: (_name, callback) => { events.select = callback; } };
  const systemTheme = { matches: dark, addEventListener: (_name, callback) => { events.system = callback; } };
  const document = {
    documentElement: { dataset: {} },
    getElementById: () => select,
    addEventListener: (_name, callback) => { events.ready = callback; }
  };
  let stored = saved;
  const code = html.split('<script id="demo-theme">')[1].split('</script>')[0];
  runInNewContext(code, {
    document,
    window: { matchMedia: () => systemTheme },
    localStorage: {
      getItem() { if (blocked) throw Error('blocked'); return stored; },
      setItem(_key, value) { if (blocked) throw Error('blocked'); stored = value; },
      removeItem() { if (blocked) throw Error('blocked'); stored = null; }
    }
  });
  return {
    theme: () => document.documentElement.dataset.theme,
    stored: () => stored,
    ready() { events.ready(); return select.value; },
    choose(value) { select.value = value; events.select(); },
    system(dark) { systemTheme.matches = dark; events.system(); }
  };
}

test('theme follows system changes and applies saved overrides before page initialization', () => {
  const page = themePage({ dark: true });
  assert.equal(page.theme(), 'dark');
  assert.equal(page.ready(), 'system');
  page.system(false);
  assert.equal(page.theme(), 'light');
  page.choose('dark');
  assert.equal(page.theme(), 'dark');
  assert.equal(page.stored(), 'dark');
  page.system(false);
  assert.equal(page.theme(), 'dark');
  const restored = themePage({ saved: page.stored() });
  assert.equal(restored.theme(), 'dark');
  assert.equal(restored.ready(), 'dark');
  page.choose('system');
  assert.equal(page.theme(), 'light');
  assert.equal(page.stored(), null);
  page.system(true);
  assert.equal(page.theme(), 'dark');
});

test('light override, invalid preferences, and unavailable storage are handled', () => {
  const light = themePage({ saved: 'light', dark: true });
  assert.equal(light.theme(), 'light');
  assert.equal(light.ready(), 'light');
  assert.equal(themePage({ saved: 'invalid', dark: true }).theme(), 'dark');
  const blocked = themePage({ blocked: true, dark: true });
  assert.equal(blocked.theme(), 'dark');
  blocked.ready();
  assert.doesNotThrow(() => blocked.choose('light'));
  assert.equal(blocked.theme(), 'light');
  assert.doesNotThrow(() => blocked.choose('system'));
  assert.equal(blocked.theme(), 'dark');
});

const keyboardGlyphPadding = runInNewContext(
  html.slice(html.indexOf('function keyboardGlyphPadding'), html.indexOf('function measureKeyboardGlyphs')) + ';keyboardGlyphPadding'
);
test('keyboard spacing contains tall marks above and below each font baseline', () => {
  for (const fontSize of [8.5, 17, 34]) {
    for (const ascent of [fontSize * .8, fontSize * 2.5]) {
      for (const descent of [0, fontSize * 1.5]) {
        const metrics = { fontBoundingBoxAscent: fontSize, fontBoundingBoxDescent: fontSize * .4,
          actualBoundingBoxAscent: ascent, actualBoundingBoxDescent: descent };
        const lineHeight = fontSize * 1.6;
        const padding = keyboardGlyphPadding(metrics, fontSize, lineHeight);
        const baseline = (lineHeight + metrics.fontBoundingBoxAscent - metrics.fontBoundingBoxDescent) / 2;
        assert.ok(padding.top + baseline >= ascent + 2);
        assert.ok(padding.bottom + lineHeight - baseline >= descent + 2);
      }
    }
  }
});

function resizableKeyboard(viewportWidth = 1440) {
  const elements = {};
  function node(width, height) {
    const classes = new Set();
    const properties = new Map();
    return {
      clientHeight: height, children: [], events: {},
      classList: { contains: name => classes.has(name), add: name => classes.add(name), remove: name => classes.delete(name) },
      style: {
        set width(value) { properties.set('width', value); },
        get width() { return properties.get('width') || ''; },
        setProperty: (name, value) => properties.set(name, value),
        getPropertyValue: name => properties.get(name) || '',
        removeProperty: name => properties.delete(name)
      },
      getBoundingClientRect() { return { width: parseFloat(properties.get('width')) || width, height: parseFloat(properties.get('--keyboard-height')) || height }; },
      addEventListener(name, callback) { this.events[name] = callback; },
      setPointerCapture() {}
    };
  }
  const shell = node(1200, 900);
  elements['keyboard-shell'] = node(1176, 320);
  elements['keyboard-keys'] = node(1150, 240);
  elements['keyboard-resizer'] = node(1176, 12);
  elements['keyboard-corner-resizer'] = node(18, 18);
  shell.children = [node(1176, 35), node(1176, 45), elements['keyboard-shell']];
  const code = html.slice(html.indexOf('function createKeyboardResize'), html.indexOf('function createTargetOptions'));
  let saves = 0;
  const create = runInNewContext(code + ';createKeyboardResize', {
    element: id => elements[id], document: { querySelector: () => shell, fonts: { addEventListener() {} } },
    window: { innerWidth: viewportWidth, addEventListener() {} },
    ResizeObserver: class { observe() {} }, MutationObserver: class { observe() {} },
    requestAnimationFrame() { return 1; }, cancelAnimationFrame() {}
  });
  return { api: create(() => saves++), elements, shell, saves: () => saves };
}

test('keyboard resize restores valid sizes, clamps to the viewport, and resets automatic layout', () => {
  const { api, shell, elements } = resizableKeyboard();
  api.setSize({ width: 900, height: 400 });
  assert.equal(shell.style.width, '900px');
  assert.equal(api.getSize().height, 400);
  assert.equal(elements['keyboard-shell'].classList.contains('is-resized'), true);
  api.setSize({ width: 9999, height: 9999 });
  assert.equal(api.getSize().width, 1440);
  assert.ok(api.getSize().height <= 900 - 80 - 70);
  api.setSize({ width: 1, height: 1 });
  assert.equal(api.getSize().width, 420);
  assert.ok(api.getSize().height >= 200);
  for (const invalid of [null, {}, { width: '900', height: 400 }, { width: NaN, height: 400 }]) {
    api.setSize(invalid);
    assert.equal(api.getSize(), null);
    assert.equal(shell.style.width, '');
    assert.equal(elements['keyboard-shell'].classList.contains('is-resized'), false);
  }
});

test('keyboard resize supports pointer dragging, arrow keys, and double-click reset', () => {
  const { api, elements, saves } = resizableKeyboard();
  const corner = elements['keyboard-corner-resizer'];
  corner.events.pointerdown({ button: 0, clientX: 100, clientY: 500, pointerId: 1, preventDefault() {} });
  corner.events.pointermove({ clientX: 50, clientY: 450 });
  assert.equal(api.getSize().width, 1100);
  assert.equal(api.getSize().height, 370);
  corner.events.pointerup();
  assert.equal(saves(), 1);
  corner.events.keydown({ key: 'ArrowLeft', preventDefault() {} });
  assert.equal(api.getSize().width, 1076);
  const width = api.getSize().width;
  elements['keyboard-resizer'].events.keydown({ key: 'ArrowUp', preventDefault() {} });
  assert.equal(api.getSize().width, width);
  assert.ok(api.getSize().height > 370);
  corner.events.dblclick();
  assert.equal(api.getSize(), null);
  assert.equal(saves(), 4);
});


test('keyboard width can fill a desktop viewport wider than 1680 pixels', () => {
  const { api, shell } = resizableKeyboard(2560);
  api.setSize({ width: 2560, height: 400 });
  assert.equal(api.getSize().width, 2560);
  assert.equal(shell.style.width, '2560px');
  api.setSize({ width: 4000, height: 400 });
  assert.equal(api.getSize().width, 2560);
});
