import importlib.util
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/urdu"))
spec = importlib.util.spec_from_file_location("evaluate_dakshina", ROOT / "scripts/urdu/evaluate_dakshina.py")
evaluation = importlib.util.module_from_spec(spec)
spec.loader.exec_module(evaluation)


class FakeEngine:
    def literal(self, word, target):
        return {"اردو": "rdū", "کتاب": "kitāb", "کل": "kl"}[word]

    def assisted(self, word, target, data):
        return {"اردو": "urdū", "کتاب": "katāb", "کل": "kl"}[word]


class DakshinaEvaluationTest(unittest.TestCase):
    def test_multiple_human_spellings_and_agreement_changes(self):
        gold = evaluation.references("اردو\turdū\t1\nاردو\turdoo\t2\nکتاب\tkitaab\t3\nکل\tkal\t1\n".encode())
        candidate = {"format": 1, "version": "test", "provenance": "test",
                     "entries": [{"word": "اردو", "readings": [[{"v": 4}]]},
                                 {"word": "کتاب", "readings": [[{"c": 0}]]}]}
        result = evaluation.evaluate(candidate, gold, FakeEngine())
        self.assertEqual((result["words"], result["eligible"], result["covered"]), (3, 3, 2))
        self.assertEqual((result["baseline_agrees"], result["assisted_agrees"],
                          result["gained_agreement"], result["lost_agreement"]), (1, 1, 1, 1))
        self.assertEqual((result["covered_multiple_spellings"], result["covered_baseline_agrees"],
                          result["covered_assisted_agrees"]), (1, 1, 1))

    def test_malformed_and_nonpositive_counts_are_rejected(self):
        for value in ("اردو\turdū\t0\n", "اردو\turdū\n"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                evaluation.references(value.encode())


if __name__ == "__main__":
    unittest.main()
