const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const { runInNewContext } = require('node:vm');
const { test } = require('node:test');
const assert = require('node:assert/strict');
const html = readFileSync(join(__dirname, '../index.html'), 'utf8');
const library = html.slice(html.indexOf('let devFontSettings'), html.indexOf('// Add hosted families'));
const appearanceCode = html.slice(html.indexOf('function createAppearance('), html.indexOf('function createTargetOptions('));
const toolsCode = html.slice(html.indexOf('function createFontDevTools('), html.indexOf('function startApp()'));

function node(value = '') {
  return {
    value, dataset: {}, style: {}, events: {}, children: [],
    addEventListener(name, fn) { this.events[name] = fn; },
    append(...children) { this.children.push(...children); },
    replaceChildren() { this.children = []; this.value = ''; },
    get options() { return this.children; },
    add(option) { this.children.push(option); if (this.children.length === 1) this.value = option.value; },
    remove() { this.removed = true; },
    showModal() { this.open = true; }, close() { this.open = false; }
  };
}

function harness({ queryLocalFonts, fetch = async () => { throw new Error('Offline'); } } = {}) {
  const nodes = new Proxy({}, { get: (all, id) => all[id] ||= node() });
  const radios = ['full', 'script', 'solo', 'custom'].map(node);
  nodes['font-devtools'].querySelectorAll = () => radios;
  const sourceScript = node('tamil'), targetScript = node('devanagari');
  const sourceFont = node(), targetFont = node(), sourceText = node(), targetText = node();
  const tags = [], pendingLoads = [];
  const fonts = Object.assign([], { addEventListener() {}, load(...args) {
    return new Promise((resolve, reject) => pendingLoads.push({ args, resolve, reject }));
  } });
  const document = { fonts, createElement: () => node(), addEventListener() {}, head: { append(tag) { tags.push(tag); } } };
  let keyboardUpdates = 0;
  const api = runInNewContext(library + appearanceCode + toolsCode + `;
    const appearance = createAppearance(args);
    function refreshFonts() {
      appearance.populateFonts(args.sourceFont, args.sourceScript.value, false);
      appearance.populateFonts(args.targetFont, args.targetScript.value, false);
    }
    refreshFonts();
    const tools = createFontDevTools({ ...args, ...appearance, refreshFonts, updateKeyboardFont });
    ({ ...appearance, ...tools, refreshFonts, additionalFonts, devFontSettings, fontHasGlyph, loadFontStylesheet });`, {
    element: id => nodes[id], document, window: { queryLocalFonts }, fetch, AbortSignal, DataView,
    setTimeout, clearTimeout,
    Option: function(text, value) { return Object.assign(node(value), { text }); },
    SCRIPT_CATALOG: { tamil: { kind: 'indic', fonts: ['Tamil Base'] }, devanagari: { kind: 'indic', fonts: ['Devanagari Base'] },
      iast: {}, indic: {} }, LATIN_FONTS: ['Inter'],
    args: { sourceScript, targetScript, sourceFont, targetFont, sourceText, targetText, onDiscoverFonts: () => nodes['font-devtools'].showModal() },
    updateKeyboardFont() { keyboardUpdates++; }
  });
  return { api, nodes, radios, sourceScript, targetScript, sourceFont, targetFont, sourceText, targetText,
    tags, fonts, pendingLoads, keyboardUpdates: () => keyboardUpdates };
}

async function tick() { await new Promise(resolve => setImmediate(resolve)); }
function face(family, status = 'loaded') { return { family, weight: '400', style: 'normal', status }; }

test('custom mode applies the visible effective stack immediately', () => {
  const h = harness();
  const expected = h.sourceText.style.fontFamily;
  h.radios[3].events.change();
  assert.equal(h.nodes['devfont-custom-stack'].value, expected);
  assert.equal(h.api.devFontSettings.customStack, expected);
  assert.equal(h.targetText.style.fontFamily, expected);
  h.sourceFont.value = 'Find more fonts…';
  h.api.applyFont(h.sourceFont);
  assert.equal(h.sourceFont.value, 'Tamil Base');
  assert.equal(h.nodes['font-devtools'].open, true);
});

