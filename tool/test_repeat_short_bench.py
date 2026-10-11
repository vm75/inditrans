#!/usr/bin/env python3
"""Regression tests for median aggregation and historical snapshot compatibility."""

from __future__ import annotations

import csv
import runpy
import stat
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

TOOL = Path(__file__).resolve().parent
import repeat_short_bench


class MedianBenchmarkTest(unittest.TestCase):
    def test_median_handles_noise_and_even_counts(self) -> None:
        self.assertEqual(repeat_short_bench.rounded_median([700, 710, 720, 1200, 1500]), 720)
        self.assertEqual(repeat_short_bench.rounded_median([700, 711]), 706)
        self.assertEqual(repeat_short_bench.rounded_median([5]), 5)

    def test_repeated_binary_keeps_csv_schema_and_raw_runs(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for metrics, first, second, expected in (
                (("median_ns", "sample_p95_ns"), [700, 710, 720, 1200, 1500],
                 [800, 810, 820, 1300, 1600], ("720", "820")),
                (("p50_ns", "p95_ns"), [700, 710, 720, 1200, 1500],
                 [850, 860, 870, 1400, 1700], ("720", "870")),
            ):
                counter = root / "count"
                counter.write_text("0", encoding="utf-8")
                binary = root / "fake_benchmark.py"
                binary.write_text(
                    "#!/usr/bin/env python3\n"
                    "from pathlib import Path\n"
                    f"p = Path({str(counter)!r})\n"
                    "n = int(p.read_text())\n"
                    "p.write_text(str(n + 1))\n"
                    f"a = {first!r}; b = {second!r}\n"
                    f"print('case,input_bytes,{metrics[0]},{metrics[1]},output_size_sink,output_fnv1a64')\n"
                    "print(f'example,100,{a[n]},{b[n]},150,123456789')\n",
                    encoding="utf-8",
                )
                binary.chmod(binary.stat().st_mode | stat.S_IXUSR)
                raw = root / "raw.csv"
                proc = subprocess.run(
                    [sys.executable, str(TOOL / "repeat_short_bench.py"),
                     "--binary", str(binary), "--runs", "5", "--raw-output", str(raw)],
                    check=True, capture_output=True, text=True,
                )
                row = list(csv.DictReader(proc.stdout.splitlines()))[0]
                self.assertEqual((row[metrics[0]], row[metrics[1]]), expected)
                self.assertEqual(row["output_fnv1a64"], "123456789")
                self.assertEqual(len(raw.read_text().splitlines()), 6)

    def test_old_snapshot_reaggregated_from_raw_runs(self) -> None:
        normalize = runpy.run_path(str(TOOL / "run-perf.py"))["normalize_snapshot"]
        with tempfile.TemporaryDirectory() as temp:
            prefix = Path(temp) / "00-baseline"
            target = Path(str(prefix) + "-throughput.csv")
            raw = Path(str(prefix) + "-throughput-runs.csv")
            header = "case,input_bytes,median_ns,sample_p95_ns,output_size_sink,output_fnv1a64"
            # Existing legacy snapshot used an arithmetic mean.
            target.write_text(header + "\nexample,100,966,1066,150,123456789\n", encoding="utf-8")
            readings = [(700, 800), (710, 810), (720, 820), (1200, 1300), (1500, 1600)]
            raw.write_text(
                "run," + header + "\n" +
                "".join(f"{i},example,100,{a},{b},150,123456789\n"
                        for i, (a, b) in enumerate(readings, 1)),
                encoding="utf-8",
            )
            normalize(prefix)
            row = list(csv.DictReader(target.read_text().splitlines()))[0]
            self.assertEqual((row["median_ns"], row["sample_p95_ns"]), ("720", "820"))
            self.assertEqual(row["output_fnv1a64"], "123456789")
            self.assertEqual(len(raw.read_text().splitlines()), 6)

    def test_missing_raw_snapshot_fails_instead_of_mixing_statistics(self) -> None:
        normalize = runpy.run_path(str(TOOL / "run-perf.py"))["normalize_snapshot"]
        with tempfile.TemporaryDirectory() as temp:
            prefix = Path(temp) / "00-baseline"
            Path(str(prefix) + "-throughput.csv").write_text("old data\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "missing"):
                normalize(prefix)


if __name__ == "__main__":
    unittest.main()
