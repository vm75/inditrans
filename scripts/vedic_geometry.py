"""Ink measurements for shaped Vedic clusters, in font design units.

Bounds detect excessive distance; sampled outline envelopes distinguish actual
collisions from disjoint ink in split vowels and upper diacritics. Curves are
flattened to half-unit accuracy and sampled every four units, well below the
clearance used by the generator. This is build-time geometry, not a renderer.
"""
import math
import unicodedata
from fontTools.pens.basePen import BasePen
from fontTools.pens.boundsPen import BoundsPen


class _OutlinePen(BasePen):
    def __init__(self, glyphs):
        super().__init__(glyphs)
        self.edges = []
        self.start = None

    def _moveTo(self, point):
        self.start = point

    def _lineTo(self, point):
        self.edges.append((self._getCurrentPoint(), point))

    def _closePath(self):
        self.edges.append((self._getCurrentPoint(), self.start))

    def _endPath(self):
        pass

    def _curveToOne(self, p1, p2, p3):
        def flatten(p0, p1, p2, p3, depth=0):
            dx, dy = p3[0] - p0[0], p3[1] - p0[1]
            length = dx * dx + dy * dy
            distances = [abs(dx * (p[1] - p0[1]) - dy * (p[0] - p0[0])) for p in (p1, p2)]
            projections = [dx * (p[0] - p0[0]) + dy * (p[1] - p0[1]) for p in (p1, p2)]
            if depth >= 12 or (length and max(distances) ** 2 <= .25 * length and min(projections) >= 0 and max(projections) <= length):
                self.edges.append((p0, p3)); return
            def mid(a, b): return ((a[0] + b[0]) / 2, (a[1] + b[1]) / 2)
            a, b, c = mid(p0, p1), mid(p1, p2), mid(p2, p3)
            d, e = mid(a, b), mid(b, c)
            f = mid(d, e)
            flatten(p0, a, d, f, depth + 1)
            flatten(f, e, c, p3, depth + 1)
        flatten(self._getCurrentPoint(), p1, p2, p3)

    def _qCurveToOne(self, control, end):
        start = self._getCurrentPoint()
        p1 = tuple(start[i] + 2 * (control[i] - start[i]) / 3 for i in (0, 1))
        p2 = tuple(end[i] + 2 * (control[i] - end[i]) / 3 for i in (0, 1))
        self._curveToOne(p1, p2, end)


class InkGeometry:
    STEP = 4

    def __init__(self, font, ignored=()):
        self.glyphs = font.getGlyphSet()
        self.ignored = set(ignored)
        self.cache = {}

    def outline(self, name):
        if name not in self.cache:
            bounds = BoundsPen(self.glyphs)
            self.glyphs[name].draw(bounds)
            pen = _OutlinePen(self.glyphs)
            self.glyphs[name].draw(pen)
            profile = {}
            for (x0, y0), (x1, y1) in pen.edges:
                if x0 == x1: continue
                lo, hi = sorted((x0, x1))
                for index in range(math.ceil((lo - 2) / self.STEP), math.floor((hi - 2) / self.STEP) + 1):
                    x = index * self.STEP + 2
                    y = y0 + (y1 - y0) * (x - x0) / (x1 - x0)
                    old = profile.get(index, (y, y))
                    profile[index] = (min(old[0], y), max(old[1], y))
            self.cache[name] = bounds.bounds, profile
        return self.cache[name]

    def gaps(self, glyphs, index, code, dx=0):
        name, rect = glyphs[index]
        if not rect: return None
        bounds, mark_profile = self.outline(name)
        mx, my = rect[0] - bounds[0] + dx, rect[1] - bounds[1]
        others = []
        # The last phonetic base can follow prefix spellings or another
        # syllable. Keep its whole shaping cluster (including reordered signs),
        # not unrelated earlier letters or spacing pronunciation annotations.
        start_cluster = 0
        if hasattr(glyphs, 'clusters'):
            letters = [i for i, c in enumerate(glyphs.text[:-1]) if unicodedata.category(c) == 'Lo' and c != 'ஃ']
            if letters:
                start_cluster = max((c for c in glyphs.clusters if c <= letters[-1]), default=0)
        for i, (other_name, other_rect) in enumerate(glyphs):
            if i == index or not other_rect or other_name in self.ignored: continue
            if hasattr(glyphs, 'clusters') and glyphs.clusters[i] < start_cluster: continue
            other_bounds, profile = self.outline(other_name)
            others.append((other_rect, profile, other_rect[0] - other_bounds[0], other_rect[1] - other_bounds[1]))
        if not others: return None
        above = code != 0x0952
        union = rect[1] - max(r[3] for r, _, _, _ in others) if above else min(r[1] for r, _, _, _ in others) - rect[3]
        gaps = []
        for column, (low, high) in mark_profile.items():
            x = column * self.STEP + 2 + mx
            for other_rect, profile, ox, oy in others:
                if not other_rect[0] <= x <= other_rect[2]: continue
                ink = profile.get(round((x - ox - 2) / self.STEP))
                if ink:
                    gaps.append(low + my - ink[1] - oy if above else ink[0] + oy - high - my)
        return union, min(gaps) if gaps else None
