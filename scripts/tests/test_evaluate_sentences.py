import importlib.util
import json
import pathlib
import struct
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/urdu"))
spec = importlib.util.spec_from_file_location("evaluate_sentences", ROOT / "scripts/urdu/evaluate_sentences.py")
evaluation = importlib.util.module_from_spec(spec)
spec.loader.exec_module(evaluation)
TABLES = json.loads((ROOT / "scripts/script_data.json").read_text(encoding="utf-8"))


class FakeEngine:
    def literal(self, text, target):
        return "literal"

    def assisted(self, text, target, data):
        size, = struct.unpack_from("<I", data, 8)
        metadata = json.loads(data[12:12 + size])
        return "reference" if metadata["version"] == "reference" else "candidate"


class SentenceEvaluationTest(unittest.TestCase):
    def test_segmentation_respects_markup_and_mixed_words(self):
        self.assertEqual(evaluation.eligible_words("<x>اردو</x>، ##کتاب## كتاب xاردو اردوx"),
                         ["اردو", "کتاب"])
        self.assertEqual(evaluation.eligible_words("اردو ##کتاب"), ["اردو"])
        words, unsupported = evaluation.scan_words("اردو کتابِ xاردو")
        self.assertEqual(words, ["اردو"])
        self.assertEqual(unsupported, ["کتابِ", "xاردو"])

    def test_requires_complete_unambiguous_context_annotations(self):
        entry = {"word": "اردو", "readings": [[{"v": 4}]]}
        candidate = {"format": 1, "version": "candidate", "provenance": "test", "entries": [entry]}
        reference = {"format": 1, "version": "reference", "provenance": "test",
                     "sentences": [{"text": "اردو ##کتاب##", "readings": [entry]}]}
        result = evaluation.evaluate(candidate, reference, "iast", FakeEngine(), TABLES)
        self.assertEqual((result["sentences"], result["eligible_words"], result["covered_words"]), (1, 1, 1))
        self.assertEqual(result["errors"][0]["expected"], "reference")
        reference["sentences"][0]["text"] = "اردو کتابِ"
        result = evaluation.evaluate(candidate, reference, "iast", FakeEngine(), TABLES)
        self.assertEqual((result["sentences"], result["scored_sentences"], result["unsupported_sentences"]), (1, 0, 1))
        reference["sentences"][0]["text"] = "اردو کتاب"
        with self.assertRaisesRegex(ValueError, "annotate exactly"):
            evaluation.evaluate(candidate, reference, "iast", FakeEngine(), TABLES)
        reference["sentences"][0]["text"] = "اردو"
        reference["sentences"][0]["readings"] = [{"word": "اردو", "readings": [[{"v": 4}], [{"v": 5}]]}]
        with self.assertRaisesRegex(ValueError, "exactly one"):
            evaluation.evaluate(candidate, reference, "iast", FakeEngine(), TABLES)


if __name__ == "__main__":
    unittest.main()
