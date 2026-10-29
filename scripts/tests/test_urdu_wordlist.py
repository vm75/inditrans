"""Source-neutral wordlist compiler tests: preferences, ambiguity, size and selection."""
import pathlib
import sys
import struct
import unittest
import zlib

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/urdu"))
import build_wordlist
from build_lexicon import binary_pack


def compile_sources(*sources, selection=None, limit=None, strict=False):
    return build_wordlist.compile_wordlist(
        [(f"test-{i}", text.encode("utf-8")) for i, text in enumerate(sources)],
        version="test-1", provenance="project-authored test fixtures",
        license_name="CC0 synthetic fixtures", selection=selection, limit=limit, strict=strict)


class WordlistTest(unittest.TestCase):
    def test_multiple_readings_and_editorial_default(self):
        raw = "پل\tpul\tiast\t2\nپل\tpal\tiast\t10\nقلم\tqalam\tiast\t4\n"
        packed, report, data = compile_sources(raw)
        self.assertEqual(report["entries"], 2)
        self.assertEqual(report["ambiguities_with_alternatives"], 1)
        self.assertEqual(len(next(x for x in data["entries"] if x["word"] == "پل")["readings"]), 2)
        self.assertEqual(packed[:4], b"IUL1")
        self.assertEqual(struct.unpack_from("<I", packed, 4)[0], zlib.crc32(packed[8:]))
        # Winning pal = p + a + l (explicitly virama-terminated).
        reading = next(x for x in data["entries"] if x["word"] == "پل")["readings"][0]
        self.assertEqual(reading, [{"c": 20}, {"c": 27, "m": 0}])

    def test_input_order_never_changes_output(self):
        x = "پل\tpal\tiast\t10\nپل\tpul\tiast\t10\n"
        y = "پل\tpul\tiast\t10\nپل\tpal\tiast\t10\n"
        a, ar, _ = compile_sources(x)
        b, br, _ = compile_sources(y)
        # The canonical source SHA-256 describes sorted, normalized entries;
        # merely reordering TSV rows must not change any packed bytes.
        self.assertEqual(a, b)
        self.assertEqual(
            compile_sources(x)[2]["entries"],
            compile_sources(y)[2]["entries"])
        self.assertEqual(ar["entries"], br["entries"])

    def test_ipa_and_iast_converge(self):
        _, report, data = compile_sources(
            "پل\tp ə l\tipa\t1\nپل\tpal\tiast\t3\n")
        self.assertEqual(report["ambiguities_with_alternatives"], 0)
        self.assertEqual(len(data["entries"]), 1)

    def test_selection_and_limit_do_not_require_context(self):
        wordlist = "پل\tpal\tiast\t0\nقلم\tqalam\tiast\t9\nکتاب\tkitāb\tiast\t1\n"
        _, report, data = compile_sources(wordlist, selection="پل\nکتاب\n".encode())
        self.assertEqual(report["entries"], 2)
        self.assertEqual({e["word"] for e in data["entries"]}, {"پل", "کتاب"})
        _, report, data = compile_sources(wordlist, limit=1)
        self.assertEqual([e["word"] for e in data["entries"]], ["قلم"])
        with self.assertRaisesRegex(ValueError, "no valid reading"):
            compile_sources(wordlist, selection="غائب\n".encode())

    def test_skips_unsupported_or_fails_strictly(self):
        text = "پل\tpal\tiast\t2\nکسی\tunsupported?\tiast\t0\n"
        _, report, _ = compile_sources(text)
        self.assertEqual(report["entries"], 1)
        self.assertEqual(report["rejected"]["unsupported pronunciation"], 1)
        with self.assertRaisesRegex(ValueError, "unsupported IAST"):
            compile_sources(text, strict=True)

    def test_pack_has_no_arbitrary_three_megabyte_limit(self):
        reading = [{"c": 20, "m": 0}] * 100
        entries = [{"word": "پل" + format(i, "05d"), "readings": [reading]}
                   for i in range(9000)]
        # The packer enforces uint16 per-record limits but has no 3 MB cap.
        packed = binary_pack({
            "version": "large-test", "provenance": "synthetic",
            "source_sha256": "0" * 64, "entries": entries,
        })
        self.assertGreater(len(packed), 3_000_000)
        self.assertEqual(packed[:4], b"IUL1")


if __name__ == "__main__":
    unittest.main()
