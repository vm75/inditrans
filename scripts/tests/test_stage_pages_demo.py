"""Check root and PR preview bundles use their own matching engine assets."""

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from importlib.util import module_from_spec, spec_from_file_location

MODULE_PATH = Path(__file__).resolve().parents[1] / "stage_pages_demo.py"
SPEC = spec_from_file_location("stage_pages_demo", MODULE_PATH)
assert SPEC is not None and SPEC.loader is not None
MODULE = module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class StagePagesDemoTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tempdir = tempfile.TemporaryDirectory()
        self.addCleanup(self.tempdir.cleanup)
        self.root = Path(self.tempdir.name)

    def checkout(self, name: str, version: str, engine: str) -> Path:
        root = self.root / name
        (root / "flutter").mkdir(parents=True)
        (root / "flutter/pubspec.yaml").write_text(
            f"name: inditrans\nversion: {version}\n", encoding="utf-8"
        )
        (root / "demo").mkdir(parents=True)
        (root / "fonts").mkdir()
        (root / "demo/index.html").write_text(
            '<!doctype html><meta name="inditrans-version" content="development">'
            '<script src="../js/public/inditrans.js"></script>', encoding="utf-8"
        )
        for name in ["inditrans-indic-fallback.woff2", "inditrans-indic-fallback.ttf", "OFL.txt"]:
            (root / "fonts" / name).write_bytes(engine.encode("utf-8"))
        (root / "js/public").mkdir(parents=True)
        (root / "js/public/inditrans.js").write_text(engine, encoding="utf-8")
        (root / "js/inditrans-standalone.js").write_text(engine, encoding="utf-8")
        return root

    def test_independent_production_and_pr_bundles(self) -> None:
        main = self.checkout("main", "0.13.0", "main-engine")
        branch = self.checkout("branch", "0.14.0", "pr-engine")
        live = self.root / "site"
        preview = live / "previews/pr-73"
        self.assertEqual(MODULE.stage_demo(main, live), "0.13.0")
        self.assertEqual(MODULE.stage_demo(branch, preview), "0.14.0")
        self.assertIn('content="v0.13.0"', (live / "demo/index.html").read_text())
        self.assertIn('content="v0.14.0"', (preview / "demo/index.html").read_text())
        for path in ["js/public/inditrans.js", "js/inditrans-standalone.js"]:
            self.assertEqual((live / path).read_text(), "main-engine")
            self.assertEqual((preview / path).read_text(), "pr-engine")
        for name in ["inditrans-indic-fallback.woff2", "inditrans-indic-fallback.ttf", "OFL.txt"]:
            self.assertEqual((live / "fonts" / name).read_bytes(), b"main-engine")
            self.assertEqual((preview / "fonts" / name).read_bytes(), b"pr-engine")
        self.assertIn('url=demo/', (preview / "index.html").read_text())

    def test_legacy_checkout_keeps_demo_fonts(self) -> None:
        source = self.checkout("legacy", "0.13.0", "legacy-engine")
        (source / "fonts").rename(source / "demo/fonts")
        destination = self.root / "site"
        MODULE.stage_demo(source, destination)
        self.assertEqual(
            (destination / "demo/fonts/inditrans-indic-fallback.woff2").read_bytes(),
            b"legacy-engine",
        )

    def test_invalid_version_fails(self) -> None:
        source = self.checkout("bad", "0.13.0", "engine")
        (source / "flutter/pubspec.yaml").write_text("version: invalid\n")
        with self.assertRaisesRegex(ValueError, "Could not read package version"):
            MODULE.stage_demo(source, self.root / "site")

    def test_missing_meta_tag_fails(self) -> None:
        source = self.checkout("bad-meta", "0.13.0", "engine")
        (source / "demo/index.html").write_text("<!doctype html>")
        with self.assertRaisesRegex(ValueError, "Expected one inditrans-version meta tag"):
            MODULE.stage_demo(source, self.root / "site")


if __name__ == "__main__":
    unittest.main()