test('manual loading waits for CSS and font bytes, survives script switches, and removal reapplies panes', async () => {
  const h = harness();
  h.nodes['devfont-family'].value = 'Test Family';
  h.nodes['devfont-apply-to'].value = 'both';
  const loading = h.nodes['devfont-load'].events.click();
  assert.equal(h.nodes['devfont-load-status'].textContent, 'Loading…');
  assert.equal(h.sourceFont.value, 'Tamil Base');
  h.fonts.push(face('Test Family'));
  h.tags[0].onload();
  await tick();
  assert.equal(h.nodes['devfont-load-status'].textContent, 'Loading…');
  h.pendingLoads[0].resolve([h.fonts[0]]);
  await loading;
  assert.match(h.nodes['devfont-load-status'].textContent, /✓ Loaded/);
  assert.equal(h.sourceFont.value, 'Test Family');
  h.sourceScript.value = 'iast';
  h.api.refreshFonts();
  assert.equal(h.sourceFont.value, 'Test Family');
  const before = h.keyboardUpdates();
  h.nodes['devfont-dynamic-list'].children[1].children[2].events.click();
  assert.equal(h.tags[0].removed, true);
  assert.equal(h.sourceFont.value, 'Inter');
  assert.equal(h.targetFont.value, 'Devanagari Base');
  assert.match(h.sourceText.style.fontFamily, /^"Inter"/);
  assert.match(h.targetText.style.fontFamily, /^"Devanagari Base"/);
  assert.ok(h.keyboardUpdates() > before);
});

test('stylesheet and font failures remove the tag and do not register a family', async () => {
  for (const stage of ['stylesheet', 'font', 'missing family']) {
    const h = harness();
    h.nodes['devfont-family'].value = 'Broken';
    const loading = h.nodes['devfont-load'].events.click();
    if (stage === 'stylesheet') h.tags[0].onerror();
    else {
      if (stage === 'font') h.fonts.push(face('Broken'));
      h.tags[0].onload();
      await tick();
      if (stage === 'font') h.pendingLoads[0].reject(new Error('Font download failed'));
    }
    await loading;
    assert.match(h.nodes['devfont-load-status'].textContent, /^✗/);
    assert.equal(h.api.additionalFonts.has('Broken'), false);
    assert.equal(h.tags[0].removed, true);
    assert.equal(h.nodes['devfont-load'].disabled, false);
  }
});

test('online discovery filters by subset and fetches font CSS only when selected', async () => {
  const catalog = [
    { family: 'New Tamil', subsets: ['tamil', 'latin'], weights: [400], styles: ['normal'] },
    { family: 'New Devanagari', subsets: ['devanagari'], weights: [400], styles: ['normal'] },
    { family: 'Italic Only', subsets: ['tamil'], weights: [400], styles: ['italic'] }
  ];
  const h = harness({ fetch: async () => ({ ok: true, json: async () => catalog }) });
  await h.nodes['devfont-discover-online'].events.click();
  assert.ok(h.sourceFont.options.some(o => o.value === 'New Tamil'));
  assert.ok(!h.sourceFont.options.some(o => o.value === 'New Devanagari'));
  assert.ok(!h.sourceFont.options.some(o => o.value === 'Italic Only'));
  assert.ok(h.targetFont.options.some(o => o.value === 'New Devanagari'));
  assert.equal(h.tags.length, 0);
  h.sourceFont.value = 'New Tamil';
  h.api.applyFont(h.sourceFont);
  h.fonts.push(face('New Tamil'));
  h.tags[0].onload();
  await tick();
  assert.equal(h.pendingLoads[0].args[1], 'கமா');
  h.pendingLoads[0].resolve([h.fonts[0]]);
  await tick();
  h.sourceScript.value = 'devanagari';
  h.api.refreshFonts();
  assert.ok(!h.sourceFont.options.some(o => o.value === 'New Tamil'));
  assert.ok(h.sourceFont.options.some(o => o.value === 'New Devanagari'));
});

test('discovery failures remain retryable and unsupported local discovery stays disabled', async () => {
  const h = harness();
  assert.equal(h.nodes['devfont-discover-local'].disabled, true);
  await h.nodes['devfont-discover-online'].events.click();
  assert.match(h.nodes['devfont-discovery-status'].textContent, /Offline/);
  assert.equal(h.nodes['devfont-discover-online'].disabled, false);
  const denied = harness({ queryLocalFonts: async () => { throw new Error('Permission denied'); } });
  await denied.nodes['devfont-discover-local'].events.click();
  assert.match(denied.nodes['devfont-discovery-status'].textContent, /Permission denied/);
  assert.equal(denied.nodes['devfont-discover-local'].disabled, false);
});

