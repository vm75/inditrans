"""Unit tests for Android 16 KiB ELF and APK ZIP alignment checks."""

from pathlib import Path
import struct
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from check_android_page_alignment import check_archive, elf_load_alignments


def fake_elf(alignment: int) -> bytes:
    data = bytearray(120)
    data[:6] = b"\x7fELF\x02\x01"
    struct.pack_into("<Q", data, 32, 64)
    struct.pack_into("<HH", data, 54, 56, 1)
    struct.pack_into("<I", data, 64, 1)  # PT_LOAD
    struct.pack_into("<Q", data, 64 + 48, alignment)
    return bytes(data)


class AndroidPageAlignmentTests(unittest.TestCase):
    def test_elf_load_segment_alignment(self):
        self.assertEqual(elf_load_alignments(fake_elf(0x4000)), [0x4000])
        self.assertEqual(elf_load_alignments(fake_elf(0x1000)), [0x1000])
        with self.assertRaises(ValueError):
            elf_load_alignments(b"not ELF")

    def test_unaligned_library_is_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            artifact = Path(folder) / "test.aab"
            with zipfile.ZipFile(artifact, "w") as archive:
                archive.writestr("base/lib/arm64-v8a/libinditrans.so", fake_elf(0x1000))
            count, problems = check_archive(artifact)
            self.assertEqual(count, 1)
            self.assertTrue(any("0x1000" in problem for problem in problems))

    def test_aligned_bundle_library_is_accepted(self):
        with tempfile.TemporaryDirectory() as folder:
            artifact = Path(folder) / "test.aab"
            with zipfile.ZipFile(artifact, "w") as archive:
                archive.writestr("base/lib/arm64-v8a/libinditrans.so", fake_elf(0x4000))
            self.assertEqual(check_archive(artifact), (1, []))

    def test_apk_uncompressed_library_zip_alignment(self):
        with tempfile.TemporaryDirectory() as folder:
            artifact = Path(folder) / "test.apk"
            with zipfile.ZipFile(artifact, "w") as archive:
                archive.writestr("lib/arm64-v8a/libinditrans.so", fake_elf(0x4000),
                                 compress_type=zipfile.ZIP_STORED)
            count, problems = check_archive(artifact)
            self.assertEqual(count, 1)
            self.assertTrue(any("ZIP alignment" in problem for problem in problems))

    def test_aligned_apk_library_is_accepted(self):
        with tempfile.TemporaryDirectory() as folder:
            artifact = Path(folder) / "test.apk"
            name = "lib/arm64-v8a/libinditrans.so"
            pad = (-(30 + len(name.encode()))) % 16384
            extra = struct.pack("<HH", 0xCAFE, pad - 4) + bytes(pad - 4)
            member = zipfile.ZipInfo(name)
            member.extra = extra
            member.compress_type = zipfile.ZIP_STORED
            with zipfile.ZipFile(artifact, "w") as archive:
                archive.writestr(member, fake_elf(0x4000))
            self.assertEqual(check_archive(artifact), (1, []))

    def test_missing_64_bit_library_is_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            artifact = Path(folder) / "test.apk"
            with zipfile.ZipFile(artifact, "w") as archive:
                archive.writestr("assets/a.txt", "hello")
            count, problems = check_archive(artifact)
            self.assertEqual(count, 0)
            self.assertTrue(problems)


if __name__ == "__main__":
    unittest.main()
