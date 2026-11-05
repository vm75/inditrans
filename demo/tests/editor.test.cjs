const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const { runInNewContext } = require('node:vm');
const { test } = require('node:test');
const assert = require('node:assert/strict');

const html = readFileSync(join(__dirname, '../index.html'), 'utf8');
const code = html.slice(html.indexOf('function createEditHistory'), html.indexOf('function createEditor'));
const createHistory = runInNewContext(code + ';createEditHistory');
const state = (value, start = value.length, end = start) => ({ value, start, end });

test('undo and redo restore text and selection snapshots', () => {
  const history = createHistory();
  const initial = state('क', 0, 1);
  const typed = state('कि');
  const edited = state('खि');
  history.record(initial, typed);
  history.record(typed, edited);
  assert.equal(history.canUndo(), true);
  assert.equal(history.canRedo(), false);
  assert.deepEqual(history.undo(edited), typed);
  assert.deepEqual(history.undo(typed), initial);
  assert.equal(history.undo(initial), undefined);
  assert.deepEqual(history.redo(initial), typed);
  assert.deepEqual(history.redo(typed), edited);
  assert.equal(history.redo(edited), undefined);
});

test('new edits discard redo, while unchanged text creates no history entry', () => {
  const history = createHistory();
  history.record(state('a'), state('ab'));
  history.undo(state('ab'));
  history.record(state('a', 0), state('a', 1));
  assert.equal(history.canRedo(), true);
  assert.equal(history.canUndo(), false);
  history.record(state('a'), state('ac'));
  assert.equal(history.canRedo(), false);
  assert.equal(history.redo(state('ac')), undefined);
});

test('history is bounded and reset clears both directions', () => {
  const history = createHistory(2);
  for (const [before, after] of [['a', 'ab'], ['ab', 'abc'], ['abc', 'abcd']]) history.record(state(before), state(after));
  assert.deepEqual(history.undo(state('abcd')), state('abc'));
  assert.deepEqual(history.undo(state('abc')), state('ab'));
  assert.equal(history.undo(state('ab')), undefined);
  history.redo(state('ab'));
  history.reset();
  assert.equal(history.canUndo(), false);
  assert.equal(history.canRedo(), false);
});

test('editor has no display/canonical text adapter', () => {
  assert.doesNotMatch(html, /displayIndicText|canonicalIndicText|canonicalIndicSelection|applyIndicTextDisplay/);
  assert.match(html, /navigator\.clipboard\.writeText\(text\.value\)/);
  assert.match(html, /sourceText: sourceText\.value, targetText: targetText\.value/);
});

function editorFixture(value = '') {
  class Control {
    constructor(value = '') {
      this.value = value;
      this.selectionStart = this.selectionEnd = value.length;
      this.selectionDirection = 'none';
      this.scrollTop = 12;
      this.scrollLeft = 3;
      this.listeners = {};
    }
    addEventListener(type, listener) { (this.listeners[type] ||= []).push(listener); }
    dispatchEvent(event) { for (const listener of this.listeners[event.type] || []) listener(event); }
    setSelectionRange(start, end, direction = 'none') {
      this.selectionStart = start; this.selectionEnd = end; this.selectionDirection = direction;
    }
    setRangeText(text, start, end) {
      this.value = this.value.slice(0, start) + text + this.value.slice(end);
      this.setSelectionRange(start + text.length, start + text.length);
    }
    focus() { document.activeElement = this; }
  }
  const controls = new Map();
  const element = id => {
    if (!controls.has(id)) controls.set(id, new Control());
    return controls.get(id);
  };
  const document = { addEventListener() {}, activeElement: null };
  const source = new Control(value), target = new Control();
  document.activeElement = source;
  const code = html.slice(html.indexOf('function createEditHistory'), html.indexOf('function createAsciiComposer'));
  const createEditor = runInNewContext(code + ';createEditor', { document, element, Event });
  const editor = createEditor(source, target);
  return { source, editor, click: id => element(id).dispatchEvent({ type: 'click' }),
    type(text, isComposing = false) {
      source.dispatchEvent({ type: 'beforeinput', inputType: 'insertText' });
      source.setRangeText(text, source.selectionStart, source.selectionEnd);
      source.dispatchEvent({ type: 'input', isComposing });
    } };
}

test('Tamil typed signs reorder with superscripts, preserve caret and undo as one edit', () => {
  const f = editorFixture('க² word');
  f.source.setSelectionRange(2, 2);
  f.type('ி');
  assert.equal(f.source.value, 'கி² word');
  assert.equal(f.source.selectionStart, 3);
  assert.equal(f.source.scrollTop, 12);
  f.click('undo-button');
  assert.equal(f.source.value, 'க² word');
  assert.equal(f.source.selectionStart, 2);
  f.click('redo-button');
  assert.equal(f.source.value, 'கி² word');
});

test('pastes and on-screen insertions reorder Tamil signs and pulli only', () => {
  const text = 'க²ி॒ க³ொ க⁴் ச²ே கீ³ கு⁴ क²ि x²ி';
  const expected = 'கி²॒ கொ³ க்⁴ சே² கீ³ கு⁴ क²ि x²ி';
  const f = editorFixture();
  f.type(text);
  assert.equal(f.source.value, expected);
  assert.equal(f.source.selectionStart, text.length);
  const virtual = editorFixture('க²');
  virtual.editor.insertAtCursor('ி');
  assert.equal(virtual.source.value, 'கி²');
  virtual.click('undo-button');
  assert.equal(virtual.source.value, 'க²');
});

test('Tamil reordering waits for composition completion and notifies conversion', () => {
  const f = editorFixture('க²');
  let observed;
  f.source.addEventListener('input', () => { observed = f.source.value; });
  f.source.dispatchEvent({ type: 'compositionstart' });
  f.type('ி', true);
  assert.equal(f.source.value, 'க²ி');
  f.source.dispatchEvent({ type: 'compositionend' });
  assert.equal(f.source.value, 'கி²');
  assert.equal(observed, 'கி²');
  f.click('undo-button');
  assert.equal(f.source.value, 'க²');
});
