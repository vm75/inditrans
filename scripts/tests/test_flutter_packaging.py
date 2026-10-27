"""Regressions for the self-contained pub.dev native source bundle (#64)."""

from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from prepare_flutter_package import prepare_package


class FlutterPackagingTest(unittest.TestCase):
    def test_bundle_is_complete_and_refresh_removes_stale_sources(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "native" / "src"
            source.mkdir(parents=True)
            (root / "flutter").mkdir()
            (root / "CHANGELOG.md").write_text("Release notes\n")
            for name in ("CMakeLists.txt", "inditrans.cpp", "script_data.h"):
                (source / name).write_text(name)
            (source / "build").mkdir()
            (source / "build" / "artifact.o").write_bytes(b"not a source")
            prepare_package(root)
            bundle = root / "flutter" / "src"
            self.assertEqual({p.name for p in bundle.iterdir()},
                             {"CMakeLists.txt", "inditrans.cpp", "script_data.h"})
            for item in bundle.iterdir():
                self.assertEqual(item.read_bytes(), (source / item.name).read_bytes())
            self.assertEqual((root / "flutter" / "CHANGELOG.md").read_bytes(),
                             (root / "CHANGELOG.md").read_bytes())
            (source / "script_data.h").unlink()
            (source / "inditrans.cpp").write_text("new engine")
            prepare_package(root)
            self.assertFalse((bundle / "script_data.h").exists())
            self.assertEqual((bundle / "inditrans.cpp").read_text(), "new engine")

    def test_missing_engine_does_not_replace_existing_bundle(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            bundle = root / "flutter" / "src"
            bundle.mkdir(parents=True)
            (bundle / "inditrans.cpp").write_text("existing")
            with self.assertRaises(FileNotFoundError):
                prepare_package(root)
            self.assertEqual((bundle / "inditrans.cpp").read_text(), "existing")
