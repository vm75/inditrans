"""Synthetic-source and checked-in package tests for the Urdu corpus builder."""
import importlib.util
import json
import pathlib
import struct
import sys
import unittest
import zlib
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/urdu"))
import build_concordant
import build_lexicon
def demo():
    return json.loads((ROOT / "scripts/urdu/demo-lexicon.json").read_text(encoding="utf-8"))

class ConcordantTest(unittest.TestCase):
    def test_reject_unpinned_sources(self):
        with self.assertRaisesRegex(ValueError, "Source SHA-256 mismatch"):
            build_concordant.compile_lexicon(b"", b"", demo())

    def test_concordance_preserves_demo_word_and_crc(self):
        # Both transforms agree on a single representative IPA sequence.
        wiki = "سلام\ts a l aː m\n".encode()
        wikt = ('{"lang_code":"ur","word":"سلام","sounds":[{"ipa":"/salaːm/"}]}\n').encode()
        packed, report, canonical = build_concordant.compile_lexicon(
            wiki, wikt, demo(), verify_sources=False)
        self.assertTrue(packed.startswith(b"IUL1"))
        self.assertEqual(struct.unpack_from("<I", packed, 4)[0], zlib.crc32(packed[8:]))
        self.assertLessEqual(len(packed), 3_000_000)
        self.assertIn("جیسی", {e["word"] for e in canonical["entries"]})
        self.assertEqual(report["packed_entries"], len(canonical["entries"]))
        self.assertEqual(report["pack_bytes"], len(packed))
        self.assertEqual(packed, build_concordant.compile_lexicon(
            wiki, wikt, demo(), verify_sources=False)[0])

    def test_checked_in_asset_manifest_if_present(self):
        pack_path = ROOT / "demo/urdu-pronunciation.iul"
        report_path = ROOT / "scripts/urdu/concordant-report.json"
        if not pack_path.exists() and not report_path.exists():
            self.skipTest("data build not yet checked in")
        self.assertTrue(pack_path.is_file() and report_path.is_file())
        blob = pack_path.read_bytes()
        report = json.loads(report_path.read_text(encoding="utf-8"))
        self.assertEqual(len(blob), report["pack_bytes"])
        self.assertEqual(build_concordant.sha(blob), report["pack_sha256"])
        self.assertLessEqual(len(blob), build_concordant.MAX_PACK_BYTES)
        self.assertGreaterEqual(report["concordant_entries"], build_concordant.MIN_CONCORDANT)
        self.assertGreaterEqual(report["packed_entries"], report["concordant_entries"])
        self.assertEqual(blob[:4], b"IUL1")
        self.assertEqual(struct.unpack_from("<I", blob, 4)[0], zlib.crc32(blob[8:]))

if __name__ == "__main__":
    unittest.main()
