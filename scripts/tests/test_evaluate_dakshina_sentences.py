import importlib.util
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/urdu"))
spec = importlib.util.spec_from_file_location("evaluate_dakshina_sentences", ROOT / "scripts/urdu/evaluate_dakshina_sentences.py")
evaluation = importlib.util.module_from_spec(spec)
spec.loader.exec_module(evaluation)


class FakeEngine:
    def literal(self, text, target):
        return {"اردو کتاب": "urdu ktāb", "اردو کل": "urdu kal", "سلام": "salām", "کتِاب": "ktāb"}[text]

    def assisted(self, text, target, data):
        return {"اردو کتاب": "urdu kitāb", "اردو کل": "urdu kul", "سلام": "salām", "کتِاب": "ktāb"}[text]


class DakshinaSentenceTest(unittest.TestCase):
    def test_agreement_and_unsupported_spans(self):
        rows = evaluation.references(("اردو کتاب\turdu kitaab\nاردو کل\turdu kal\n"
                                      "کتِاب\tkitaab\nسلام\tsalaam\n").encode())
        candidate = {"format": 1, "version": "test", "provenance": "test",
                     "entries": [{"word": "اردو", "readings": [[{"v": 4}]]}]}
        result = evaluation.evaluate(candidate, rows, FakeEngine())
        self.assertEqual((result["sentences"], result["scored"], result["unsupported_urdu"],
                          result["scored_with_unsupported_urdu"]), (4, 4, 1, 1))
        self.assertEqual((result["baseline_agrees"], result["assisted_agrees"],
                          result["gained_agreement"], result["lost_agreement"]), (2, 2, 1, 1))
        self.assertEqual(result["unexpected_change_without_hit"], 0)

    def test_preserves_word_boundaries_and_rejects_non_latin(self):
        self.assertNotEqual(evaluation.fold_sentence("ab c"), evaluation.fold_sentence("a bc"))
        self.assertEqual(evaluation.fold_sentence("kitāb ۱۲"), "kitab 12")
        self.assertIsNone(evaluation.fold_sentence("urdu کتاب"))
        with self.assertRaises(ValueError):
            evaluation.references("one\ttwo\tthree\n".encode())


if __name__ == "__main__":
    unittest.main()
