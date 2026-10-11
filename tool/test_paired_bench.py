#!/usr/bin/env python3
"""Unit and integration tests for paired counterbalanced benchmarking."""

from __future__ import annotations

import csv
import stat
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parent.parent
if str(ROOT / "tool") not in sys.path:
    sys.path.insert(0, str(ROOT / "tool"))

import paired_bench
import perf_stats


class PairedMathAndOrderingTest(unittest.TestCase):
    def test_paired_ratio_and_regression_calculation(self):
        """Confirm element-wise paired ratios and regression percentages."""
        # 3 paired rounds: baseline 100ns, candidate 110ns -> ratio 1.10 (+10.0%)
        b = [100.0, 100.0, 100.0]
        c = [110.0, 110.0, 110.0]
        ratios = perf_stats.paired_ratios(b, c)
        self.assertEqual(ratios, [1.1, 1.1, 1.1])
        self.assertAlmostEqual(perf_stats.median_paired_ratio(b, c), 1.1, places=4)
        self.assertAlmostEqual(perf_stats.paired_regression_pct(b, c), 10.0, places=4)

        # Speedup: baseline 200ns, candidate 160ns -> ratio 0.80 (-20.0%)
        b2 = [200.0, 200.0]
        c2 = [160.0, 160.0]
        self.assertAlmostEqual(perf_stats.paired_regression_pct(b2, c2), -20.0, places=4)

    def test_disturbance_resilience_of_median_paired_ratio(self):
        """Demonstrate that paired median ratio resists host hitches on either baseline or candidate.

        Scenario from specification:
        Round 1: B=700, C=695 -> R_1 = 0.9929 (-0.7%)
        Round 2: C=715, B=720 -> R_2 = 0.9931 (-0.7%)
        Round 3: B=705, C=700 -> R_3 = 0.9929 (-0.7%)
        Round 4: B=1200 (WSL2 hitch on baseline), C=705 -> R_4 = 0.5875
        Round 5: B=710, C=1220 (WSL2 hitch on candidate) -> R_5 = 1.7183
        """
        b = [700.0, 720.0, 705.0, 1200.0, 710.0]
        c = [695.0, 715.0, 700.0, 705.0, 1220.0]

        ratios = perf_stats.paired_ratios(b, c)
        med_ratio = perf_stats.median_paired_ratio(b, c)
        regression_pct = perf_stats.paired_regression_pct(b, c)

        # True underlying difference is approx -0.7% (speedup)
        # Median ratio should be approx 695/700 = 0.9929 (-0.71%)
        self.assertAlmostEqual(med_ratio, 695.0 / 700.0, places=3)
        self.assertAlmostEqual(regression_pct, -0.71, places=1)

        # Arithmetic mean of ratios would be heavily pulled by round 5 outlier:
        mean_ratio = sum(ratios) / len(ratios)
        # Ratios: [0.9929, 0.9931, 0.9929, 0.5875, 1.7183] -> mean = 1.0569 (+5.7% false regression!)
        self.assertGreater(mean_ratio, 1.05)
        # Proves that median ratio is immune to single-side host hitches where mean of ratios fails.

    def test_proportional_slowdown_preserved_under_paired_ratios(self):
        """If host system throttles both baseline and candidate proportionally, paired ratio is invariant."""
        # Clean run: B=500, C=550 -> 1.10 ratio (+10%)
        # Throttled system (2x slower for all processes): B=1000, C=1100 -> 1.10 ratio (+10%)
        b = [500.0, 1000.0, 500.0, 1000.0]
        c = [550.0, 1100.0, 550.0, 1100.0]
        self.assertAlmostEqual(perf_stats.median_paired_ratio(b, c), 1.10, places=4)
        self.assertAlmostEqual(perf_stats.paired_regression_pct(b, c), 10.0, places=4)


