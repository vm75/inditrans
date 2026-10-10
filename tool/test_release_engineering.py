#!/usr/bin/env python3
"""Automated regression checks for release engineering and validation logic.

Verifies:
1. SemVer tag validation rules (valid, missing 'v', non-SemVer, prerelease/build).
2. Manifest consistency and mismatch detection across all distributions.
3. Changelog entry requirement and release notes extraction.
4. Binary artifact detection and failure on missing artifacts.
5. Command-line flag parsing (--check-artifacts, --skip-artifacts, unrecognized flags).
6. Workflow structure integrity (trigger constraints, least privilege permissions,
   job dependency topology, OIDC configuration, publication isolation from branch CI).
"""

from __future__ import annotations

import io
import json
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch

# Add repository root to path
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tool"))

import verify_release


class ReleaseValidatorTest(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmpdir.name)
        # Populate minimal valid mock repository layout
        self.create_manifests("0.13.0")
        self.create_changelog("0.13.0", "Release 0.13.0 notes")
        self.create_artifacts()

    def tearDown(self):
        self.tmpdir.cleanup()

    def create_manifests(self, version: str):
        (self.dir / "flutter").mkdir(parents=True, exist_ok=True)
        (self.dir / "flutter/pubspec.yaml").write_text(f"name: inditrans\nversion: {version}\n", encoding="utf-8")

        (self.dir / "flutter/ios").mkdir(parents=True, exist_ok=True)
        (self.dir / "flutter/ios/inditrans.podspec").write_text(f"Pod::Spec.new do |s|\n  s.version = '{version}'\nend\n", encoding="utf-8")

        (self.dir / "flutter/macos").mkdir(parents=True, exist_ok=True)
        (self.dir / "flutter/macos/inditrans.podspec").write_text(f"Pod::Spec.new do |s|\n  s.version = '{version}'\nend\n", encoding="utf-8")

        (self.dir / "flutter/android").mkdir(parents=True, exist_ok=True)
        (self.dir / "flutter/android/build.gradle").write_text(f'version = "{version}"\n', encoding="utf-8")

        (self.dir / "flutter/linux").mkdir(parents=True, exist_ok=True)
        (self.dir / "flutter/linux/CMakeLists.txt").write_text(f"project(${{PROJECT_NAME}} VERSION {version})\n", encoding="utf-8")

        (self.dir / "flutter/windows").mkdir(parents=True, exist_ok=True)
        (self.dir / "flutter/windows/CMakeLists.txt").write_text(f"project(${{PROJECT_NAME}} VERSION {version})\n", encoding="utf-8")

        (self.dir / "nodejs").mkdir(parents=True, exist_ok=True)
        (self.dir / "nodejs/package.json").write_text(json.dumps({"name": "@vm75/inditrans", "version": version}), encoding="utf-8")

    def create_changelog(self, version: str, notes: str):
        content = f"""# Changelog

## [{version}]
* {notes}

## [0.12.0]
* Older changes
"""
        (self.dir / "CHANGELOG.md").write_text(content, encoding="utf-8")

    def create_artifacts(self):
        (self.dir / "flutter/assets").mkdir(parents=True, exist_ok=True)
        (self.dir / "flutter/assets/inditrans.wasm").write_bytes(b"\x00asm\x01\x00\x00\x00mockwasm")

        (self.dir / "js/public").mkdir(parents=True, exist_ok=True)
        (self.dir / "js/public/inditrans.js").write_text("var Module = {};", encoding="utf-8")

    def run_validator(self, args: list[str]) -> tuple[int, str]:
        cwd = os.getcwd()
        os.chdir(self.dir)
        stdout = io.StringIO()
        try:
            with patch("sys.stdout", stdout):
                code = verify_release.main(args)
        finally:
            os.chdir(cwd)
        return code, stdout.getvalue()

    # 1. Valid and invalid SemVer tags
    def test_valid_tag(self):
        code, out = self.run_validator(["--tag=v0.13.0"])
        self.assertEqual(code, 0)
        self.assertIn("All release and version validation checks PASSED", out)

    def test_valid_positional_tag(self):
        code, out = self.run_validator(["v0.13.0"])
        self.assertEqual(code, 0)
        self.assertIn("All release and version validation checks PASSED", out)

    def test_tag_missing_v_prefix(self):
        code, out = self.run_validator(["--tag=0.13.0"])
        self.assertEqual(code, 1)
        self.assertIn("Tag must start with 'v'", out)

    def test_invalid_semver_tag(self):
        code, out = self.run_validator(["--tag=v1.2"])
        self.assertEqual(code, 1)
        self.assertIn("does not contain valid SemVer", out)

    def test_prerelease_tag_rejected_by_semver_pattern(self):
        code, out = self.run_validator(["--tag=v0.13.0-rc1"])
        self.assertEqual(code, 1)
        self.assertIn("does not contain valid SemVer", out)

    def test_tag_with_build_metadata_rejected(self):
        code, out = self.run_validator(["--tag=v0.13.0+build123"])
        self.assertEqual(code, 1)
        self.assertIn("does not contain valid SemVer", out)

    # 2. Mismatched package versions
    def test_mismatched_nodejs_version(self):
        (self.dir / "nodejs/package.json").write_text(json.dumps({"name": "@vm75/inditrans", "version": "0.12.0"}), encoding="utf-8")
        code, out = self.run_validator(["--tag=v0.13.0"])
        self.assertEqual(code, 1)
        self.assertIn("nodejs/package.json version '0.12.0' != '0.13.0'", out)

    def test_mismatched_ios_podspec_version(self):
        (self.dir / "flutter/ios/inditrans.podspec").write_text("Pod::Spec.new do |s|\n  s.version = '0.12.0'\nend\n", encoding="utf-8")
        code, out = self.run_validator(["--tag=v0.13.0"])
        self.assertEqual(code, 1)
        self.assertIn("iOS podspec version '0.12.0' != '0.13.0'", out)

    def test_mismatched_android_gradle_version(self):
        (self.dir / "flutter/android/build.gradle").write_text('version = "0.12.0"\n', encoding="utf-8")
        code, out = self.run_validator(["--tag=v0.13.0"])
        self.assertEqual(code, 1)
        self.assertIn("Android build.gradle version '0.12.0' != '0.13.0'", out)

    # 3. Missing changelog entries
    def test_missing_changelog_entry(self):
        self.create_changelog("0.12.0", "Older notes only")
        code, out = self.run_validator(["--tag=v0.13.0"])
        self.assertEqual(code, 1)
        self.assertIn("CHANGELOG.md missing release section for version '0.13.0'", out)

    def test_changelog_notes_extraction(self):
        notes_file = self.dir / "extracted_notes.md"
        code, out = self.run_validator(["--tag=v0.13.0", f"--extract-changelog={notes_file}"])
        self.assertEqual(code, 0)
        self.assertTrue(notes_file.is_file())
        self.assertIn("Release 0.13.0 notes", notes_file.read_text(encoding="utf-8"))

    # 4. Missing release artifacts
    def test_missing_wasm_artifact(self):
        (self.dir / "flutter/assets/inditrans.wasm").unlink()
        code, out = self.run_validator(["--tag=v0.13.0"])
        self.assertEqual(code, 1)
        self.assertIn("Required standalone WASM artifact", out)

    def test_empty_wasm_artifact(self):
        (self.dir / "flutter/assets/inditrans.wasm").write_bytes(b"")
        code, out = self.run_validator(["--tag=v0.13.0"])
        self.assertEqual(code, 1)
        self.assertIn("Required standalone WASM artifact flutter/assets/inditrans.wasm missing or empty", out)

    def test_skip_artifacts_flag(self):
        (self.dir / "flutter/assets/inditrans.wasm").unlink()
        code, out = self.run_validator(["--tag=v0.13.0", "--skip-artifacts"])
        self.assertEqual(code, 0)

    # 5. Argument parser consistency
    def test_check_artifacts_flag_accepted(self):
        code, out = self.run_validator(["--tag=v0.13.0", "--check-artifacts"])
        self.assertEqual(code, 0)

    def test_invalid_flag_rejected(self):
        cwd = os.getcwd()
        os.chdir(self.dir)
        stderr = io.StringIO()
        try:
            with patch("sys.stderr", stderr):
                with self.assertRaises(SystemExit) as ctx:
                    verify_release.main(["--nonexistent-flag"])
                self.assertNotEqual(ctx.exception.code, 0)
        finally:
            os.chdir(cwd)


