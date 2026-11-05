const { test } = require('node:test');
const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { runInNewContext } = require('node:vm');
const { join } = require('node:path');
const source = name => readFileSync(join(__dirname, '..', name), 'utf8');
const api = runInNewContext(source('../demo/keyboard-data.js') + '\n' + source('vedic-review-data.js') + '\n({ ...VedicReview, nativeKeyboardValues })');
const native = (script, entry, group) => api.nativeKeyboardValues(script, group, entry.keyboard[group]).filter(Boolean);
test('review corpus covers every supported Indic script and exact three accents', () => {
  assert.equal(api.scripts.map(([id]) => id).join(','),
    'assamese,bengali,devanagari,gujarati,gurmukhi,kannada,malayalam,oriya,tamil,telugu');
  assert.equal(api.accents.map(a => a.text).join(''), '॒॑᳚');
  for (const [script, entry] of api.scripts) {
    const consonants = new Set(native(script, entry, 'consonants'));
    const vowels = api.stems(script, 'vowels');
    assert.equal(new Set(vowels).size, vowels.length);
    for (const vowel of native(script, entry, 'vowels')) assert.ok(vowels.includes(vowel));
    const pairs = api.stems(script, 'conjuncts');
    assert.equal(pairs.length, consonants.size ** 2);
    const c = [...consonants][0], virama = entry.keyboard.vowelMarks[0];
    assert.ok(pairs.includes(c + virama + c));
    const signs = api.stems(script, 'conjunctSigns');
    assert.ok(signs.includes(c + virama + c + entry.keyboard.vowelMarks[1]));
  }
});
test('review keys retain script and exact Unicode sequences', () => {
  assert.notEqual(api.key('tamil', 'க', '॒'), api.key('tamil', 'க', '॑'));
  assert.notEqual(api.key('bengali', 'ক', '॒'), api.key('assamese', 'ক', '॒'));
  assert.equal(api.codepoints('க॒').join(' '), 'U+0B95 U+0952');
  assert.equal(api.codepoints('𑌕').join(' '), 'U+11315');
});

test('all combinations deduplicates and includes each generated group', () => {
  const all = api.stems('telugu', 'all');
  const combined = new Set(all);
  assert.equal(combined.size, all.length);
  for (const group of ['vowels', 'consonants', 'signs', 'diacritics', 'conjuncts', 'conjunctSigns']) {
    for (const stem of api.stems('telugu', group)) assert.ok(combined.has(stem));
  }
});

test('legacy reviews migrate without assuming the other axis is okay', () => {
  const review = api.normalizeReview({ status: 'collision', note: 'original note', text: 'क॒' });
  assert.equal(review.vertical, 'collision');
  assert.equal(review.horizontal, null);
  assert.equal(review.note, 'original note');
  assert.equal(review.status, undefined);
  assert.ok(api.isReviewed(review));
  assert.ok(api.hasProblem(review));
  assert.equal(api.isComplete(review), false);
});
test('independent axis review, unsure and note-only semantics', () => {
  const review = api.normalizeReview({ vertical: 'good', horizontal: 'left' });
  assert.ok(api.hasProblem(review));
  assert.ok(api.isComplete(review));
  assert.equal(api.hasProblem({ vertical: 'good', horizontal: 'good' }), false);
  assert.equal(api.isComplete({ vertical: 'good', horizontal: 'unsure' }), false);
  assert.ok(api.isReviewed({ horizontal: 'unsure' }));
  assert.ok(api.isReviewed({ note: 'needs reference' }));
  assert.equal(api.isReviewed(api.normalizeReview()), false);
});

