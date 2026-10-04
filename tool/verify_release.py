#!/usr/bin/env python3
"""Validate release versions, changelog notes, artifacts, and package dry-runs."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys


SEMVER = re.compile(r"^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$")


def read_match(path: str, pattern: str, label: str, errors: list[str]) -> str | None:
    """Read one manifest field, reporting missing files and malformed values."""
    file = Path(path)
    if not file.is_file():
        errors.append(f"{path} not found.")
        return None
    match = re.search(pattern, file.read_text(encoding="utf-8"), re.MULTILINE)
    if not match:
        errors.append(f"{label} not found in {path}")
        return None
    return match.group(1)


def run_dry_run(command: list[str], cwd: str, label: str, errors: list[str]) -> None:
    """Capture both streams so a failed publish check has useful diagnostics."""
    result = subprocess.run(command, cwd=cwd, text=True, capture_output=True, check=False)
    if result.returncode:
        errors.append(f"{label} failed with exit code {result.returncode}:\n{result.stderr}\n{result.stdout}")
    else:
        print(f"✓ {label} passed cleanly")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tag", nargs="?", help="release tag such as v0.13.0")
    parser.add_argument("--tag", dest="tag_option", help="release tag (same as positional tag)")
    parser.add_argument("--extract-changelog", metavar="PATH")
    parser.add_argument("--check-packages", action="store_true")
    parser.add_argument("--skip-artifacts", action="store_true")
    args = parser.parse_args(argv)

    tag = args.tag_option or args.tag
    if tag is None and os.environ.get("GITHUB_REF_TYPE") == "tag":
        tag = os.environ.get("GITHUB_REF_NAME")
    errors: list[str] = []

    # Tags are the release authority. Local and branch CI checks use pubspec.
    if tag:
        if not tag.startswith("v"):
            print(f"❌ Release tag format error: Tag must start with 'v', got {tag!r}")
            return 1
        expected = tag[1:]
        if not SEMVER.fullmatch(expected):
            print(f"❌ Release tag {tag!r} does not contain valid SemVer: {expected!r}")
            return 1
        print(f"✓ Git tag '{tag}' is sole release authority (expected version: '{expected}')")
    else:
        expected = read_match("flutter/pubspec.yaml", r"^version:\s*([^\s#]+)", "version field", errors)
        if expected is None or not SEMVER.fullmatch(expected):
            print("❌ Error: Valid SemVer version not found in flutter/pubspec.yaml")
            return 1
        print(f"✓ Derived expected version from flutter/pubspec.yaml: '{expected}'")

    checks = [
        ("flutter/pubspec.yaml", r"^version:\s*([^\s#]+)", "flutter/pubspec.yaml"),
        ("flutter/ios/inditrans.podspec", r"s\.version\s*=\s*'([^']+)'", "iOS podspec"),
        ("flutter/macos/inditrans.podspec", r"s\.version\s*=\s*'([^']+)'", "macOS podspec"),
        ("flutter/android/build.gradle", r"version\s*=?\s*[\"']([^\"']+)[\"']", "Android build.gradle"),
        ("flutter/linux/CMakeLists.txt", r"project\(\$\{PROJECT_NAME\}\s+VERSION\s+([^\s\)]+)", "Linux CMakeLists.txt"),
        ("flutter/windows/CMakeLists.txt", r"project\(\$\{PROJECT_NAME\}\s+VERSION\s+([^\s\)]+)", "Windows CMakeLists.txt"),
    ]
    for path, pattern, label in checks:
        value = read_match(path, pattern, "version field", errors)
        if value is not None:
            if value != expected:
                errors.append(f"{label} version '{value}' != '{expected}'")
            else:
                print(f"✓ {label} version matches '{expected}'")

    package = Path("nodejs/package.json")
    if not package.is_file():
        errors.append("nodejs/package.json not found.")
    else:
        try:
            value = json.loads(package.read_text(encoding="utf-8"))["version"]
            if value != expected:
                errors.append(f"nodejs/package.json version '{value}' != '{expected}'")
            else:
                print(f"✓ nodejs/package.json version matches '{expected}'")
        except (json.JSONDecodeError, KeyError, TypeError) as error:
            errors.append(f"Failed to parse nodejs/package.json: {error}")

    changelog = Path("CHANGELOG.md")
    section_match = None
    if not changelog.is_file():
        errors.append("Authoritative CHANGELOG.md not found at repository root.")
    else:
        content = changelog.read_text(encoding="utf-8")
        section_match = re.search(rf"^##\s*\[?v?{re.escape(expected)}\]?.*$", content, re.MULTILINE)
        if not section_match:
            errors.append(f"CHANGELOG.md missing release section for version '{expected}'")
        else:
            print(f"✓ CHANGELOG.md contains release notes for version '{expected}'")
            if args.extract_changelog:
                start = section_match.end()
                following = re.search(r"^##\s+", content[start:], re.MULTILINE)
                body = content[start:start + following.start()] if following else content[start:]
                Path(args.extract_changelog).write_text(body.strip() + "\n", encoding="utf-8")
                print(f"✓ Extracted release notes to '{args.extract_changelog}'")

    if not args.skip_artifacts:
        for path, label in [("flutter/assets/inditrans.wasm", "standalone WASM"),
                            ("js/public/inditrans.js", "JS wrapper")]:
            file = Path(path)
            if not file.is_file() or file.stat().st_size == 0:
                errors.append(f"Required {label} artifact {path} missing or empty.")
            else:
                print(f"✓ {path} present ({file.stat().st_size} bytes)")

    if args.check_packages:
        print("Running package publish dry-runs...")
        temp_changelog = Path("flutter/CHANGELOG.md")
        created = not temp_changelog.exists()
        try:
            if created and changelog.is_file():
                temp_changelog.write_bytes(changelog.read_bytes())
            run_dry_run(["flutter", "pub", "publish", "--dry-run"], "flutter",
                        "flutter pub publish --dry-run", errors)
        finally:
            if created and temp_changelog.exists():
                temp_changelog.unlink()
        run_dry_run(["npm", "pack", "--dry-run"], "nodejs", "npm pack --dry-run", errors)

    if errors:
        print(f"\n❌ Release validation failed with {len(errors)} error(s):")
        for error in errors:
            print(f"  - {error}")
        return 1
    print(f"\n✨ All release and version validation checks PASSED for {expected}!")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