class WorkflowIntegrityTest(unittest.TestCase):
    """Static audit of GitHub Actions release and CI workflows."""

    def setUp(self):
        self.ci_yaml = (ROOT / ".github/workflows/ci.yml").read_text(encoding="utf-8")
        self.platform_yaml = (ROOT / ".github/workflows/platform.yml").read_text(encoding="utf-8")
        self.release_yaml = (ROOT / ".github/workflows/release.yml").read_text(encoding="utf-8")

    def test_ci_workflow_never_publishes(self):
        """Verify CI and platform workflows never contain publishing commands."""
        forbidden_commands = [
            "flutter pub publish --force",
            "npm publish",
            "gh release create",
        ]
        for cmd in forbidden_commands:
            self.assertNotIn(cmd, self.ci_yaml, f"CI workflow should not contain publish command: {cmd}")
            self.assertNotIn(cmd, self.platform_yaml, f"Platform workflow should not contain publish command: {cmd}")

    def test_platform_workflow_trigger_restricted(self):
        """Platform workflow must trigger strictly on platform-* tags or manual dispatch."""
        self.assertIn("push:", self.platform_yaml)
        self.assertIn("tags:", self.platform_yaml)
        self.assertIn("- 'platform-*'", self.platform_yaml)
        self.assertIn("workflow_dispatch:", self.platform_yaml)
        self.assertNotIn("branches:", self.platform_yaml)

    def test_release_workflow_trigger_restricted_to_semver_tags(self):
        """Release workflow must trigger strictly on SemVer tags 'v*.*.*'."""
        self.assertIn("push:", self.release_yaml)
        self.assertIn("tags:", self.release_yaml)
        self.assertIn("- 'v[0-9]+.[0-9]+.[0-9]+'", self.release_yaml)
        # Verify branches cannot trigger release.yml
        self.assertNotIn("branches:", self.release_yaml)

    def test_release_job_dependency_chain(self):
        """Ensure publish and release jobs have strict dependencies blocking premature publishing."""
        # test-and-build depends on validate
        self.assertIn("test-and-build:", self.release_yaml)
        self.assertRegex(self.release_yaml, r"test-and-build:\s+name:[^\n]+\s+needs:\s*validate")

        # publish-pub depends on test-and-build
        self.assertRegex(self.release_yaml, r"publish-pub:\s+name:[^\n]+\s+needs:\s*test-and-build")

        # publish-npm depends on test-and-build
        self.assertRegex(self.release_yaml, r"publish-npm:\s+name:[^\n]+\s+needs:\s*test-and-build")

        # github-release depends on validate, test-and-build, publish-pub, publish-npm
        self.assertRegex(
            self.release_yaml,
            r"github-release:\s+name:[^\n]+\s+needs:\s*\[validate,\s*test-and-build,\s*publish-pub,\s*publish-npm\]",
        )

    def test_native_release_builds_precede_emscripten_setup(self):
        """Keep Emscripten's cmake/ directory out of native build command lookup."""
        build_job = self.release_yaml.split("  test-and-build:", 1)[1].split("  publish-pub:", 1)[0]
        emsdk_setup = build_job.index("uses: emscripten-core/setup-emsdk@")
        for command in (
            "make dll",
            "./tool/verify_dll.sh native/build_win/inditrans.dll",
            "cmake -S native/src -B native/build_linux",
            "cmake --build native/build_linux",
        ):
            with self.subTest(command=command):
                self.assertLess(build_job.index(command), emsdk_setup)

    def test_release_restores_generated_dll_copy_before_package_validation(self):
        """DLL verification should not leave the checked-in example copy dirty for pub dry-runs."""
        build_job = self.release_yaml.split("  test-and-build:", 1)[1].split("  publish-pub:", 1)[0]
        verify = build_job.index("./tool/verify_dll.sh native/build_win/inditrans.dll")
        restore = build_job.index(
            "git restore --source=HEAD --worktree flutter/example.dart/inditrans.dll"
        )
        package_validation = build_job.index("--check-packages")
        self.assertLess(verify, restore)
        self.assertLess(restore, package_validation)

    def test_release_analysis_resolves_standalone_example_dependencies(self):
        """Root analysis includes the standalone Dart example's package imports."""
        build_job = self.release_yaml.split("  test-and-build:", 1)[1].split("  publish-pub:", 1)[0]
        self.assertLess(
            build_job.index("(cd example.dart && dart pub get)"),
            build_job.index("dart analyze"),
        )

    def test_oidc_permissions_least_privilege(self):
        """Verify workflow-level and job-level permissions enforce least privilege and OIDC id-token write."""
        # Top-level default must be read-only
        self.assertRegex(self.release_yaml, r"(?m)^permissions:\s*\n\s+contents:\s*read")

        # publish-pub has id-token: write and contents: read
        pub_match = re.search(r"publish-pub:.*?(?=publish-npm:)", self.release_yaml, re.DOTALL)
        self.assertIsNotNone(pub_match)
        self.assertIn("id-token: write", pub_match.group(0))
        self.assertIn("contents: read", pub_match.group(0))

        # publish-npm has id-token: write and contents: read
        npm_match = re.search(r"publish-npm:.*?(?=github-release:)", self.release_yaml, re.DOTALL)
        self.assertIsNotNone(npm_match)
        self.assertIn("id-token: write", npm_match.group(0))
        self.assertIn("contents: read", npm_match.group(0))

        # github-release has contents: write
        gh_match = re.search(r"github-release:.*", self.release_yaml, re.DOTALL)
        self.assertIsNotNone(gh_match)
        self.assertIn("contents: write", gh_match.group(0))

    def test_pub_publish_configures_oidc_before_flutter(self):
        """Flutter setup alone does not provision pub.dev OIDC credentials."""
        pub_job = self.release_yaml.split("  publish-pub:", 1)[1].split("  publish-npm:", 1)[0]
        self.assertIn("environment: pub.dev", pub_job)
        self.assertLess(
            pub_job.index("uses: dart-lang/setup-dart@v1"),
            pub_job.index("uses: subosito/flutter-action@v2"),
        )
        self.assertIn("flutter pub publish --force", pub_job)

    def test_npm_install_does_not_require_token_config(self):
        """setup-node registry-url creates an .npmrc token placeholder that breaks Yarn."""
        npm_job = self.release_yaml.split("  publish-npm:", 1)[1].split("  github-release:", 1)[0]
        self.assertIn("environment: npm", npm_job)
        self.assertNotIn("registry-url:", npm_job)
        self.assertIn("yarn install --frozen-lockfile", npm_job)
        self.assertIn("npm publish --access=public", npm_job)

    def test_no_hardcoded_npm_tokens_in_release_workflow(self):
        """Release workflow uses OIDC trusted publishing; no long-lived token secret is referenced."""
        self.assertNotIn("secrets.NPM_TOKEN", self.release_yaml)
        self.assertNotIn("NODE_AUTH_TOKEN", self.release_yaml)


if __name__ == "__main__":
    unittest.main()
