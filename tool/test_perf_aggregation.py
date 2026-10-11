#!/usr/bin/env python3
"""Comprehensive unit and integration tests for benchmark aggregation and variability statistics."""

from __future__ import annotations

import csv
import stat
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
if str(ROOT / "tool") not in sys.path:
    sys.path.insert(0, str(ROOT / "tool"))

import perf_stats
import repeat_short_bench


class PerfStatsUnitTest(unittest.TestCase):
    def test_rounded_median_and_mean(self):
        # Example from problem statement: [700, 710, 720, 1200, 1500]
        # Mean = 966, Median = 720
        samples = [700.0, 710.0, 720.0, 1200.0, 1500.0]
        self.assertEqual(perf_stats.rounded_median(samples), 720)
        self.assertEqual(perf_stats.rounded_mean(samples), 966)

    def test_empty_sequence_raises(self):
        with self.assertRaises(ValueError):
            perf_stats.rounded_median([])
        with self.assertRaises(ValueError):
            perf_stats.rounded_mean([])
        with self.assertRaises(ValueError):
            perf_stats.aggregate_metric([], "median")

    def test_unknown_aggregation_mode_raises(self):
        with self.assertRaises(ValueError):
            perf_stats.aggregate_metric([100.0], "geometric_mean")

    def test_single_value(self):
        self.assertEqual(perf_stats.rounded_median([42.0]), 42)
        self.assertEqual(perf_stats.rounded_mean([42.0]), 42)
        self.assertEqual(perf_stats.compute_mad([42.0]), 0.0)
        self.assertEqual(perf_stats.compute_relative_mad([42.0]), 0.0)

    def test_even_and_odd_run_counts(self):
        # Odd: 3 values
        self.assertEqual(perf_stats.rounded_median([300.0, 100.0, 200.0]), 200)
        # Even: 4 values -> (200 + 300) / 2 = 250
        self.assertEqual(perf_stats.rounded_median([400.0, 100.0, 200.0, 300.0]), 250)
        # Even: fractional rounding (200 + 201) / 2 = 200.5 -> 201
        self.assertEqual(perf_stats.rounded_median([200.0, 201.0]), 201)

    def test_mad_calculation(self):
        # Samples: [700, 710, 720, 1200, 1500]
        # Median = 720
        # Absolute deviations from 720: [20, 10, 0, 480, 780]
        # Sorted deviations: [0, 10, 20, 480, 780] -> Median deviation = 20.0
        samples = [700.0, 710.0, 720.0, 1200.0, 1500.0]
        mad = perf_stats.compute_mad(samples)
        self.assertAlmostEqual(mad, 20.0, places=4)

        # Relative MAD = 100 * 20 / 720 = 2.777... %
        rel_mad = perf_stats.compute_relative_mad(samples)
        self.assertAlmostEqual(rel_mad, (20.0 / 720.0) * 100.0, places=4)

    def test_mad_with_identical_values(self):
        samples = [500.0, 500.0, 500.0, 500.0, 500.0]
        self.assertEqual(perf_stats.compute_mad(samples), 0.0)
        self.assertEqual(perf_stats.compute_relative_mad(samples), 0.0)
        self.assertEqual(perf_stats.classify_quality(0.0), "stable")

    def test_mad_zero_handling(self):
        self.assertEqual(perf_stats.compute_mad([]), 0.0)
        self.assertEqual(perf_stats.compute_relative_mad([]), 0.0)
        self.assertEqual(perf_stats.compute_relative_mad([0.0, 0.0, 0.0]), 0.0)

    def test_quality_classification(self):
        # Default thresholds: <= 2.0% stable, 2.0-5.0% noisy, > 5.0% highly unstable
        self.assertEqual(perf_stats.classify_quality(0.5), "stable")
        self.assertEqual(perf_stats.classify_quality(2.0), "stable")
        self.assertEqual(perf_stats.classify_quality(2.01), "noisy")
        self.assertEqual(perf_stats.classify_quality(4.99), "noisy")
        self.assertEqual(perf_stats.classify_quality(5.0), "noisy")
        self.assertEqual(perf_stats.classify_quality(5.01), "highly unstable")
        self.assertEqual(perf_stats.classify_quality(15.0), "highly unstable")

        # Custom thresholds
        self.assertEqual(
            perf_stats.classify_quality(1.5, warning_pct=1.0, critical_pct=3.0),
            "noisy",
        )
        self.assertEqual(
            perf_stats.classify_quality(3.5, warning_pct=1.0, critical_pct=3.0),
            "highly unstable",
        )

    def test_paired_ratios_and_regression(self):
        # Baseline = [100, 100, 100], Candidate = [110, 110, 110] -> 10% regression
        baseline = [100.0, 100.0, 100.0]
        candidate = [110.0, 110.0, 110.0]
        self.assertEqual(perf_stats.paired_ratios(baseline, candidate), [1.1, 1.1, 1.1])
        self.assertAlmostEqual(perf_stats.median_paired_ratio(baseline, candidate), 1.1, places=4)
        self.assertAlmostEqual(perf_stats.paired_regression_pct(baseline, candidate), 10.0, places=4)

        # Candidate speedup: Baseline = [100, 100], Candidate = [80, 80] -> -20% regression (speedup)
        self.assertAlmostEqual(
            perf_stats.paired_regression_pct([100.0, 100.0], [80.0, 80.0]),
            -20.0,
            places=4,
        )

        # Baseline temporary disturbance should not distort paired median:
        disturbed_b = [100.0, 100.0, 100.0, 500.0, 100.0]
        disturbed_c = [100.0, 100.0, 100.0, 100.0, 500.0]
        self.assertAlmostEqual(
            perf_stats.median_paired_ratio(disturbed_b, disturbed_c),
            1.0,
            places=4,
        )
        self.assertAlmostEqual(
            perf_stats.paired_regression_pct(disturbed_b, disturbed_c),
            0.0,
            places=4,
        )

    def test_paired_mismatched_lengths_raises(self):
        with self.assertRaises(ValueError):
            perf_stats.paired_ratios([100.0], [100.0, 100.0])
        with self.assertRaises(ValueError):
            perf_stats.paired_ratios([], [])
        with self.assertRaises(ValueError):
            perf_stats.paired_ratios([0.0], [100.0])


