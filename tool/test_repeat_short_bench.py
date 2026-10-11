#!/usr/bin/env python3
"""Tests for benchmark repetition, median aggregation, and CSV consumer compatibility."""

from __future__ import annotations

import csv
import io
import os
import shutil
import stat
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tool"))

import repeat_short_bench
import perf_report


class BenchmarkAggregationTest(unittest.TestCase):
    def test_median_vs_mean_aggregation(self):
        """Confirm [700, 710, 720, 1200, 1500] aggregates to 720, not 966."""
        values = [700.0, 710.0, 720.0, 1200.0, 1500.0]
        self.assertEqual(repeat_short_bench.rounded_median(values), 720)
        self.assertNotEqual(repeat_short_bench.rounded_median(values), 966)
        # Verify rounded_mean computes arithmetic mean
        self.assertEqual(repeat_short_bench.rounded_mean(values), 966)

    def test_odd_run_counts(self):
        """Confirm odd run counts (1, 3, 5, 9) aggregate correctly."""
        self.assertEqual(repeat_short_bench.rounded_median([700.0]), 700)
        self.assertEqual(repeat_short_bench.rounded_median([900.0, 700.0, 800.0]), 800)
        self.assertEqual(repeat_short_bench.rounded_median([700.0, 710.0, 720.0, 1200.0, 1500.0]), 720)
        self.assertEqual(
            repeat_short_bench.rounded_median([10.0, 90.0, 20.0, 80.0, 30.0, 70.0, 40.0, 60.0, 50.0]),
            50,
        )
        # Outlier resistance
        self.assertEqual(repeat_short_bench.rounded_median([500.0, 510.0, 520.0, 530.0, 50000.0]), 520)

    def test_even_run_counts(self):
        """Confirm even run counts and fractional rounding work correctly."""
        # Exact average of middle two
        self.assertEqual(repeat_short_bench.rounded_median([700.0, 710.0]), 705)
        self.assertEqual(repeat_short_bench.rounded_median([100.0, 400.0, 200.0, 300.0]), 250)
        # Half-integer rounding: (700 + 711) / 2 = 705.5 -> int(705.5 + 0.5) = 706
        self.assertEqual(repeat_short_bench.rounded_median([700.0, 711.0]), 706)
        self.assertEqual(repeat_short_bench.rounded_median([1.0, 2.0, 3.0, 4.0, 5.0, 6.0]), 4)
        self.assertEqual(repeat_short_bench.rounded_median([1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]), 5)


class EndToEndRepeatBenchTest(unittest.TestCase):
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

    def test_throughput_median_aggregation_and_raw_output(self):
        """Verify throughput schema aggregates median_ns and sample_p95_ns using median across runs."""
        # Mock benchmark that produces 5 different timing readings across 5 sequential invocations
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
print(f"indic-to-indic,100,{medians[idx]},{p95s[idx]},120,0x123456789abcdef0")
"""
        )
        raw_output = self.dir / "raw.csv"
        proc = subprocess.run(
            [
                sys.executable,
                str(ROOT / "tool/repeat_short_bench.py"),
                "--binary",
                str(mock),
                "--runs",
                "5",
                "--raw-output",
                str(raw_output),
            ],
            capture_output=True,
            text=True,
            check=True,
        )
        lines = proc.stdout.strip().splitlines()
        self.assertEqual(len(lines), 2)
        reader = list(csv.DictReader(lines))
        row = reader[0]
        self.assertEqual(row["case"], "indic-to-indic")
        self.assertEqual(row["input_bytes"], "100")
        self.assertEqual(row["median_ns"], "720")
        self.assertEqual(row["sample_p95_ns"], "820")
        self.assertEqual(row["output_size_sink"], "120")
        self.assertEqual(row["output_fnv1a64"], "0x123456789abcdef0")

        # Verify raw-output contains all 5 runs
        raw_lines = raw_output.read_text(encoding="utf-8").strip().splitlines()
        self.assertEqual(len(raw_lines), 6)  # header + 5 runs
        raw_reader = list(csv.DictReader(raw_lines))
        self.assertEqual([r["run"] for r in raw_reader], ["1", "2", "3", "4", "5"])
        self.assertEqual([r["median_ns"] for r in raw_reader], ["700", "710", "720", "1200", "1500"])

    def test_latency_median_aggregation_and_consumers(self):
        """Verify latency schema aggregates p50_ns and p95_ns, and consumers parse the CSV."""
        mock = self._create_mock_benchmark(
            """