// Minimal sfnt with a Unicode format 12 cmap; each code point maps to a real glyph.
function localFont(points) {
  const buffer = new ArrayBuffer(56 + points.length * 12);
  const d = new DataView(buffer);
  d.setUint16(4, 1); d.setUint32(12, 0x636d6170); d.setUint32(20, 28);
  d.setUint16(30, 1); d.setUint16(32, 3); d.setUint16(34, 10); d.setUint32(36, 12);
  d.setUint16(40, 12); d.setUint32(52, points.length);
  points.forEach((point, i) => { const offset = 56 + i * 12;
    d.setUint32(offset, point); d.setUint32(offset + 4, point); d.setUint32(offset + 8, i + 1);
  });
  return buffer;
}

test('installed discovery requires glyph coverage, keeps files local, and filters scripts', async () => {
  const buffer = localFont([...new Set([...'கமா'].map(char => char.codePointAt(0)))]);
  const h = harness({ queryLocalFonts: async () => [{ family: 'Local Tamil', blob: async () => ({ arrayBuffer: async () => buffer }) }] });
  await h.nodes['devfont-discover-local'].events.click();
  assert.ok(h.sourceFont.options.some(o => o.value === 'Local Tamil'));
  assert.ok(!h.targetFont.options.some(o => o.value === 'Local Tamil'));
  assert.equal(h.tags.length, 0);
  assert.equal(h.api.fontHasGlyph(new ArrayBuffer(2), 0x0b95), false);
  assert.equal(h.api.fontHasGlyph(buffer, 0x0b95), true);
  assert.equal(h.api.fontHasGlyph(buffer, 0x0915), false);
});

test('local coverage handles format 4 delta and glyph-array mappings, including missing glyphs', () => {
  const h = harness();
  const buffer = new ArrayBuffer(78);
  const d = new DataView(buffer);
  d.setUint16(4, 1); d.setUint32(12, 0x636d6170); d.setUint32(20, 28);
  d.setUint16(30, 1); d.setUint16(32, 3); d.setUint16(34, 1); d.setUint32(36, 12);
  const sub = 40;
  d.setUint16(sub, 4); d.setUint16(sub + 2, 38); d.setUint16(sub + 6, 4);
  // Two segments: U+0041–0042, then the sentinel U+FFFF.
  d.setUint16(sub + 14, 0x42); d.setUint16(sub + 16, 0xffff);
  d.setUint16(sub + 20, 0x41); d.setUint16(sub + 22, 0xffff);
  d.setUint16(sub + 24, 1); d.setUint16(sub + 26, 1);
  assert.equal(h.api.fontHasGlyph(buffer, 0x41), true);
  assert.equal(h.api.fontHasGlyph(buffer, 0x43), false);
  d.setUint16(sub + 28, 4); // Offset into glyph array, relative to this word.
  d.setUint16(sub + 32, 5); d.setUint16(sub + 34, 0);
  assert.equal(h.api.fontHasGlyph(buffer, 0x41), true);
  assert.equal(h.api.fontHasGlyph(buffer, 0x42), false);
  d.setUint16(sub + 24, 0xffff); d.setUint16(sub + 32, 1);
  assert.equal(h.api.fontHasGlyph(buffer, 0x41), false); // Delta maps to glyph zero.
});

test('font inspector distinguishes idle, loading, and failed faces', () => {
  const h = harness();
  h.fonts.push(face('Idle', 'unloaded'), face('Loading', 'loading'), face('Broken', 'error'));
  h.api.openDialog();
  const rows = h.nodes['devfont-loaded-list'].children;
  const find = family => rows.find(row => row.children[1].textContent === family);
  assert.equal(find('Idle').children[0].className, 'devfont-status-dot');
  assert.match(find('Idle').children[2].textContent, /idle/);
  assert.match(find('Loading').children[0].className, /devfont-partial/);
  assert.match(find('Broken').children[2].textContent, /error/);
});
