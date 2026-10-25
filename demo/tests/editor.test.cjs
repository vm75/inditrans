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

const displayCode = html.slice(html.indexOf('function displayIndicText'), html.indexOf('function createEditHistory'));
const { displayIndicText, canonicalIndicText, canonicalIndicSelection, applyIndicTextDisplay } = runInNewContext(
  displayCode + ';({displayIndicText, canonicalIndicText, canonicalIndicSelection, applyIndicTextDisplay})'
);

test('Telugu separator follows accents only in display, with exact canonical round-trip', () => {
  for (const vowel of ['ె॒', 'ే॒', 'ొ॒', 'ఎ᳴', 'ఒ᳴']) {
    for (const accent of ['॑', '॒', '᳚', '॒॑']) {
      const original = 'ధ్య' + vowel + '\u200b' + accent;
      const display = 'ధ్య' + vowel + accent + '\u200b';
      assert.equal(displayIndicText(original), display);
      assert.equal(canonicalIndicText(display), original);
      assert.equal(displayIndicText(display), display);
      assert.equal(display.length, original.length);
    }
  }
  const unchanged = 'ధ్య॑ ध्य॒ ధ్యె॒\u200b text\u200b॑';
  assert.equal(displayIndicText(unchanged), unchanged);
  assert.equal(canonicalIndicText(unchanged), unchanged);
});

test('Telugu display adjustment preserves editing selection and scroll position', () => {
  const original = 'ధ్యె॒\u200b॑';
  const text = {
    value: original, selectionStart: original.indexOf('॑'), selectionEnd: original.length,
    selectionDirection: 'forward', scrollTop: 100, scrollLeft: 12,
    setSelectionRange(start, end, direction) {
      this.selectionStart = start; this.selectionEnd = end; this.selectionDirection = direction;
    }
  };
  applyIndicTextDisplay(text);
  assert.equal(text.value, 'ధ్యె॒॑\u200b');
  assert.equal(text.selectionStart, original.indexOf('॑') - 1);
  assert.equal(text.selectionEnd, original.length);
  assert.equal(text.scrollTop, 100);
  assert.equal(text.scrollLeft, 12);
});


test('additional typed accents join an already adjusted Telugu marker', () => {
  assert.equal(displayIndicText('ధ్యె॒॑\u200b॒'), 'ధ్యె॒॒॑\u200b');
  assert.equal(canonicalIndicText(displayIndicText('ధ్యె॒॑\u200b॒')), 'ధ్యె॒\u200b॒॑');
});

test('word selection includes the invisible Telugu vowel marker when copying', () => {
  const value = 'ధ్యె॒॑\u200b';
  const selection = canonicalIndicSelection({ value, selectionStart: 0, selectionEnd: value.length - 1 });
  assert.equal(selection.value, 'ధ్యె॒\u200b॑');
  assert.equal(selection.end, value.length);
});
