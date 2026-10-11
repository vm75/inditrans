#!/usr/bin/env python3
"""Shared statistics and quality metrics for IndiTrans benchmark tooling."""

from __future__ import annotations

import math
from statistics import fmean, median
from typing import Sequence


def rounded_median(values: Sequence[float]) -> int:
    """Return the integer-rounded median of values."""
    if not values:
        raise ValueError("Cannot compute median of empty sequence")
    return int(median(values) + 0.5)


def rounded_mean(values: Sequence[float]) -> int:
    """Return the integer-rounded arithmetic mean of values."""
    if not values:
        raise ValueError("Cannot compute mean of empty sequence")
    return int(fmean(values) + 0.5)


def aggregate_metric(values: Sequence[float], mode: str = "median") -> int:
    """Aggregate a sequence of numeric timings into an integer nanosecond metric."""
    if mode == "median":
        return rounded_median(values)
    elif mode == "mean":
        return rounded_mean(values)
    else:
        raise ValueError(f"Unknown aggregation mode: {mode!r} (expected 'median' or 'mean')")


def compute_mad(values: Sequence[float], center: float | None = None) -> float:
    """Compute Median Absolute Deviation (MAD) across run-level values.

    MAD = median(|x - median(x)| for x in values)
    """
    if not values:
        return 0.0
    if len(values) == 1:
        return 0.0
    c = median(values) if center is None else center
    deviations = [abs(x - c) for x in values]
    return float(median(deviations))


def compute_relative_mad(values: Sequence[float], center: float | None = None) -> float:
    """Compute Relative MAD as a percentage of the central median value.

    relative_MAD = 100 * MAD / median
    Returns 0.0 if the central median is 0.0.
    """
    if not values:
        return 0.0
    c = median(values) if center is None else center
    if c == 0.0:
        return 0.0
    mad = compute_mad(values, center=c)
    return (mad / abs(c)) * 100.0


def classify_quality(
    relative_mad_pct: float,
    warning_pct: float = 2.0,
    critical_pct: float = 5.0,
) -> str:
    """Classify measurement variability quality.

    <= warning_pct: "stable"
    > warning_pct and <= critical_pct: "noisy"
    > critical_pct: "highly unstable"
    """
    if math.isnan(relative_mad_pct):
        return "unknown"
    if relative_mad_pct <= warning_pct:
        return "stable"
    elif relative_mad_pct <= critical_pct:
        return "noisy"
    else:
        return "highly unstable"


def paired_ratios(
    baseline_values: Sequence[float],
    candidate_values: Sequence[float],
) -> list[float]:
    """Compute element-wise paired ratios (candidate_i / baseline_i).

    Requires non-empty sequences of identical length and non-zero baseline values.
    """
    if len(baseline_values) != len(candidate_values):
        raise ValueError(
            f"Mismatched paired sequence lengths: {len(baseline_values)} vs {len(candidate_values)}"
        )
    if not baseline_values:
        raise ValueError("Cannot compute paired ratios of empty sequences")

    ratios: list[float] = []
    for b, c in zip(baseline_values, candidate_values):
        if b <= 0.0:
            raise ValueError(f"Baseline measurement must be strictly positive, got {b}")
        ratios.append(c / b)
    return ratios


def median_paired_ratio(
    baseline_values: Sequence[float],
    candidate_values: Sequence[float],
) -> float:
    """Compute the median of element-wise paired ratios (candidate_i / baseline_i)."""
    ratios = paired_ratios(baseline_values, candidate_values)
    return float(median(ratios))


def paired_regression_pct(
    baseline_values: Sequence[float],
    candidate_values: Sequence[float],
) -> float:
    """Compute regression percentage from the median paired ratio.

    regression_pct = (median_paired_ratio - 1.0) * 100
    Positive values indicate slowdown (regression), negative values indicate speedup.
    """
    ratio = median_paired_ratio(baseline_values, candidate_values)
    return (ratio - 1.0) * 100.0
