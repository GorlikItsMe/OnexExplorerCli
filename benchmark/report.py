"""Report formatting — console table and JSON output."""
from __future__ import annotations

import json
from dataclasses import dataclass
from typing import Any, Dict, Optional

from config import OpStats


# ---------------------------------------------------------------------------
# Report config
# ---------------------------------------------------------------------------


@dataclass(frozen=True)
class ReportConfig:
    """Immutable settings shared across report functions."""
    cli_version: str
    iterations: int
    warmup: int
    platform_info: Dict[str, Any]


# ---------------------------------------------------------------------------
# Console report
# ---------------------------------------------------------------------------


def _fmt_ms(ms: float) -> str:
    """Format milliseconds with consistent precision."""
    if ms >= 1000:
        return f"{ms / 1000:.3f}s"  # preserves ms-level precision
    return f"{ms:.1f}ms"


def _fmt_size(size: int) -> str:
    if size >= 1_000_000:
        return f"{size // 1_000_000} MB"
    if size >= 1_000:
        return f"{size // 1_000} KB"
    return f"{size} B"


_COL_WIDTHS = {"name": 46, "op": 14, "num": 10, "mem": 10}


def format_stats(stats: OpStats) -> str:
    """Format a single stats row for the console table."""
    mem_val = stats.get("memory_kb")
    mem = f"{mem_val / 1024:.1f} MB" if mem_val is not None else f"{'N/A':>{_COL_WIDTHS['mem']}}"
    return (f"{_fmt_ms(stats['min_ms']):>{_COL_WIDTHS['num']}} "
            f"{_fmt_ms(stats['median_ms']):>{_COL_WIDTHS['num']}} "
            f"{_fmt_ms(stats['max_ms']):>{_COL_WIDTHS['num']}} "
            f"{stats['stdev_ms']:>{_COL_WIDTHS['num']}.1f}ms  "
            f"{mem:>{_COL_WIDTHS['mem']}}")


def failure_warning(stats: OpStats) -> Optional[str]:
    """Return a failure warning string if any runs failed, else None."""
    if stats.get("successes", 0) < stats.get("samples", 0):
        failed = stats["samples"] - stats["successes"]
        return f"⚠ {failed} / {stats['samples']} failed"
    return None


def print_report(results: Dict[str, dict],
                 platform_info: Dict[str, Any],
                 cli_version: str,
                 iterations: int,
                 warmup: int) -> None:
    """Print a formatted benchmark report to stdout."""
    cfg = ReportConfig(
        cli_version=cli_version,
        iterations=iterations,
        warmup=warmup,
        platform_info=platform_info,
    )

    # Header
    print()
    print("═" * 72)
    print("  OnexExplorerCli — Benchmark Report")
    print("═" * 72)
    print()

    print("  System")
    print("  ──────")
    pi = cfg.platform_info
    print(f"    OS:        {pi.get('os', 'N/A')}")
    if "kernel" in pi:
        print(f"    Kernel:    {pi['kernel']}")
    print(f"    CPU:       {pi.get('cpu', 'N/A')}")
    print(f"    Cores:     {pi.get('cpu_count', 'N/A')}")
    print(f"    RAM:       {pi.get('ram', 'N/A')}")
    print()

    print("  Tool")
    print("  ────")
    print(f"    Version:   {cfg.cli_version}")
    print(f"    Iterations: {cfg.iterations}  |  Warmup: {cfg.warmup}")
    print()

    # Results table
    cols = _COL_WIDTHS
    print("  Results")
    print("  ───────")
    hdr = (f"{'Name':<{cols['name']}} {'Operation':<{cols['op']}} "
           f"{'Min':>{cols['num']}} {'Median':>{cols['num']}} "
           f"{'Max':>{cols['num']}} {'σ':>{cols['num']}}  {'Memory':>{cols['mem']}}")
    print(f"  {hdr}")
    print(f"  {'─' * (len(hdr) + 2)}")

    for name, entry in results.items():
        label = f"{name} | {entry['file']} ({_fmt_size(entry['size'])})"

        for idx, (cmd, stats) in enumerate(entry["operations"].items()):
            left = label if idx == 0 else ""
            print(f"  {left:<{cols['name']}} {cmd:<{cols['op']}} "
                  f"{format_stats(stats)}")

            warn = failure_warning(stats)
            if warn:
                print(f"  {'':<{cols['name']}} {'':<{cols['op']}} "
                      f"{'':>{cols['num']}} {'':>{cols['num']}} "
                      f"{'':>{cols['num']}} {'':>{cols['num']}}  "
                      f"{warn:>{cols['mem']}}")
        print()

    # Footer
    total = sum(len(e["operations"]) for e in results.values())
    print(f"  Measured {total} operations × {cfg.iterations} iterations = "
          f"{total * cfg.iterations} total runs")
    if cfg.warmup:
        print(f"  + {total * cfg.warmup} warmup runs")
    print()


# ---------------------------------------------------------------------------
# JSON report
# ---------------------------------------------------------------------------


def _op_stats_to_json(stats: OpStats) -> dict:
    """Serialize a single OpStats to a JSON-friendly dict."""
    return {
        "min_ms": round(stats["min_ms"], 2),
        "max_ms": round(stats["max_ms"], 2),
        "mean_ms": round(stats["mean_ms"], 2),
        "median_ms": round(stats["median_ms"], 2),
        "stdev_ms": round(stats["stdev_ms"], 2),
        "samples": stats["samples"],
        "successes": stats["successes"],
        "memory_kb": stats["memory_kb"],
    }


def build_json_report(results: Dict[str, dict],
                      platform_info: Dict[str, Any],
                      cli_version: str,
                      iterations: int,
                      warmup: int) -> dict:
    """Build a JSON-serializable report dictionary."""
    cfg = ReportConfig(
        cli_version=cli_version,
        iterations=iterations,
        warmup=warmup,
        platform_info=platform_info,
    )

    results_out: dict = {}
    for name, entry in results.items():
        ops = {
            cmd: _op_stats_to_json(stats)
            for cmd, stats in entry["operations"].items()
        }
        results_out[name] = {
            "file": entry["file"],
            "size_bytes": entry["size"],
            "operations": ops,
        }

    return {
        "benchmark": {
            "tool": "OnexExplorerCli",
            "cli_version": cfg.cli_version,
            "iterations": cfg.iterations,
            "warmup": cfg.warmup,
        },
        "system": cfg.platform_info,
        "results": results_out,
    }