class PairedRunnerMockIntegrationTest(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmpdir.name)
        self._bench_idx = 0

    def tearDown(self):
        self.tmpdir.cleanup()

    def _create_mock_benchmark(self, script_body: str) -> Path:
        self._bench_idx += 1
        script = self.dir / f"mock_bench_{self._bench_idx}.py"
        script.write_text(f"#!/usr/bin/env python3\n{script_body}", encoding="utf-8")
        script.chmod(script.stat().st_mode | stat.S_IEXEC)
        return script

    def test_counterbalanced_execution_order_and_record_generation(self):
        """Verify odd rounds run baseline first and even rounds run candidate first."""
        order_log = self.dir / "order_log.txt"

        mock_b = self._create_mock_benchmark(f"""
import os
with open('{order_log}', 'a') as f:
    f.write('B\\n')
print('case,input_bytes,p50_ns,p95_ns,output_size_sink,output_fnv1a64')
print('devanagari,32,700,750,64,0x1122334455667788')
""")
        mock_c = self._create_mock_benchmark(f"""
import os
with open('{order_log}', 'a') as f:
    f.write('C\\n')
print('case,input_bytes,p50_ns,p95_ns,output_size_sink,output_fnv1a64')
print('devanagari,32,700,750,64,0x1122334455667788')
""")

        out_dir = self.dir / "out"

        with patch("paired_bench.build_binaries") as mock_build:
            mock_build.side_effect = [
                (mock_b, mock_b),
                (mock_c, mock_c),
            ]
            exit_code, stats, records = paired_bench.run_paired_benchmarks(
                repo_root=ROOT,
                baseline_ref="mock-baseline",
                candidate_ref="mock-candidate",
                runs=4,
                cpu="",
                suite="latency",
                output_dir=out_dir,
                noise_warning_pct=2.0,
                noise_critical_pct=5.0,
                fail_on_hash_mismatch=True,
            )

        self.assertEqual(exit_code, 0)
        self.assertEqual(len(stats), 1)

        # Check execution sequence:
        # Round 1: B -> C
        # Round 2: C -> B
        # Round 3: B -> C
        # Round 4: C -> B
        log_entries = order_log.read_text(encoding="utf-8").strip().splitlines()
        self.assertEqual(log_entries, ["B", "C", "C", "B", "B", "C", "C", "B"])

        # Check paired-runs.csv
        runs_csv = out_dir / "paired-runs.csv"
        self.assertTrue(runs_csv.is_file())
        rows = list(csv.DictReader(runs_csv.read_text(encoding="utf-8").splitlines()))
        self.assertEqual(len(rows), 8)
        # Check round 1
        self.assertEqual((rows[0]["round"], rows[0]["execution_order"], rows[0]["target"]), ("1", "1", "baseline"))
        self.assertEqual((rows[1]["round"], rows[1]["execution_order"], rows[1]["target"]), ("1", "2", "candidate"))
        # Check round 2
        self.assertEqual((rows[2]["round"], rows[2]["execution_order"], rows[2]["target"]), ("2", "1", "candidate"))
        self.assertEqual((rows[3]["round"], rows[3]["execution_order"], rows[3]["target"]), ("2", "2", "baseline"))

        # Check paired-stats.csv
        stats_csv = out_dir / "paired-stats.csv"
        self.assertTrue(stats_csv.is_file())
        stat_rows = list(csv.DictReader(stats_csv.read_text(encoding="utf-8").splitlines()))
        self.assertEqual(stat_rows[0]["baseline_median_ns"], "700")
        self.assertEqual(stat_rows[0]["candidate_median_ns"], "700")
        self.assertEqual(stat_rows[0]["median_ratio"], "1.0000")
        self.assertEqual(stat_rows[0]["hash_match"], "1")

        # Check report markdown
        report_md = out_dir / "paired-report.md"
        self.assertTrue(report_md.is_file())
        content = report_md.read_text(encoding="utf-8")
        self.assertIn("# Paired Performance Comparison Report", content)
        self.assertIn("devanagari", content)

    def test_hash_mismatch_failure(self):
        """Verify that FNV-1a output hash mismatch is detected and triggers failure when requested."""
        mock_b = self._create_mock_benchmark("""
print('case,input_bytes,p50_ns,p95_ns,output_size_sink,output_fnv1a64')
print('case1,32,700,750,64,0x1111111111111111')
""")
        mock_c = self._create_mock_benchmark("""
print('case,input_bytes,p50_ns,p95_ns,output_size_sink,output_fnv1a64')
print('case1,32,700,750,64,0x9999999999999999')
""")
        out_dir = self.dir / "out"

        with patch("paired_bench.build_binaries") as mock_build:
            mock_build.side_effect = [
                (mock_b, mock_b),
                (mock_c, mock_c),
            ]
            exit_code, stats, _ = paired_bench.run_paired_benchmarks(
                repo_root=ROOT,
                baseline_ref="mock-baseline",
                candidate_ref="mock-candidate",
                runs=2,
                cpu="",
                suite="latency",
                output_dir=out_dir,
                noise_warning_pct=2.0,
                noise_critical_pct=5.0,
                fail_on_hash_mismatch=True,
            )

        self.assertEqual(exit_code, 1)
        self.assertFalse(stats[0].hash_match)

        # Check report marks hash mismatch
        report = (out_dir / "paired-report.md").read_text(encoding="utf-8")
        self.assertIn("Output Hash Mismatches Detected", report)


class RepositorySafetyTest(unittest.TestCase):
    def test_git_working_tree_not_modified_during_paired_bench(self):
        """Verify paired benchmark runner leaves working directory completely untouched."""
        status_before = subprocess.run(
            ["git", "status", "--porcelain"],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=True,
        ).stdout

        branch_before = subprocess.run(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=True,
        ).stdout

        # Verify build_binaries creates builds in isolated temp directory without touching repo
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            target_dir = tmp_path / "test_target"
            # Extract baseline commit fa145f3 into tmp
            short_bin, throughput_bin = paired_bench.build_binaries(
                repo_root=ROOT,
                ref="fa145f3",
                target_dir=target_dir,
                name="test_baseline",
            )
            self.assertTrue(short_bin.is_file())
            self.assertTrue(throughput_bin.is_file())

        status_after = subprocess.run(
            ["git", "status", "--porcelain"],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=True,
        ).stdout

        branch_after = subprocess.run(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=True,
        ).stdout

        self.assertEqual(status_before, status_after)
        self.assertEqual(branch_before, branch_after)


if __name__ == "__main__":
    unittest.main()
