import importlib.util
import json
import pathlib
import struct
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/urdu"))
spec = importlib.util.spec_from_file_location("evaluate_runtime", ROOT / "scripts/urdu/evaluate_runtime.py")
evaluation = importlib.util.module_from_spec(spec)
spec.loader.exec_module(evaluation)


class FakeEngine:
    def literal(self, word, target):
        return {"اردو": "literal-wrong", "نام": "reference", "کل": "reference"}[word]

    def assisted(self, word, target, data):
        metadata_size, = struct.unpack_from("<I", data, 8)
        metadata = json.loads(data[12:12 + metadata_size])
        if metadata["version"] == "reference":
            return "reference"
        return {"اردو": "reference", "نام": "assisted-wrong", "کل": "reference"}[word]


class RuntimeEvaluationTest(unittest.TestCase):
    def test_reports_corrections_false_corrections_and_unrenderable_words(self):
        def entry(word):
            return {"word": word, "readings": [[{"v": 0}]]}

        candidate = {"format": 1, "version": "candidate", "provenance": "test",
                     "entries": [entry("اردو"), entry("نام")]}
        reference = {"format": 1, "version": "reference", "provenance": "test",
                     "entries": [entry("اردو"), entry("نام"), entry("کل"), entry("کِتاب")]}
        result = evaluation.evaluate(candidate, reference, "iast", FakeEngine())
        self.assertEqual((result["total"], result["eligible"], result["covered"], result["unrenderable"]),
                         (4, 3, 2, 1))
        self.assertEqual((result["baseline_exact"], result["assisted_exact"], result["corrected"],
                          result["false_corrections"]), (2, 2, 1, 1))
        self.assertEqual(result["coverage"], 2 / 3)
        self.assertEqual(result["false_correction_rate"], 0.5)
        self.assertEqual([item["word"] for item in result["errors"]], ["نام"])

    def test_rejects_same_script_evaluation(self):
        with self.assertRaises(ValueError):
            evaluation.evaluate({"entries": []}, {"entries": []}, "URDU", FakeEngine())

    def test_empty_reference_has_no_rates(self):
        candidate = {"format": 1, "version": "candidate", "provenance": "test", "entries": []}
        result = evaluation.evaluate(candidate, {"entries": []}, "iast", FakeEngine())
        self.assertIsNone(result["coverage"])
        self.assertIsNone(result["false_correction_rate"])


if __name__ == "__main__":
    unittest.main()