test('pattern rows retain consonant columns across vowel signs and conjuncts', () => {
  for (const [script, entry] of api.scripts) {
    const rows = api.rows(script, 'signs');
    assert.equal(rows[0].label, 'Consonants');
    const consonants = [...new Set(native(script, entry, 'consonants'))];
    assert.equal(rows[0].stems.join('|'), consonants.join('|'));
    for (const row of rows.slice(1)) {
      for (let i = 0; i < row.stems.length; i++) {
        if (row.stems[i]) {
          const base = script === 'tamil' ? consonants[i].replace(/[¹²³⁴]$/, '') : consonants[i];
          assert.ok(row.stems[i].startsWith(base));
        }
      }
    }
    const all = api.rows(script, 'all').flatMap(r => r.stems.filter(Boolean));
    assert.equal(new Set(all).size, all.length);
    const set = new Set(all);
    for (const stem of api.stems(script, 'all')) assert.ok(set.has(stem));
  }
});
test('rectangular selection respects row and column coordinates and holes', () => {
  const cells = [
    { id: 'a', row: 0, column: 0 }, { id: 'b', row: 0, column: 1 },
    { id: 'c', row: 1, column: 0 }, { id: 'd', row: 1, column: 2 },
    { id: 'e', row: 2, column: 0 }
  ];
  assert.equal(api.rectangle(cells, cells[0], cells[3]).join(','), 'a,b,c,d');
  assert.equal(api.rectangle(cells, cells[3], cells[0]).join(','), 'a,b,c,d');
  assert.equal(api.rectangle(cells, cells[0], cells[2]).join(','), 'a,c');
});

test('generated corpus uses native inventories and retains Tamil superscripts', () => {
  for (const [script, entry] of api.scripts) {
    for (const [group, property] of [['vowels', 'vowels'], ['consonants', 'consonants']]) {
      assert.equal(api.stems(script, group).join('|'), [...new Set(native(script, entry, property))].join('|'));
    }
    const consonants = native(script, entry, 'consonants');
    const signs = native(script, entry, 'vowelMarks').slice(1);
    if (script !== 'tamil') assert.equal(api.stems(script, 'signs').join('|'),
      [...new Set(consonants.flatMap(c => signs.map(s => c + s)))].join('|'));
    const marks = new Set(native(script, entry, 'otherDiacritics'));
    for (const stem of api.stems(script, 'diacritics')) {
      assert.ok([...marks].some(mark => stem.endsWith(mark)));
    }
    assert.ok(api.stems(script, 'all').every(stem => !stem.includes('ˆ')));
  }
  const tamil = api.stems('tamil', 'consonants');
  for (const consonant of ['க²', 'க³', 'க⁴', 'ச²', 'ச³', 'ச⁴', 'ஜ']) assert.ok(tamil.includes(consonant));
  assert.ok(!tamil.includes('ஃக'));
  assert.ok(!api.stems('tamil', 'vowels').includes('ரு'));
  assert.ok(!api.stems('tamil', 'signs').includes('க்ரு'));
  assert.ok(!api.stems('gurmukhi', 'vowels').includes('ਰੁ'));
  assert.ok(!api.stems('gujarati', 'consonants').includes('ક઼'));
  assert.ok(!api.stems('bengali', 'consonants').includes('ভ়'));
  assert.equal(api.rows('tamil', 'custom', ['custom sample'])[0].stems[0], 'custom sample');
});

test('Tamil vowel signs precede superscripts in single consonants and conjuncts', () => {
  for (const group of ['signs', 'conjuncts', 'conjunctSigns', 'all']) {
    const stems = api.stems('tamil', group);
    const rows = api.rows('tamil', group).flatMap(row => row.stems.filter(Boolean));
    for (const samples of [stems, rows]) {
      assert.ok(samples.every(stem => !/[¹²³⁴][ாிீுூெேைொோௌ்]/u.test(stem)));
      const expected = group === 'signs' ? 'கி²' : group === 'conjuncts' ? 'க்²க³' : 'க்²கி³';
      assert.ok(samples.includes(expected), `${group}: ${expected}`);
    }
  }
  for (const stem of ['கி²', 'கீ³', 'கு⁴', 'கொ²', 'கோ³', 'கௌ⁴']) {
    assert.ok(api.stems('tamil', 'signs').includes(stem));
  }
  assert.equal(api.stems('tamil', 'signs').find(stem => stem === 'கி²') + api.accents[0].text, 'கி²॒');
});
