#!/usr/bin/env python3
"""Update every distribution version and prepend release notes.

The Flutter manifest supplies the current version. Keep the replacement
prefixes narrow: this avoids changing unrelated version numbers in build
files. A custom semantic version can be entered interactively, or select a
major/minor/patch increment on the command line.
"""

from dataclasses import dataclass
from pathlib import Path
import re
import sys


VERSION_RE = re.compile(r"(\d+)\.(\d+)\.(\d+)(?:\+(.+))?")


@dataclass(frozen=True)
class Version:
    major: int
    minor: int
    patch: int
    build: str | None = None

    @classmethod
    def parse(cls, value: str) -> "Version":
        match = VERSION_RE.fullmatch(value)
        if not match:
            raise ValueError(f"Invalid version: {value}")
        return cls(int(match[1]), int(match[2]), int(match[3]), match[4])

    def __str__(self) -> str:
        base = f"{self.major}.{self.minor}.{self.patch}"
        return f"{base}+{self.build}" if self.build else base

    def bump(self, kind: str) -> "Version":
        if kind == "major":
            return Version(self.major + 1, 0, 0, self.build)
        if kind == "minor":
            return Version(self.major, self.minor + 1, 0, self.build)
        return Version(self.major, self.minor, self.patch + 1, self.build)


VERSION_FILES = {
    "\nversion: ": ["flutter/pubspec.yaml"],
    '\n  "version": "': ["nodejs/package.json"],
    "\n  s.version          = '": ["flutter/ios/inditrans.podspec", "flutter/macos/inditrans.podspec"],
    '\nversion = "': ["flutter/android/build.gradle"],
    "} VERSION ": ["flutter/linux/CMakeLists.txt", "flutter/windows/CMakeLists.txt"],
}


def version_from_file(path: str, prefix: str) -> Version:
    try:
        contents = Path(path).read_text(encoding="utf-8")
    except OSError as error:
        raise ValueError(f"File not found: {path}") from error
    pattern = re.compile(re.escape(prefix) + r"(\d+\.\d+\.\d+(?:\+[^\s]+)?)")
    for line in contents.splitlines():
        if match := pattern.search(line):
            return Version.parse(match[1])
    raise ValueError(f"Version not found in {path}")


def parse_args(argv: list[str]) -> tuple[str | None, list[str], str | None]:
    kind = None
    messages = []
    custom = None
    index = 0
    while index < len(argv):
        arg = argv[index]
        if arg in {"major", "minor", "patch", "--major", "--minor", "--patch"}:
            kind = arg.removeprefix("--")
        elif arg in {"--log", "log"}:
            index += 1
            if index == len(argv):
                raise ValueError("--log requires a message")
            messages.append(argv[index])
        elif arg.startswith("--log="):
            messages.append(arg.partition("=")[2])
        elif arg in {"help", "--help", "-h"}:
            print("usage: bump_version.py [major|minor|patch] [--log MESSAGE] [VERSION]")
            raise SystemExit(0)
        elif not arg.startswith("-"):
            custom = arg
        else:
            raise ValueError(f"Unknown argument: {arg}")
        index += 1
    return kind, (messages if messages else _interactive_changelog()), custom


def _interactive_changelog() -> list[str]:
    print("Enter changelogs (empty line to stop):")
    result = []
    while line := input():
        if not line.strip() or line.strip() == ".":
            break
        result.append(line.strip())
    if not result:
        raise ValueError("No changelog provided")
    return result


def main(argv: list[str]) -> int:
    try:
        current = version_from_file("flutter/pubspec.yaml", "version: ")
        kind, messages, custom = parse_args(argv)
        next_version = Version.parse(custom) if custom else current.bump(kind) if kind else None
        if next_version is None:
            next_version = Version.parse(input(f'Enter version next to "{current}": '))

        old, new = str(current), str(next_version)
        for prefix, files in VERSION_FILES.items():
            for filename in files:
                path = Path(filename)
                if not path.is_file():
                    raise ValueError(f"Version manifest not found: {filename}")
                contents = path.read_text(encoding="utf-8")
                path.write_text(contents.replace(prefix + old, prefix + new), encoding="utf-8")

        changelog = Path("CHANGELOG.md")
        previous = changelog.read_text(encoding="utf-8")
        section = f"## [{new}]\n" + "".join(f"* {message}\n" for message in messages) + "\n"
        changelog.write_text(section + previous, encoding="utf-8")
        print(f"Updated version from '{old}' to {new}")
        print(f"Commit log: {'. '.join(messages)}")
        return 0
    except (OSError, ValueError, EOFError) as error:
        print(error, file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
