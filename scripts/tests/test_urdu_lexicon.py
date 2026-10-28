import importlib.util
import json
import pathlib
import subprocess
import struct
import sys
import tempfile
import unittest
import zlib

ROOT = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/urdu/build_lexicon.py"
spec = importlib.util.spec_from_file_location("urdu_lexicon", SCRIPT)
lexicon = importlib.util.module_from_spec(spec)
spec.loader.exec_module(lexicon)
TABLES = json.loads((ROOT / "scripts/script_data.json").read_text())


def source(entries):
    return {"format": 1, "version": "test", "provenance": "test data", "entries": entries}


class UrduLexiconTest(unittest.TestCase):
    def test_normalization_preserves_marks_and_alternatives(self):
        entry = {"word": "كِتاب", "readings": [[{"c": 0, "m": 2}], [{"c": 0, "m": 0}]]}
        result = lexicon.build(source([entry]), TABLES)
        self.assertEqual(result["entries"][0]["word"], "کِتاب")
        self.assertEqual(result["entries"][0]["readings"], entry["readings"])

    def test_conflicting_equivalent_spellings_are_rejected(self):
        entries = [{"word": w, "readings": [[{"v": 0}]]} for w in ("كتاب", "کتاب")]
        with self.assertRaisesRegex(ValueError, "duplicate normalized"):
            lexicon.build(source(entries), TABLES)

    def test_invalid_units_and_boundaries_are_rejected(self):
        for word, unit in [("کتاب!", {"c": 0}), ("کتاب", {"v": 0, "m": 1}),
                           ("کتاب", {"c": 999}), ("کتاب", {"c": True})]:
            with self.subTest(word=word, unit=unit), self.assertRaises(ValueError):
                lexicon.build(source([{"word": word, "readings": [[unit]]}]), TABLES)

    def test_artifact_is_reproducible(self):
        data = source([{"word": "کتاب", "readings": [[{"c": 0, "m": 2}]]}])
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory)
            (path / "input.json").write_text(json.dumps(data), encoding="utf-8")
            for name in ("a", "b"):
                subprocess.run([sys.executable, SCRIPT, path / "input.json", path / name], check=True)
            self.assertEqual((path / "a").read_bytes(), (path / "b").read_bytes())

    def test_binary_records_preserve_readings(self):
        data = source([{"word": "کتاب", "readings": [[{"c": 0, "m": 2}], [{"v": 1, "o": 0}]]}])
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory)
            (path / "input.json").write_text(json.dumps(data), encoding="utf-8")
            subprocess.run([sys.executable, SCRIPT, path / "input.json", path / "out", "--binary"], check=True)
            packed = (path / "out").read_bytes()
            self.assertEqual(packed[:4], b"IUL1")
            self.assertEqual(struct.unpack_from("<I", packed, 4)[0], zlib.crc32(packed[8:]))
            metadata_size, = struct.unpack_from("<I", packed, 8)
            metadata = json.loads(packed[12:12 + metadata_size])
            self.assertEqual(metadata["version"], "test")
            offset = 12 + metadata_size
            self.assertEqual(struct.unpack_from("<I", packed, offset)[0], 1)
            offset += 4
            word_size, count = struct.unpack_from("<HH", packed, offset)
            offset += 4
            self.assertEqual((packed[offset:offset + word_size].decode(), count), ("کتاب", 2))
            offset += word_size
            self.assertEqual(packed[offset:], b"\x01\x00\x01\x00\x02\xff\x01\x00\x00\x01\xff\x00")


if __name__ == "__main__":
    unittest.main()
