#!/usr/bin/env python3
"""Regression tests for persistent sparse Git worktrees and shared benchmark harness."""

from __future__ import annotations

import csv
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parents[1]
ROOT_DIR = SCRIPT_DIR.parent
sys.path.insert(0, str(SCRIPT_DIR))

import importlib
import perf_metadata

run_perf = importlib.import_module("run-perf")


class PerfWorktreeTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.git("init", "-b", "main")
        self.git("config", "user.name", "Test User")
        self.git("config", "user.email", "test@example.com")

        # Initial commit with historical engine and dummy files
        (self.root / "native" / "src").mkdir(parents=True)
        (self.root / "native" / "src" / "inditrans.cpp").write_text("// historical inditrans.cpp v1\n")
        (self.root / "native" / "src" / "script_data.h").write_text("// historical script_data.h v1\n")
        (self.root / "flutter").mkdir()
        (self.root / "flutter" / "pubspec.yaml").write_text("name: test\n")
        (self.root / "nodejs").mkdir()
        (self.root / "nodejs" / "package.json").write_text("{}\n")
        (self.root / "README.md").write_text("# Repo\n")
        self.git("add", ".")
        self.git("commit", "-m", "v1 baseline")
        self.git("tag", "perf-00-baseline")
        self.baseline_commit = self.git("rev-parse", "HEAD")

        # Current revision with updated files
        (self.root / "native" / "src" / "inditrans.cpp").write_text("// current inditrans.cpp v2\n")
        (self.root / "native" / "bench").mkdir(parents=True)
        (self.root / "native" / "bench" / "benchmark.cpp").write_text(
            'Case { "devanagari", "telugu", "indic-to-indic", seed },\n'
            'Case { "devanagari", "tamil", "tamil-output", seed },\n'
        )
        self.git("add", ".")
        self.git("commit", "-m", "v2 current")
        self.current_commit = self.git("rev-parse", "HEAD")

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def git(self, *args: str) -> str:
        proc = subprocess.run(
            ["git", "-C", str(self.root), *args],
            check=True,
            capture_output=True,
            text=True,
        )
        return proc.stdout.strip()

    # --- 1. Worktree creation ---
    def test_worktree_creation_points_to_tag_and_is_sparse(self) -> None:
        wt = run_perf.ensure_worktree(self.root, "perf-00-baseline")
        self.assertTrue(wt.is_dir())
        self.assertEqual(wt, self.root / "out" / "perf" / "worktrees" / "perf-00-baseline")

        # Verify HEAD matches tag
        wt_head = subprocess.run(
            ["git", "-C", str(wt), "rev-parse", "HEAD"],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
        self.assertEqual(wt_head, self.baseline_commit)

        # Verify historical engine sources are present
        self.assertEqual(
            (wt / "native" / "src" / "inditrans.cpp").read_text(),
            "// historical inditrans.cpp v1\n",
        )
        self.assertEqual(
            (wt / "native" / "src" / "script_data.h").read_text(),
            "// historical script_data.h v1\n",
        )

        # Verify sparse checkout avoids unnecessary directories
        self.assertFalse((wt / "flutter").exists())
        self.assertFalse((wt / "nodejs").exists())
        self.assertFalse((wt / "native" / "bench").exists())

    # --- 2. Worktree reuse ---
    def test_worktree_reuse_without_duplicates(self) -> None:
        wt1 = run_perf.ensure_worktree(self.root, "perf-00-baseline")
        wt2 = run_perf.ensure_worktree(self.root, "perf-00-baseline")
        self.assertEqual(wt1, wt2)

        registered = run_perf.get_registered_worktrees(self.root)
        self.assertIn(wt1.resolve(), registered)
        self.assertEqual(registered[wt1.resolve()]["HEAD"], self.baseline_commit)

    def test_mismatched_head_detected(self) -> None:
        wt = run_perf.ensure_worktree(self.root, "perf-00-baseline")
        # Change worktree HEAD to a different commit
        subprocess.run(["git", "-C", str(wt), "checkout", "--detach", self.current_commit], check=True)
        with self.assertRaisesRegex(ValueError, "expected"):
            run_perf.ensure_worktree(self.root, "perf-00-baseline")

    def test_existing_non_worktree_directory_causes_error(self) -> None:
        bogus_dir = self.root / "out" / "perf" / "worktrees" / "perf-00-baseline"
        bogus_dir.mkdir(parents=True)
        (bogus_dir / "untracked.txt").write_text("not a worktree")
        with self.assertRaisesRegex(ValueError, "not a registered Git worktree"):
            run_perf.ensure_worktree(self.root, "perf-00-baseline")

    def test_unknown_tag_causes_error(self) -> None:
        with self.assertRaisesRegex(ValueError, "Unknown performance tag or ref"):
            run_perf.ensure_worktree(self.root, "perf-nonexistent")

    # --- 3. Workspace preservation ---
    def test_workspace_preservation(self) -> None:
        # Create dirty workspace: staged change, unstaged change, untracked file, stash
        staged_file = self.root / "staged.txt"
        staged_file.write_text("staged content")
        self.git("add", "staged.txt")

        unstaged_file = self.root / "unstaged.txt"
        unstaged_file.write_text("unstaged content")

        untracked_file = self.root / "untracked.txt"
        untracked_file.write_text("untracked content")

        # Snapshot workspace status before worktree operations
        branch_before = self.git("branch", "--show-current")
        head_before = self.git("rev-parse", "HEAD")
        status_before = [
            line for line in self.git("status", "--porcelain").splitlines()
            if not line.endswith("out/")
        ]

        # Prepare worktree
        wt = run_perf.ensure_worktree(self.root, "perf-00-baseline")
        self.assertTrue(wt.is_dir())

        # Verify workspace remains completely untouched
        branch_after = self.git("branch", "--show-current")
        head_after = self.git("rev-parse", "HEAD")
        status_after = [
            line for line in self.git("status", "--porcelain").splitlines()
            if not line.endswith("out/")
        ]

        self.assertEqual(branch_before, branch_after)
        self.assertEqual(head_before, head_after)
        self.assertEqual(status_before, status_after)
        self.assertEqual(staged_file.read_text(), "staged content")
        self.assertEqual(unstaged_file.read_text(), "unstaged content")
        self.assertEqual(untracked_file.read_text(), "untracked content")

    # --- 4. Harness fingerprint and snapshot metadata ---
    def test_harness_fingerprint_detects_changes(self) -> None:
        fp1 = perf_metadata.compute_harness_fingerprint(ROOT_DIR)
        self.assertEqual(len(fp1), 16)
        # Recomputing gives same fingerprint
        fp2 = perf_metadata.compute_harness_fingerprint(ROOT_DIR)
        self.assertEqual(fp1, fp2)

    def test_snapshot_validity_and_invalidation(self) -> None:
        prefix = self.root / "out" / "perf" / "00-baseline"
        prefix.parent.mkdir(parents=True)

        expected_cases = {"indic-to-indic", "tamil-output"}

        # Missing files -> invalid
        self.assertFalse(
            run_perf.is_snapshot_valid(
                prefix, self.baseline_commit, "abcd1234efgh5678", "5", "0", expected_cases
            )
        )

        # Create valid snapshot files
        for kind in ("throughput", "latency"):
            path = prefix.with_name(f"{prefix.name}-{kind}.csv")
            with path.open("w", newline="", encoding="utf-8") as handle:
                writer = csv.writer(handle)
                writer.writerow(["case", "input_bytes", "median_ns"])
                writer.writerow(["indic-to-indic", "100", "500"])
                writer.writerow(["tamil-output", "100", "600"])
            raw = prefix.with_name(f"{prefix.name}-{kind}-runs.csv")
            raw.write_text("raw run data\n")

        prefix.with_name(f"{prefix.name}-cold.csv").write_text("case,p50_ns\ncold,100\n")
        prefix.with_name(f"{prefix.name}-wasm-size.txt").write_text("12345\n")
        prefix.with_name(f"{prefix.name}-info.txt").write_text(
            f"commit_full={self.baseline_commit}\n"
            "harness_fingerprint=abcd1234efgh5678\n"
            "bench_runs=5\n"
            "bench_cpu=0\n"
        )

        # Valid snapshot matches
        self.assertTrue(
            run_perf.is_snapshot_valid(
                prefix, self.baseline_commit, "abcd1234efgh5678", "5", "0", expected_cases
            )
        )

        # Different harness fingerprint invalidates snapshot
        self.assertFalse(
            run_perf.is_snapshot_valid(
                prefix, self.baseline_commit, "different_fingerprint", "5", "0", expected_cases
            )
        )

        # Different engine commit invalidates snapshot
        self.assertFalse(
            run_perf.is_snapshot_valid(
                prefix, "wrong_commit", "abcd1234efgh5678", "5", "0", expected_cases
            )
        )

        # Missing benchmark case invalidates snapshot
        incomplete_cases = expected_cases | {"missing-case"}
        self.assertFalse(
            run_perf.is_snapshot_valid(
                prefix, self.baseline_commit, "abcd1234efgh5678", "5", "0", incomplete_cases
            )
        )

    def test_validate_snapshot_compatibility_raises_on_missing_cases(self) -> None:
        prefix = self.root / "out" / "perf" / "00-baseline"
        prefix.parent.mkdir(parents=True)
        csv_path = prefix.with_name(f"{prefix.name}-throughput.csv")
        with csv_path.open("w", newline="", encoding="utf-8") as handle:
            writer = csv.writer(handle)
            writer.writerow(["case", "input_bytes", "median_ns"])
            writer.writerow(["indic-to-indic", "100", "500"])

        with self.assertRaisesRegex(ValueError, "missing benchmark cases"):
            run_perf.validate_snapshot_compatibility(
                prefix, {"indic-to-indic", "tamil-output"}, "00-baseline"
            )


if __name__ == "__main__":
    unittest.main()
