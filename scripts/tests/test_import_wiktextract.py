import importlib.util
import json
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/urdu"))
spec = importlib.util.spec_from_file_location("import_wiktextract", ROOT / "scripts/urdu/import_wiktextract.py")
importer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(importer)


class WiktextractImportTest(unittest.TestCase):
    def test_ipa_boundaries_and_unknowns(self):
        self.assertEqual(importer.convert_ipa("/ʊɾ.d̪uː/"),
                         [{"v": 4}, {"c": 26, "m": 0}, {"c": 17, "m": 5}])
        self.assertEqual(importer.convert_ipa("/kʰə/"), [{"c": 1}])
        self.assertEqual(importer.convert_ipa("/mẽː/"), [{"c": 24, "m": 11, "o": 0}])
        with self.assertRaises(ValueError):
            importer.convert_ipa("/kəɬ/")
        with self.assertRaises(ValueError):
            importer.convert_ipa("/k(ə)l/")
        with self.assertRaises(ValueError):
            importer.convert_ipa("/kəl/kʊl/")

    def test_ambiguity_and_unsupported_variants_abstain(self):
        def row(word, ipa, tags=None):
            return {"lang_code": "ur", "word": word,
                    "sounds": [{"ipa": ipa, "tags": tags or ["Standard", "Urdu"]}]}
        rows = [row("اردو", "/ʊɾ.d̪uː/"), row("کل", "/kəl/"), row("کل", "/kʊl/"),
                row("نام", "/nɑːm/"), row("نام", "/nɑːɬ/"), row("سلام", "/səlɑːm/"),
                {"lang_code": "ur", "word": "سلام", "sounds": []}]
        data = "\n".join(json.dumps(x, ensure_ascii=False) for x in rows).encode()
        output, rejected = importer.convert(data)
        self.assertEqual([entry["word"] for entry in output["entries"]], ["اردو"])
        self.assertEqual(rejected["conflicting spellings"], 1)
        self.assertEqual(rejected["unsupported spelling"], 2)

    def test_dialect_note_prevents_selecting_another_reading(self):
        row = {"lang_code": "ur", "word": "اردو", "sounds": [
            {"ipa": "/ʊɾ.d̪uː/", "tags": ["Standard", "Urdu"]},
            {"ipa": "/əɾ.d̪uː/", "note": "Punjabic Urdu"}]}
        output, rejected = importer.convert(json.dumps(row, ensure_ascii=False).encode())
        self.assertEqual(output["entries"], [])
        self.assertEqual(rejected["unsupported spelling"], 1)


if __name__ == "__main__":
    unittest.main()