class RepeatBenchSidecarIntegrationTest(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmpdir.name)

    def tearDown(self):
        self.tmpdir.cleanup()

    def _create_mock_benchmark(self, script_body: str) -> Path:
        script = self.dir / "mock_bench.py"
        script.write_text(f"#!/usr/bin/env python3\n{script_body}", encoding="utf-8")
        script.chmod(script.stat().st_mode | stat.S_IEXEC)
        return script

    def test_stats_sidecar_generation_and_aggregate_modes(self):
        # 5 runs with:
        # medians: [700, 710, 720, 1200, 1500]
        # p95s:    [800, 810, 820, 1300, 1600]
        mock = self._create_mock_benchmark(
            """
import sys, os
state_file = os.path.join(os.path.dirname(__file__), "counter.txt")
count = int(open(state_file).read()) if os.path.exists(state_file) else 0
with open(state_file, "w") as f:
    f.write(str(count + 1))

medians = [700, 710, 720, 1200, 1500]
p95s = [800, 810, 820, 1300, 1600]
idx = count % len(medians)
print("case,input_bytes,median_ns,sample_p95_ns,output_size_sink,output_fnv1a64")
print(f"devanagari,32,{medians[idx]},{p95s[idx]},64,0x1122334455667788")
"""
        )
        stats_file = self.dir / "stats.csv"

        # 1. Test default median mode
        proc_median = subprocess.run(
            [
                sys.executable,
                str(ROOT / "tool/repeat_short_bench.py"),
                "--binary",
                str(mock),
                "--runs",
                "5",
                "--aggregate",
                "median",
                "--stats-output",
                str(stats_file),
            ],
            capture_output=True,
            text=True,
            check=True,
        )
        rows_median = list(csv.DictReader(proc_median.stdout.splitlines()))
        self.assertEqual(rows_median[0]["median_ns"], "720")
        self.assertEqual(rows_median[0]["sample_p95_ns"], "820")

        # Verify sidecar stats file
        self.assertTrue(stats_file.is_file())
        stats_rows = list(csv.DictReader(stats_file.read_text(encoding="utf-8").splitlines()))
        self.assertEqual(len(stats_rows), 1)
        row = stats_rows[0]
        self.assertEqual(row["case"], "devanagari")
        self.assertEqual(row["input_bytes"], "32")
        self.assertEqual(row["runs"], "5")
        self.assertEqual(row["median_ns"], "720")
        self.assertEqual(row["min_ns"], "700")
        self.assertEqual(row["max_ns"], "1500")
        self.assertEqual(row["mad_ns"], "20.0")
        self.assertEqual(row["relative_mad_pct"], "2.78")
        self.assertEqual(row["quality"], "noisy")
        self.assertEqual(row["p95_median_ns"], "820")
        self.assertEqual(row["p95_min_ns"], "800")
        self.assertEqual(row["p95_max_ns"], "1600")

        # 2. Test explicit mean mode
        proc_mean = subprocess.run(
            [
                sys.executable,
                str(ROOT / "tool/repeat_short_bench.py"),
                "--binary",
                str(mock),
                "--runs",
                "5",
                "--aggregate",
                "mean",
            ],
            capture_output=True,
            text=True,
            check=True,
        )
        rows_mean = list(csv.DictReader(proc_mean.stdout.splitlines()))
        self.assertEqual(rows_mean[0]["median_ns"], "966")
        self.assertEqual(rows_mean[0]["sample_p95_ns"], "1066")


if __name__ == "__main__":
    unittest.main()
