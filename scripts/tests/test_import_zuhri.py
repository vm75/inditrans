import importlib.util
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/urdu"))
spec = importlib.util.spec_from_file_location("import_zuhri", ROOT / "scripts/urdu/import_zuhri.py")
importer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(importer)


class ZuhriImportTest(unittest.TestCase):
    def test_canonical_units_and_aspirates(self):
        self.assertEqual(importer.units("ʊ r d uː"), [{"v": 4}, {"c": 26, "m": 0}, {"c": 17, "m": 5}])
        self.assertEqual(importer.units("k ʰ ə"), [{"c": 1}])
        self.assertEqual(importer.units("kʰ ə"), [{"c": 1}])
        self.assertEqual(importer.units("m ẽː"), [{"c": 24, "m": 11, "o": 0}])
        with self.assertRaises(ValueError):
            importer.units("b ʰ ə")
        with self.assertRaises(ValueError):
            importer.units("k unknown ə")

    def test_conflicts_unknowns_and_marks_abstain(self):
        self.assertFalse(importer.eligible_word("ٴب"))
        csv = ("urdu_text,phonemes,is_valid\n"
               "اردو,ʊ r d uː,True\n"
               "اردو,ʊ r d uː,True\n"
               "کل,k ə l,True\n"
               "کل,k ʊ l,True\n"
               "نام,n aː m,True\n"
               "نام,n mystery m,True\n"
               "کِتاب,k ɪ t aː b,True\n").encode()
        output, rejected = importer.convert(csv)
        self.assertEqual([entry["word"] for entry in output["entries"]], ["اردو"])
        self.assertEqual(rejected["conflicting spellings"], 1)
        self.assertEqual(rejected["unsupported spelling"], 1)
        self.assertEqual(rejected["invalid"], 1)


if __name__ == "__main__":
    unittest.main()
