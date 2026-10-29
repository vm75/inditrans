"""Single-wordlist compiler, script independence and isolated build coverage."""
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('consumer_lexicon', ROOT / 'scripts/build_lexicon.py')
compiler = importlib.util.module_from_spec(spec)
spec.loader.exec_module(compiler)
TABLES = json.loads((ROOT / 'scripts/script_data.json').read_text())
PROFILES = json.loads((ROOT / 'scripts/lexicon_profiles.json').read_text())


def compile_rows(script, source, strict=False):
    return compiler.compile_wordlist('wordlist.tsv', source.encode(),
                                    compiler.ScriptProfile(script, TABLES, PROFILES), strict=strict)


class LexiconCompilerTest(unittest.TestCase):
    def test_tamil_and_devanagari_share_the_same_compiler(self):
        for script, word in [('tamil', 'அகம்'), ('devanagari', 'अगम्')]:
            blob, report, entries = compile_rows(script, word + '\tə ɡ ə m\n', strict=True)
            self.assertEqual(report['script'], script)
            self.assertEqual(entries, [{'word': word, 'reading': [{'v': 0}, {'c': 2}, {'c': 24, 'm': 0}]}])
            self.assertEqual(blob[:4], b'IUL2')
            self.assertEqual(struct.unpack_from('<I', blob, 4)[0], zlib.crc32(blob[8:]))
            self.assertNotIn('license', report)
            self.assertNotIn('provenance', report)

    def test_duplicate_words_are_rejected_including_normalized_aliases(self):
        for text in ['پل\tp ə l\nپل\tp ʊ l\n',
                     'كتاب\tk ɪ t̪ ɑː b\nکتاب\tk ɪ t̪ ɑː b\n']:
            with self.assertRaisesRegex(ValueError, 'duplicate normalized word'):
                compile_rows('urdu', text)

    def test_script_validation_aliases_and_normalization(self):
        _, _, entries = compile_rows('urdu', 'كتاب\tk ɪ t̪ ɑː b\n')
        self.assertEqual(entries[0]['word'], 'کتاب')
        _, _, entries = compile_rows('tamil', 'கொ\tk o\n')
        self.assertEqual(entries[0]['word'], 'கொ')
        _, report, entries = compile_rows('tamil', 'کتاب\tk ɪ t̪ ɑː b\n')
        self.assertEqual(entries, [])
        self.assertEqual(report['rejected']['ineligible source spelling'], 1)
        for script in ['wx', 'indlish', 'imaginary']:
            with self.assertRaises(ValueError):
                compiler.ScriptProfile(script, TABLES, PROFILES)

    def test_unsupported_phone_fails_strictly_and_legacy_columns_are_rejected(self):
        with self.assertRaisesRegex(ValueError, 'unsupported IPA'):
            compile_rows('tamil', 'அகம்\tʕ\n', strict=True)
        with self.assertRaisesRegex(ValueError, 'exactly native word and IPA'):
            compile_rows('urdu', 'پل\tpal\tiast\t100\n')

    def test_compiler_runs_with_only_engine_data_and_one_wordlist(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ['scripts/build_lexicon.py', 'scripts/script_data.json', 'scripts/lexicon_profiles.json',
                         'scripts/python/lexicon_ipa.py', 'scripts/python/packed_lexicon.py']:
                target = root / name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(ROOT / name, target)
            (root / 'tamil.tsv').write_text('அகம்\tə ɡ ə m\n')
            command = [sys.executable, '-I', 'scripts/build_lexicon.py', 'tamil.tsv', '--script', 'tamil',
                       '--out', 'tamil.iul', '--report', 'tamil.json', '--strict']
            subprocess.run(command, cwd=root, check=True, capture_output=True)
            subprocess.run([*command, '--check'], cwd=root, check=True, capture_output=True)
            self.assertEqual(json.loads((root / 'tamil.json').read_text())['entries'], 1)

    def test_bundled_lexicon_hash_and_size(self):
        report = json.loads((ROOT / 'lexicons/urdu.json').read_text())
        data = (ROOT / 'lexicons/urdu.iul').read_bytes()
        self.assertEqual(compiler.digest(data), report['pack_sha256'])
        self.assertEqual(len(data), report['pack_bytes'])
        self.assertEqual(report['wordlist']['name'], 'wordlist.tsv')


if __name__ == '__main__':
    unittest.main()
