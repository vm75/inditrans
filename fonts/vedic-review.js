(() => {
  'use strict';
  const $ = id => document.getElementById(id);
  const storageKey = 'inditrans-vedic-review-v1';
  const { normalizeReview, isReviewed, hasProblem, isComplete } = VedicReview;
  let selected = null;
  const fontURL = new URL('inditrans-indic-fallback.woff2?rev=be0346d1', location.href).href;
  let state = { reviews: {}, custom: {} }, ready = false, fontHash = null;
  let corpusKey = '', corpus = [], displayedCases = [], patternRows = [];
  let selection = new Set(), anchorId = null, restoringView = true, suppressFocus = false, pointerSelecting = false;
  const caseIndexes = new Map();
  const corpusIndexes = new Map();
  const casesById = new Map();
  const cardsById = new Map();
  const headerObserver = new ResizeObserver(() => {
    document.documentElement.style.setProperty('--toolbar-height', document.querySelector('header').offsetHeight + 'px');
  });
  headerObserver.observe(document.querySelector('header'));
  try {
    const saved = JSON.parse(localStorage.getItem(storageKey));
    if (saved && saved.reviews && saved.custom) state = saved;
  } catch { $('status').textContent = 'Saved reviews could not be loaded. Export your reviews regularly.'; }
  for (const [id, review] of Object.entries(state.reviews)) state.reviews[id] = normalizeReview(review);
  function saveView() {
    state.view = Object.fromEntries(['script', 'group', 'accent', 'filter', 'size'].map(id => [id, $(id).value]));
    state.view.selectedId = selected?.id;
    const indices = [...selection].map(id => corpusIndexes.get(id)).filter(i => i != null).sort((a, b) => a - b);
    const ranges = [];
    for (const index of indices) {
      const last = ranges.at(-1);
      if (last && last[1] + 1 === index) last[1] = index;
      else ranges.push([index, index]);
    }
    state.view.selectionRanges = ranges;
    state.view.selectionLayout = 'native-patterns-v2';
    save();
  }
  function save() {
    try { localStorage.setItem(storageKey, JSON.stringify(state)); }
    catch { $('status').textContent = 'Local saving is unavailable. Export JSON before closing this page.'; }
  }
  for (const [id, script] of VedicReview.scripts) $('script').add(new Option(script.label, id));
  for (const accent of VedicReview.accents) $('accent').add(new Option(accent.label, accent.text));
  for (const id of ['script', 'group', 'accent', 'filter', 'size']) {
    const value = state.view?.[id];
    if (value != null && (id === 'size' ? Number(value) >= 20 && Number(value) <= 100 : [...$(id).options].some(o => o.value === value))) $(id).value = value;
  }
  document.documentElement.style.setProperty('--sample-size', $('size').value + 'px');
  function getCases() {
    const script = $('script').value, group = $('group').value;
    const nextKey = JSON.stringify([script, group, state.custom[script]]);
    if (nextKey !== corpusKey) {
      corpusKey = nextKey;
      const sourceRows = VedicReview.rows(script, group, state.custom[script] || []);
      patternRows = [];
      corpus = [];
      corpusIndexes.clear();
      for (const source of sourceRows) {
        for (const accent of VedicReview.accents) {
          const row = patternRows.length;
          const cells = source.stems.map((stem, column) => {
            if (!stem) return null;
            const existing = VedicReview.accents.filter(a => stem.includes(a.text));
            if (existing.length && existing[0].text !== accent.text) return null;
            const suffix = existing.length ? '' : accent.text;
            const text = stem + suffix;
            return { script, group, stem, text, accent: existing.length ? existing.map(a => a.text).join('') : accent.text,
              label: accent.label, row, column, id: VedicReview.key(script, stem, suffix) };
          });
          if (!cells.some(Boolean)) continue;
          patternRows.push({ label: source.label, accent: accent.label, cells });
          corpus.push(...cells.filter(Boolean));
        }
      }
    }
    if (corpusIndexes.size !== corpus.length || !corpusIndexes.has(corpus[0]?.id)) {
      corpusIndexes.clear(); corpus.forEach((c, i) => corpusIndexes.set(c.id, i));
    }
    return corpus.filter(c => {
      if ($('accent').value !== 'all' && !c.accent.includes($('accent').value)) return false;
      const review = state.reviews[c.id];
      return $('filter').value === 'all' ||
        ($('filter').value === 'problems' ? hasProblem(review) :
          $('filter').value === 'reviewed' ? isReviewed(review) :
          $('filter').value === 'incomplete' ? !isComplete(review) : !isReviewed(review));
    });
  }
  function record(c, change, persist = true) {
    state.reviews[c.id] = { ...normalizeReview(state.reviews[c.id]), ...c, ...change, codepoints: VedicReview.codepoints(c.text),
      fontSize: Number($('size').value), lineHeight: 2.2, fontSHA256: fontHash,
      browser: navigator.userAgent, devicePixelRatio: window.devicePixelRatio,
      reviewedAt: new Date().toISOString() };
    if (persist) save();
  }
  for (const button of document.querySelectorAll('[data-close]')) {
    button.onclick = () => button.closest('dialog').close();
  }
  $('open-about').onclick = () => $('about-modal').showModal();
  $('open-custom').onclick = () => $('custom-modal').showModal();
  $('open-editor').onclick = () => {
    if (!selected) return;
    $('editor').value = selected.text;
    $('editor').lang = SCRIPT_CATALOG[selected.script].lang;
    $('editor-modal').showModal();
    $('editor').focus();
  };
  function setSelection(ids) {
    for (const id of selection) if (!ids.has(id)) cardsById.get(id)?.setAttribute('data-selected', 'false');
    for (const id of ids) if (!selection.has(id)) cardsById.get(id)?.setAttribute('data-selected', 'true');
    selection = ids;
    $('selection-count').textContent = `${selection.size} selected`;
    saveView();
  }
  function selectCase(c, mode = 'single') {
    if (selected) cardsById.get(selected.id)?.setAttribute('data-active', 'false');
    selected = c;
    cardsById.get(c.id)?.setAttribute('data-active', 'true');
    $('open-editor').disabled = false;
    if (mode === 'range') {
      const anchor = casesById.get(anchorId) || c;
      setSelection(new Set(VedicReview.rectangle(displayedCases, anchor, c)));
    } else if (mode === 'toggle') {
      const next = new Set(selection);
      if (next.has(c.id)) next.delete(c.id); else next.add(c.id);
      anchorId = c.id; setSelection(next);
    } else if (mode === 'preserve') saveView();
    else { anchorId = c.id; setSelection(new Set([c.id])); }
  }
  function focusCase(index, mode = 'single') {
    if (!displayedCases.length) return;
    index = Math.max(0, Math.min(displayedCases.length - 1, index));
    const c = displayedCases[index], card = cardsById.get(c.id);
    suppressFocus = true;
    card?.focus({ preventScroll: true });
    suppressFocus = false;
    selectCase(c, mode);
    card?.scrollIntoView({ block: 'nearest', inline: 'nearest' });
  }
  document.addEventListener('pointerup', () => { pointerSelecting = false; });
  document.addEventListener('pointercancel', () => { pointerSelecting = false; });
  $('select-all').onclick = () => {
    setSelection(new Set(displayedCases.map(c => c.id)));
    if (selected) focusCase(caseIndexes.get(selected.id) || 0, 'preserve');
  };
  $('clear-selection').onclick = () => setSelection(new Set());
  function openReview() {
    if (!selected || !selection.size || !ready) return;
    $('review-text').textContent = selection.size === 1 ? casesById.get([...selection][0]).text : `${selection.size} selected`;
    $('review-text').lang = SCRIPT_CATALOG[selected.script].lang;
    updateDialog(); $('review-modal').showModal();
  }
  function updateCard(card, c) {
    const review = state.reviews[c.id];
    card.dataset.status = review?.vertical || 'unreviewed';
    card.dataset.horizontal = review?.horizontal || 'unreviewed';
    card.dataset.complete = String(isComplete(review));
    const vertical = { collision: 'too close', good: 'okay', gap: 'too far away', unsure: 'unsure' }[review?.vertical] || 'unreviewed';
    const horizontal = { left: 'too far left', good: 'okay', right: 'too far right', unsure: 'unsure' }[review?.horizontal] || 'unreviewed';
    card.setAttribute('aria-label', `${c.text}: vertical ${vertical}; horizontal ${horizontal}`);
  }
  function selectedCases() { return [...selection].map(id => casesById.get(id)).filter(Boolean); }
  function updateDialog() {
    const reviews = selectedCases().map(c => state.reviews[c.id]);
    for (const button of document.querySelectorAll('[data-review]')) {
      button.setAttribute('aria-pressed', String(reviews.length > 0 && reviews.every(r => r?.[button.dataset.axis] === button.dataset.review)));
    }
    const notes = reviews.map(r => r?.note || '');
    const sameNote = notes.every(note => note === notes[0]);
    $('review-note').value = sameNote ? notes[0] || '' : '';
    $('review-note').placeholder = sameNote ? '' : 'Mixed notes — typing replaces notes for selected cells';
    $('review-save-status').textContent = `${selection.size} selected · changes save automatically`;
  }
  function refreshAfterReview(index) {
    if ($('filter').value !== 'all') render();
    if (displayedCases.length) focusCase(Math.max(0, index), selection.size ? 'preserve' : 'single');
  }
  function rate(axis, value) {
    if (!ready || !selection.size) return;
    const index = caseIndexes.get(selected?.id) || 0;
    const targets = selectedCases();
    for (const c of targets) {
      record(c, { [axis]: value }, false);
      updateCard(cardsById.get(c.id), c);
    }
    save(); updateCount();
    $('status').textContent = `Rated ${targets.length} cells: ${axis} ${value}`;
    if ($('review-modal').open) updateDialog();
    else refreshAfterReview(index);
  }
  for (const button of document.querySelectorAll('[data-review]')) button.onclick = () => rate(button.dataset.axis, button.dataset.review);
  $('review-note').oninput = () => {
    if (!ready || !selection.size) return;
    for (const c of selectedCases()) record(c, { note: $('review-note').value }, false);
    save();
    $('review-save-status').textContent = `Note saved for ${selection.size} cells`;
    updateCount();
  };
  $('review-modal').onclose = () => {
    if (selected && !$('editor-modal').open && !$('review-modal').open) refreshAfterReview(caseIndexes.get(selected.id) || 0);
  };
  function keyboard(event) {
    if (event.isComposing || event.altKey ||
        event.target.matches('input, textarea, select') || event.target.isContentEditable) return;
    if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 'a' && !$('review-modal').open) {
      event.preventDefault(); $('select-all').click(); return;
    }
    if (event.ctrlKey || event.metaKey) return;
    if (event.key === 'Escape' && !$('review-modal').open) { event.preventDefault(); $('clear-selection').click(); return; }
    if (event.key === 'Enter' && !$('review-modal').open) { event.preventDefault(); openReview(); return; }
    const ratings = { '1': ['vertical', 'collision'], '2': ['vertical', 'good'], '3': ['vertical', 'gap'],
      '4': ['horizontal', 'left'], '5': ['horizontal', 'good'], '6': ['horizontal', 'right'] };
    if (ratings[event.key]) { event.preventDefault(); rate(...ratings[event.key]); return; }
    if (event.key.toLowerCase() === 'e') {
      event.preventDefault();
      if ($('review-modal').open) $('review-modal').close();
      $('open-editor').click(); return;
    }
    if ($('review-modal').open) return;
    const index = displayedCases.findIndex(c => c.id === selected?.id);
    const active = displayedCases[index];
    const rowIndices = [...new Set(displayedCases.map(c => c.row))];
    const rowIndex = rowIndices.indexOf(active?.row);
    const rowHeight = $('samples').querySelector('.pattern-row')?.getBoundingClientRect().height || 1;
    const screenRows = Math.max(1, Math.floor($('samples').clientHeight / rowHeight));
    function verticalTarget(delta) {
      const row = rowIndices[Math.max(0, Math.min(rowIndices.length - 1, rowIndex + delta))];
      const candidates = displayedCases.filter(c => c.row === row);
      return caseIndexes.get(candidates.reduce((best, c) => Math.abs(c.column - active.column) < Math.abs(best.column - active.column) ? c : best).id);
    }
    const moves = { ArrowLeft: index - 1, ArrowRight: index + 1,
      Home: 0, End: displayedCases.length - 1 };
    if (['ArrowUp', 'ArrowDown', 'PageUp', 'PageDown'].includes(event.key) && active) {
      moves[event.key] = verticalTarget(({ ArrowUp: -1, ArrowDown: 1, PageUp: -screenRows, PageDown: screenRows })[event.key]);
    }
    if (event.key in moves) { event.preventDefault(); focusCase(moves[event.key], event.shiftKey ? 'range' : 'single'); }
  }
  $('samples').onkeydown = keyboard;
  $('review-modal').onkeydown = keyboard;
  function updateCount() {
    const reviews = Object.values(state.reviews);
    $('count').textContent = `${reviews.filter(isReviewed).length} reviewed · ${reviews.filter(hasProblem).length} problems saved across all scripts`;
  }
  function render() {
    displayedCases = getCases();
    $('sample-count').textContent = `${displayedCases.length} samples in this selection`;
    selected = displayedCases.find(c => c.id === (selected?.id || state.view?.selectedId)) || displayedCases[0] || null;
    $('open-editor').disabled = !selected;
    const grid = $('samples');
    const fragment = document.createDocumentFragment();
    cardsById.clear(); casesById.clear(); caseIndexes.clear();
    displayedCases.forEach((c, i) => { casesById.set(c.id, c); caseIndexes.set(c.id, i); });
    if (restoringView && state.view?.selectionLayout === 'native-patterns-v2' && state.view.selectionRanges) {
      selection = new Set(state.view.selectionRanges.flatMap(([start, end]) =>
        corpus.slice(Math.max(0, start), Math.min(corpus.length, end + 1)).filter(c => casesById.has(c.id)).map(c => c.id)));
    } else selection = new Set([...selection].filter(id => casesById.has(id)));
    if (restoringView && state.view?.selectionLayout !== 'native-patterns-v2' && selected) selection.add(selected.id);
    restoringView = false;
    if (!anchorId || !casesById.has(anchorId)) anchorId = selected?.id;
    const visibleRows = new Map();
    for (const c of displayedCases) {
      if (!visibleRows.has(c.row)) visibleRows.set(c.row, []);
      visibleRows.get(c.row).push(c);
    }
    const columns = Math.max(1, ...patternRows.map(row => row.cells.length));
    grid.style.setProperty('--columns', columns);
    for (const [rowIndex, cells] of visibleRows) {
      const row = document.createElement('div'); row.className = 'pattern-row';
      const heading = document.createElement('button'); heading.className = 'row-heading';
      const pattern = document.createElement('span'); pattern.textContent = patternRows[rowIndex].label;
      const accent = document.createElement('small'); accent.textContent = patternRows[rowIndex].accent;
      heading.append(pattern, accent); heading.setAttribute('aria-label', 'Select row ' + pattern.textContent + ' ' + accent.textContent);
      heading.onclick = event => {
        selectCase(cells[0], 'preserve'); anchorId = cells[0].id;
        const ids = event.ctrlKey || event.metaKey ? new Set(selection) : new Set();
        for (const c of cells) ids.add(c.id);
        setSelection(ids); focusCase(caseIndexes.get(cells[0].id), 'preserve');
      };
      row.append(heading);
      const byColumn = new Map(cells.map(c => [c.column, c]));
      for (let column = 0; column < columns; column++) {
        const c = byColumn.get(column);
        if (!c) { const empty = document.createElement('span'); empty.className = 'empty-cell'; row.append(empty); continue; }
        const card = document.createElement('button'); card.className = 'card visual';
        card.type = 'button'; card.dataset.id = c.id; card.dataset.row = c.row; card.dataset.column = c.column;
        card.dataset.selected = String(selection.has(c.id)); card.dataset.active = String(c.id === selected?.id);
        card.onpointerdown = () => { pointerSelecting = true; };
        card.onfocus = () => { if (!suppressFocus && !pointerSelecting) selectCase(c); };
        updateCard(card, c);
        card.lang = SCRIPT_CATALOG[c.script].lang; card.textContent = c.text; card.disabled = !ready;
        card.onclick = event => {
          pointerSelecting = false;
          selectCase(c, event.shiftKey ? 'range' : event.ctrlKey || event.metaKey ? 'toggle' : 'single');
          if (!event.shiftKey && !event.ctrlKey && !event.metaKey) openReview();
        };
        cardsById.set(c.id, card); row.append(card);
      }
      fragment.append(row);
    }
    grid.replaceChildren(fragment);
    $('selection-count').textContent = `${selection.size} selected`;
    updateCount();
    saveView();
  }
  for (const id of ['script', 'group', 'accent', 'filter']) $(id).onchange = () => { selection.clear(); anchorId = null; render(); $('samples').scrollTo(0, 0); };
  $('size').oninput = () => {
    const size = Number($('size').value);
    if (size >= 20 && size <= 100) {
      document.documentElement.style.setProperty('--sample-size', size + 'px');
      saveView();
    }
  };
  function addCustom(text) {
    const script = $('script').value;
    state.custom[script] = [...new Set([...(state.custom[script] || []), ...text.split(/\r?\n/).map(s => s.trim()).filter(Boolean)])];
    $('group').value = 'custom'; selected = null; selection.clear(); render();
  }
  $('add').onclick = () => { addCustom($('custom').value); $('custom').value = ''; $('custom-modal').close(); };
  $('save-edit').onclick = () => {
    if (!$('editor').value.trim()) return;
    addCustom($('editor').value); $('editor-modal').close();
  };
  $('export').onclick = () => {
    const data = { schemaVersion: 2, exportedAt: new Date().toISOString(),
      page: location.href, browser: navigator.userAgent,
      font: { family: 'ReviewIndic', url: fontURL, sha256: fontHash },
      corpus: 'demo keyboard tables: vowels, consonants, vowel signs, diacritics, ordered consonant pairs; custom lines',
      custom: state.custom, reviews: Object.values(state.reviews).filter(isReviewed) };
    const url = URL.createObjectURL(new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' }));
    const link = document.createElement('a'); link.href = url; link.download = 'inditrans-vedic-review.json'; link.click();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
    $('status').textContent = `Exported ${data.reviews.length} reviews with independent vertical and horizontal ratings.`;
  };
  render();
  (async () => {
    try {
      const faces = await document.fonts.load('40px ReviewIndic', 'क॒');
      if (!faces.length) throw new Error('Font unavailable');
      ready = true; $('export').disabled = false;
      $('font-status').textContent = 'Font ready';
      for (const card of cardsById.values()) card.disabled = false;
      if (selected) cardsById.get(selected.id)?.scrollIntoView({ block: 'nearest' });
    } catch {
      $('font-status').textContent = 'Font failed to load. Serve the repository over HTTP and reload before reviewing. Reviews are disabled to avoid recording fallback-font results.';
      return;
    }
    // Fingerprinting is optional: a failed fetch or digest must not disable review.
    try {
      const response = await fetch(fontURL);
      if (!response.ok) throw new Error('Font fingerprint unavailable');
      const bytes = await response.arrayBuffer();
      if (globalThis.crypto?.subtle) {
        fontHash = Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256', bytes)), b => b.toString(16).padStart(2, '0')).join('');
      }
      $('font-status').textContent = 'Font ready' + (fontHash ? ' · ' + fontHash.slice(0, 8) : ' · fingerprint unavailable');
    } catch {
      $('font-status').textContent = 'Font ready · fingerprint unavailable';
    }
  })();
})();
