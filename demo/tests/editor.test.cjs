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
