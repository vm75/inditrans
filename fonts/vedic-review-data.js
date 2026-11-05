// Corpus generation is independent of rendering so exports can be reproduced.
const VedicReview = (() => {
  const scripts = Object.entries(SCRIPT_CATALOG).filter(([, s]) =>
    s.kind === 'indic' && s.supportsVedicAccents && s.keyboard);
  const accents = VEDIC_KEYBOARD_ACCENTS.map((text, i) => ({
    text, label: ['Anudātta', 'Svarita', 'Dīrgha svarita'][i]
  }));
  const unique = items => [...new Set(items.filter(Boolean))];
  function keyboard(script) {
    const k = SCRIPT_CATALOG[script].keyboard;
    return Object.fromEntries(['vowels', 'consonants', 'vowelMarks', 'otherDiacritics']
      .map(group => [group, nativeKeyboardValues(script, group, k[group])]));
  }
  function withSign(script, stem, sign) {
    // Tamil writes the vowel sign (including pulli) before its allophone number.
    if (script === 'tamil' && /[¹²³⁴]$/.test(stem)) {
      return stem.slice(0, -1) + sign + stem.slice(-1);
    }
    return stem + sign;
  }
  function stems(script, group) {
    if (group === 'all') return unique(['vowels', 'consonants', 'signs', 'diacritics', 'conjuncts', 'conjunctSigns'].flatMap(g => stems(script, g)));
    const k = keyboard(script);
    const consonants = unique(k.consonants);
    const signs = unique(k.vowelMarks.slice(1));
    if (group === 'vowels') return unique(k.vowels);
    if (group === 'consonants') return consonants;
    if (group === 'signs') return unique(consonants.flatMap(c => signs.map(v => withSign(script, c, v))));
    if (group === 'diacritics') return unique([...unique(k.vowels), ...consonants]
      .flatMap(c => unique(k.otherDiacritics).map(m => c + m)));
    const virama = k.vowelMarks[0];
    const pairs = consonants.flatMap(a => consonants.map(b => withSign(script, a, virama) + b));
    if (group === 'conjuncts') return unique(pairs);
    if (group === 'conjunctSigns') return unique(pairs.flatMap(c => signs.map(v => withSign(script, c, v))));
    return [];
  }
  function rows(script, group, custom = []) {
    const k = keyboard(script);
    const consonants = unique(k.consonants), vowels = unique(k.vowels);
    const signs = unique(k.vowelMarks.slice(1));
    const result = [];
    const add = (label, values) => result.push({ label, stems: values });
    if (['vowels', 'all'].includes(group)) add('Vowels', vowels);
    if (['consonants', 'signs', 'all'].includes(group)) add('Consonants', consonants);
    if (['signs', 'all'].includes(group)) {
      for (const sign of signs) add('◌' + sign, consonants.map(c => withSign(script, c, sign)));
    }
    if (['diacritics', 'all'].includes(group)) {
      for (const mark of unique(k.otherDiacritics)) {
        add('Vowels + ◌' + mark, vowels.map(c => c + mark));
        add('Consonants + ◌' + mark, consonants.map(c => c + mark));
      }
    }
    if (['conjuncts', 'conjunctSigns', 'all'].includes(group)) {
      for (const first of consonants) {
        const prefix = withSign(script, first, k.vowelMarks[0]);
        add(prefix + '…', consonants.map(c => prefix + c));
        if (group !== 'conjuncts') {
          for (const sign of signs) add(prefix + '… + ◌' + sign, consonants.map(c => prefix + withSign(script, c, sign)));
        }
      }
    }
    if (['custom', 'all'].includes(group)) {
      const width = Math.max(vowels.length, consonants.length);
      for (let i = 0; i < custom.length; i += width) add('Custom', custom.slice(i, i + width));
    }
    // Keep a single review per exact stem, with empty slots preserving columns.
    const seen = new Set();
    return result.map(row => ({ ...row, stems: row.stems.map(stem => {
      if (seen.has(stem)) return null;
      seen.add(stem); return stem;
    }) })).filter(row => row.stems.some(Boolean));
  }
  function rectangle(cases, anchor, target) {
    const top = Math.min(anchor.row, target.row), bottom = Math.max(anchor.row, target.row);
    const left = Math.min(anchor.column, target.column), right = Math.max(anchor.column, target.column);
    return cases.filter(c => c.row >= top && c.row <= bottom && c.column >= left && c.column <= right).map(c => c.id);
  }
  function key(script, stem, accent) { return JSON.stringify([script, stem, accent]); }
  function codepoints(text) {
    return Array.from(text, c => 'U+' + c.codePointAt(0).toString(16).toUpperCase().padStart(4, '0'));
  }
  const verticalValues = ['collision', 'good', 'gap', 'unsure'];
  const horizontalValues = ['left', 'good', 'right', 'unsure'];
  function normalizeReview(review = {}) {
    const { status, ...rest } = review;
    return { ...rest,
      vertical: verticalValues.includes(review.vertical) ? review.vertical :
        verticalValues.includes(status) ? status : null,
      horizontal: horizontalValues.includes(review.horizontal) ? review.horizontal : null,
      note: typeof review.note === 'string' ? review.note : '' };
  }
  const isReviewed = r => Boolean(r && (r.vertical || r.horizontal || r.note?.trim()));
  const hasProblem = r => Boolean(r && (['collision', 'gap'].includes(r.vertical) || ['left', 'right'].includes(r.horizontal)));
  const isComplete = r => Boolean(r?.vertical && r?.horizontal && r.vertical !== 'unsure' && r.horizontal !== 'unsure');
  return { scripts, accents, stems, rows, rectangle, key, codepoints, normalizeReview, isReviewed, hasProblem, isComplete };

})();