import sys, os
state_file = os.path.join(os.path.dirname(__file__), "lat_counter.txt")
count = int(open(state_file).read()) if os.path.exists(state_file) else 0
with open(state_file, "w") as f:
    f.write(str(count + 1))

p50s = [700, 710, 720, 1200, 1500]
p95s = [850, 860, 870, 1400, 1700]
idx = count % len(p50s)
print("case,input_bytes,p50_ns,p95_ns,output_size_sink,output_fnv1a64")
print(f"indic-to-indic,32,{p50s[idx]},{p95s[idx]},40,0xdeadbeefcafebabe")
"""
        )
        latency_csv = self.dir / "snap-latency.csv"
        proc = subprocess.run(
            [
                sys.executable,
                str(ROOT / "tool/repeat_short_bench.py"),
                "--binary",
                str(mock),
                "--runs",
                "5",
            ],
            capture_output=True,
            text=True,
            check=True,
        )
        latency_csv.write_text(proc.stdout, encoding="utf-8")

        # Verify repeat_short_bench output
        reader = list(csv.DictReader(proc.stdout.strip().splitlines()))
        self.assertEqual(reader[0]["p50_ns"], "720")
        self.assertEqual(reader[0]["p95_ns"], "870")

        # Verify perf_report consumer functions still work
        p50 = perf_report.read_latency(self.dir / "snap", "indic-to-indic")
        self.assertEqual(p50, 720)
        matrix = perf_report.read_metric_matrix(self.dir / "snap", "-latency.csv", "p50_ns")
        self.assertEqual(matrix[("indic-to-indic", 32)], 720)

    def test_awk_consumers_compatibility(self):
        """Verify awk commands from Makefile bench-all and bench-compare can parse the outputs."""
        throughput_content = (
            "case,input_bytes,median_ns,sample_p95_ns,output_size_sink,output_fnv1a64\n"
            "indic-to-indic,100,720,820,120,0x123456789abcdef0\n"
        )
        awk_script = """\
NR==1 { printf "%-28s  %8s  %10s  %8s  %10s\\n","case","bytes","median_ns","MB/s","p95_ns" }
NR >1 { mbs=$2/$3*1000; printf "%-28s  %8s  %10.0f  %8.1f  %10.0f\\n",$1,$2,$3,mbs,$4 }
"""
        proc = subprocess.run(
            ["awk", "-F,", awk_script],
            input=throughput_content,
            capture_output=True,
            text=True,
            check=True,
        )
        self.assertIn("indic-to-indic", proc.stdout)
        self.assertIn("720", proc.stdout)


class ConfigurationAndEnvironmentTest(unittest.TestCase):
    def test_run_perf_default_runs(self):
        """Confirm tool/run-perf.py defaults to 9 runs when RUNS is unset."""
        run_perf_code = (ROOT / "tool/run-perf.py").read_text(encoding="utf-8")
        self.assertIn('runs = os.environ.get("RUNS", "9")', run_perf_code)

    def test_makefile_bench_runs_default_and_override(self):
        """Confirm Makefile BENCH_RUNS default is 5 and allows override."""
        # Query default
        proc_def = subprocess.run(
            ["make", "-f", str(ROOT / "Makefile"), "-f", "-", "--no-print-directory", "check"],
            input="check:\n\t@echo $(BENCH_RUNS)\n",
            cwd=str(ROOT),
            capture_output=True,
            text=True,
            check=True,
        )
        self.assertEqual(proc_def.stdout.strip(), "5")

        # Query override with BENCH_RUNS=9
        proc_override = subprocess.run(
            ["make", "-f", str(ROOT / "Makefile"), "-f", "-", "BENCH_RUNS=9", "--no-print-directory", "check"],
            input="check:\n\t@echo $(BENCH_RUNS)\n",
            cwd=str(ROOT),
            capture_output=True,
            text=True,
            check=True,
        )
        self.assertEqual(proc_override.stdout.strip(), "9")


if __name__ == "__main__":
    unittest.main()
