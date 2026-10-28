import importlib.util
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/urdu"))
spec = importlib.util.spec_from_file_location("import_wikipron", ROOT / "scripts/urdu/import_wikipron.py")
importer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(importer)


class WikiPronImportTest(unittest.TestCase):
    def test_combined_aspiration_and_nasal_vowels(self):
        self.assertEqual(importer.convert_phonemes("kʰ ə"), [{"c": 1}])
        self.assertEqual(importer.convert_phonemes("m ẽː"), [{"c": 24, "m": 11, "o": 0}])
        with self.assertRaises(ValueError):
            importer.convert_phonemes("bʰ ə")

    def test_conflicting_and_unsupported_variants_abstain(self):
        data = ("اردو\tʊ ɾ d̪ uː\n"
                "کل\tk ə l\nکل\tk ʊ l\n"
                "نام\tn ɑː m\nنام\tn unknown m\n"
                "کِتاب\tk ɪ t̪ ɑː b\n").encode()
        result, rejected = importer.convert(data)
        self.assertEqual([entry["word"] for entry in result["entries"]], ["اردو"])
        self.assertEqual(rejected["conflicting spellings"], 1)
        self.assertEqual(rejected["unsupported spelling"], 1)
        self.assertEqual(rejected["unsupported word"], 1)

    def test_malformed_record_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "line 1"):
            importer.convert("اردو\tʊ\textra\n".encode())

    def test_confirmation_requires_matching_single_reading(self):
        data = "اردو\tʊ ɾ d̪ uː\nکل\tk ə l\n".encode()
        confirmed = {"اردو": [[{"v": 4}, {"c": 26, "m": 0}, {"c": 17, "m": 5}]],
                     "کل": [[{"c": 0}, {"c": 27, "m": 0}], [{"c": 0, "m": 4}, {"c": 27, "m": 0}]]}
        result, rejected = importer.convert(data, confirmed)
        self.assertEqual([entry["word"] for entry in result["entries"]], ["اردو"])
        self.assertEqual(rejected["not confirmed"], 1)


if __name__ == "__main__":
    unittest.main()
