import importlib.util
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/urdu"))
spec = importlib.util.spec_from_file_location("evaluate_urdu", ROOT / "scripts/urdu/evaluate_lexicon.py")
evaluation = importlib.util.module_from_spec(spec)
spec.loader.exec_module(evaluation)


class UrduEvaluationTest(unittest.TestCase):
    def test_coverage_ambiguity_and_phoneme_errors(self):
        def entry(word, *readings):
            return {"word": word, "readings": list(readings)}

        kal = [{"c": 0}, {"c": 27, "m": 0}]
        kul = [{"c": 0, "m": 4}, {"c": 27, "m": 0}]
        candidate = {"entries": [entry("کل", kul), entry("نام", [{"c": 19}, {"c": 24, "m": 0}])]}
        reference = {"entries": [entry("کل", kal, kul), entry("نام", [{"c": 19, "m": 1}, {"c": 24, "m": 0}]),
                                 entry("کتاب", [{"c": 0, "m": 2}])]}
        result = evaluation.evaluate(candidate, reference)
        self.assertEqual((result["total"], result["covered"], result["ambiguous_reference"],
                          result["ambiguous_covered"], result["oov_or_unsupported"], result["exact"]),
                         (3, 2, 1, 1, 1, 1))
        self.assertEqual(result["phoneme_edits"], 1)
        self.assertEqual(result["errors"], ["نام"])



if __name__ == "__main__":
    unittest.main()
