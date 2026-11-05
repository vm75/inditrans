"""Font release archives contain only portable client assets and instructions."""

from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
import tempfile
import unittest
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[2]
SPEC = spec_from_file_location("package_fonts", ROOT / "scripts/package_fonts.py")
MODULE = module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class PackageFontsTest(unittest.TestCase):
    def test_client_archive_contents_and_reproducibility(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            archive = MODULE.package_fonts(ROOT, "v1.2.3", output)
            first = archive.read_bytes()
            with ZipFile(archive) as bundle:
                self.assertEqual(set(bundle.namelist()), {
                    "inditrans-indic-fallback.ttf", "inditrans-indic-fallback.woff2",
                    "OFL.txt", "README.md",
                })
                self.assertIsNone(bundle.testzip())
                for name in bundle.namelist():
                    if name != "README.md":
                        self.assertEqual(bundle.read(name), (ROOT / "fonts" / name).read_bytes())
                readme = bundle.read("README.md").decode("utf-8")
                self.assertIn("### Browser", readme)
                self.assertIn("### Flutter", readme)
                self.assertIn("v1.2.3", readme)
                self.assertNotIn("## Sources and generation", readme)
                self.assertNotIn("vedic-review.html", readme)
            self.assertEqual(MODULE.package_fonts(ROOT, "v1.2.3", output).read_bytes(), first)

    def test_invalid_tag_fails(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError, "release tag"):
                MODULE.package_fonts(ROOT, "../invalid", Path(directory))

    def test_missing_assets_fail(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaises(FileNotFoundError):
                MODULE.package_fonts(Path(directory), "v1.2.3", Path(directory) / "out")


if __name__ == "__main__":
    unittest.main()
