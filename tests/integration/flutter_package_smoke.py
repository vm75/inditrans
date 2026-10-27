#!/usr/bin/env python3
"""Build the staged Flutter native plugin without access to monorepo sources."""

import ctypes
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def run(*command: str) -> None:
    subprocess.run(command, check=True)


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="inditrans-package-") as temporary:
        root = Path(temporary)
        package = root / "package"
        for directory in ("src", "linux", "windows", "ios", "macos"):
            shutil.copytree(ROOT / "flutter" / directory, package / directory)
        # Both CMake entry points must configure with package-local sources.
        # A parent project also exercises the bundled-library PARENT_SCOPE value.
        for platform in ("linux", "windows"):
            project = root / platform
            project.mkdir()
            (project / "CMakeLists.txt").write_text(
                'cmake_minimum_required(VERSION 3.20)\n'
                'project(package_smoke LANGUAGES CXX)\n'
                f'add_subdirectory("{package.as_posix()}/{platform}" plugin)\n'
                'if(NOT inditrans_bundled_libraries)\n'
                '  message(FATAL_ERROR "Plugin library is not bundled")\n'
                'endif()\n', encoding="utf-8")
            run("cmake", "-S", str(project), "-B", str(root / f"build-{platform}"))
        run("cmake", "--build", str(root / "build-linux"), "--parallel", "2")
        library_path = next((root / "build-linux").rglob("libinditrans.so"))
        library = ctypes.CDLL(str(library_path))
        library.transliterate.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p,
                                          ctypes.c_ulong, ctypes.c_char_p, ctypes.c_char_p]
        library.transliterate.restype = ctypes.c_void_p
        library.releaseBuffer.argtypes = [ctypes.c_void_p]
        library.releaseBuffer.restype = None
        result = library.transliterate("नमस्ते".encode(), b"devanagari", b"bengali", 0, b"", b"")
        assert result, "Packaged engine returned a null buffer"
        try:
            assert ctypes.string_at(result).decode() == "নমস্তে"
        finally:
            library.releaseBuffer(result)
        # Verify the CocoaPods forwarders resolve bundled sources too; Apple
        # framework/link validation remains in the macOS platform workflow.
        for platform in ("ios", "macos"):
            run(os.environ.get("CXX", "c++"), "-std=c++23", "-fsyntax-only",
                str(package / platform / "Classes" / "inditrans.cpp"))
    print("Isolated Flutter package native build and ABI smoke test passed")


if __name__ == "__main__":
    main()
