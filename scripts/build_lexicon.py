#!/usr/bin/env python3
"""Compile one native-script→IPA TSV into a deterministic packed lexicon.

Each normalized native word must occur once. The compiler consumes only that
wordlist and the engine's script/phoneme inventory.
"""
import argparse
import collections
import csv
import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import unicodedata
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parent / 'python'))
from lexicon_ipa import IpaReader
from packed_lexicon import pack as pack_dictionary

ROOT = Path(__file__).resolve().parents[1]


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


class ScriptProfile:
    def __init__(self, name, tables, profiles):
        selected = None
        for family in tables.values():
            for script_id, data in family.items():
                if name.lower() in [script_id, *data.get('aliases', ())]:
                    selected = script_id, data
                    break
            if selected:
                break
        if selected is None:
            raise ValueError('unknown concrete source script: ' + name)
        self.name, self.table = selected
        self.settings = profiles.get(self.name, {})
        if self.settings.get('disabled'):
            raise ValueError('script cannot be used as a source: ' + self.name)
        self.aliases = str.maketrans(self.settings.get('aliases', {}))
        self.characters = set()
        for field in ['vowels', 'vowelMarks', 'consonants', 'otherDiacritics', 'vedicSymbols']:
            for value in self.table.get(field, ()):
                self.characters.update(unicodedata.normalize('NFD', value))
                self.characters.update(unicodedata.normalize('NFC', value))
        for alternatives in self.table.get('equivalents', {}).values():
            for value in alternatives:
                self.characters.update(unicodedata.normalize('NFD', value))
                self.characters.update(unicodedata.normalize('NFC', value))
        self.ipa = IpaReader(tables['roman']['ipa'], self.settings.get('ipaAliases'))

    def normalize(self, word):
        return unicodedata.normalize('NFC', word.strip().translate(self.aliases))

    def eligible(self, word):
        if not word or unicodedata.category(word[0])[0] != 'L':
            return False
        ranges = self.settings.get('wordRanges')
        exclusions = self.settings.get('wordExclusions', ())
        for char in word:
            cp = ord(char)
            if unicodedata.category(char)[0] not in 'LM':
                return False
            if any(low <= cp <= high for low, high in exclusions):
                return False
            if ranges:
                if not any(low <= cp <= high for low, high in ranges):
                    return False
            elif char not in self.characters:
                return False
        return True


def encode_entries(entries, metadata):
    """IUL2 envelope and opaque PLX1 values using canonical 16-bit token units."""
    records = []
    for entry in entries:
        reading = entry['reading']
        if not 0 < len(reading) <= 65535:
            raise ValueError('reading exceeds 16-bit unit count')
        payload = bytearray(struct.pack('<HH', 1, len(reading)))
        for unit in reading:
            lead = unit['c'] + 22 if 'c' in unit else unit['v']
            mark, other = unit.get('m', 22), unit.get('o', 8)
            if not 0 <= lead < 86 or not 0 <= mark <= 22 or not 0 <= other <= 8 or (lead < 22 and mark != 22):
                raise ValueError('reading exceeds canonical token inventory')
            payload.extend(struct.pack('<H', lead | (mark << 7) | (other << 12)))
        records.append((entry['word'], payload))
    raw_metadata = json.dumps(metadata, ensure_ascii=False, sort_keys=True, separators=(',', ':')).encode()
    body = struct.pack('<I', len(raw_metadata)) + raw_metadata + pack_dictionary(records)
    return b'IUL2' + struct.pack('<I', zlib.crc32(body)) + body


def compile_wordlist(name, raw, profile, *, strict=False):
    seen = set()
    entries = []
    rejected = collections.Counter()
    skipped = []
    rows = csv.reader(io.StringIO(raw.decode('utf-8-sig')), delimiter='\t')
    for line, fields in enumerate(rows, 1):
        if not fields or not fields[0].strip() or fields[0].lstrip().startswith('#'):
            continue
        if len(fields) != 2:
            raise ValueError(f'{name}:{line}: expected exactly native word and IPA columns')
        word, ipa = profile.normalize(fields[0]), fields[1].strip()
        if word in seen:
            raise ValueError(f'{name}:{line}: duplicate normalized word: {word!r}')
        seen.add(word)
        if not profile.eligible(word):
            reason = 'ineligible source spelling'
        else:
            try:
                reading = profile.ipa.parse(ipa)
            except ValueError as error:
                reason = str(error)
            else:
                entries.append({'word': word, 'reading': reading})
                continue
        rejected[reason] += 1
        if reason != 'ineligible source spelling':
            skipped.append({'word': word, 'line': line, 'reason': reason})
        if strict:
            raise ValueError(f'{name}:{line}: {reason}: {word!r}')
    entries.sort(key=lambda entry: entry['word'])
    wordlist = {'name': name, 'sha256': digest(raw)}
    metadata = {'format': 2, 'script': profile.name, 'wordlist': wordlist}
    blob = encode_entries(entries, metadata)
    report = {'format': 2, 'script': profile.name, 'binary_format': 'IUL2',
              'entries': len(entries), 'pack_bytes': len(blob), 'pack_sha256': digest(blob),
              'wordlist': wordlist, 'source_words': len(seen),
              'rejected': dict(sorted(rejected.items())), 'skipped': skipped}
    return blob, report, entries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('wordlist', type=Path, help='one native→IPA TSV for the selected script')
    parser.add_argument('--script', required=True, help='concrete source script or alias')
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True, help='technical build information')
    parser.add_argument('--strict', action='store_true', help='fail on any unrepresentable entry')
    parser.add_argument('--check', action='store_true', help='check outputs without writing')
    args = parser.parse_args()
    tables = json.loads((ROOT / 'scripts/script_data.json').read_text())
    profiles = json.loads((ROOT / 'scripts/lexicon_profiles.json').read_text())
    try:
        profile = ScriptProfile(args.script, tables, profiles)
        blob, report, _ = compile_wordlist(args.wordlist.name, args.wordlist.read_bytes(), profile, strict=args.strict)
    except ValueError as error:
        parser.error(str(error))
    output = json.dumps(report, ensure_ascii=False, sort_keys=True, indent=2) + '\n'
    if args.check:
        if not args.out.exists() or not args.report.exists() or args.out.read_bytes() != blob or args.report.read_text() != output:
            parser.error('lexicon/report are stale; rebuild from the wordlist')
    else:
        for path in [args.out, args.report]:
            path.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_bytes(blob)
        args.report.write_text(output, encoding='utf-8')
    print(f'{profile.name}: {report["entries"]} entries, {len(blob)} packed bytes')


if __name__ == '__main__':
    main()
