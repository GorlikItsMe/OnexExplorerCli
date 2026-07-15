"""Report formatting — console table and JSON output."""
from __future__ import annotations

import json
from typing import Any, Dict, List, Optional, Tuple

from config import BENCH_FILES, LABEL_SHORT, OP_DISPLAY

# ---------------------------------------------------------------------------
# Console report
# ---------------------------------------------------------------------------


def _fmt_ms(ms: float) -> str:
    """Format milliseconds human-readably."""
    return f"{ms / 1000:.2f}s" if ms >= 1000 else f"{ms:.1f}ms"


def print_report(results: Dict[str, dict],
                 platform_info: Dict[str, Any],
                 cli_version: str,
                 iterations: int,
                 warmup: int) -> None:
    """Print a formatted benchmark report to stdout."""

    # Build lookup: label → (filename, size_bytes)
    file_info: Dict[str, Tuple[str, int]] = {}
    for label, mpath, sz in BENCH_FILES:
        fname = mpath.replace("\\", "/").split("/")[-1]
        file_info[label] = (fname, sz)

    def _name(label: str) -> str:
        """Build the Name column value."""
        fname, fsize = file_info.get(label, (label, 0))
        short = LABEL_SHORT.get(label, label)
        if fsize >= 1_000_000:
            sz = f"{fsize / 1_000_000:.0f} MB"
        elif fsize >= 1_000:
            sz = f"{fsize / 1_000:.0f} KB"
        else:
            sz = f"{fsize} B"
        return f"{short} | {fname} ({sz})"

    # ---- Header ----
    print()
    print("═" * 72)
    print("  OnexExplorerCli — Benchmark Report")
    print("═" * 72)
    print()

    print("  System")
    print("  ──────")
    print(f"    OS:        {platform_info.get('os', 'N/A')}")
    if "kernel" in platform_info:
        print(f"    Kernel:    {platform_info['kernel']}")
    print(f"    CPU:       {platform_info.get('cpu', 'N/A')}")
    print(f"    Cores:     {platform_info.get('cpu_count', 'N/A')}")
    print(f"    RAM:       {platform_info.get('ram', 'N/A')}")
    print()

    print("  Tool")
    print("  ────")
    print(f"    Version:   {cli_version}")
    print(f"    Iterations: {iterations}  |  Warmup: {warmup}")
    print()

    # ---- Results table ----
    print("  Results")
    print("  ───────")
    header_cols = (46, 14, 10, 10, 10, 10)
    # fmt: off
    h = f"{'Name':<{header_cols[0]}} {'Operation':<{header_cols[1]}} {'Min':>{header_cols[2]}} {'Median':>{header_cols[3]}} {'Max':>{header_cols[4]}} {'σ':>{header_cols[5]}}  {'Memory':>10}"
    sep = "  " + "─" * (len(h) - 2)
    print(f"  {h}")
    print(sep)
    # fmt: on

    for fmt_label, ops in results.items():
        name = _name(fmt_label)
        for idx, (op, s) in enumerate(ops.items()):
            label = name if idx == 0 else ""
            mem = f"{s['memory_kb'] / 1024:.1f} MB" if s.get("memory_kb") else "      N/A"
            print(f"  {label:<{header_cols[0]}} {OP_DISPLAY[op]:<{header_cols[1]}} "
                  f"{_fmt_ms(s['min_ms']):>{header_cols[2]}} "
                  f"{_fmt_ms(s['median_ms']):>{header_cols[3]}} "
                  f"{_fmt_ms(s['max_ms']):>{header_cols[4]}} "
                  f"{s['stdev_ms']:>{header_cols[5]}.1f}ms  {mem}")
            if s["successes"] < s["samples"]:
                print(f"  {'':<{header_cols[0]}} {'':<{header_cols[1]}} "
                      f"{'':>{header_cols[2]}} {'⚠ failed':>{header_cols[3]}} "
                      f"{s['samples'] - s['successes']}/{s['samples']}")
        print()

    # ---- Footer ----
    total_ops = sum(len(ops) for ops in results.values())
    print(f"  Measured {total_ops} operations × {iterations} iterations = "
          f"{total_ops * iterations} total runs")
    if warmup:
        print(f"  + {total_ops * warmup} warmup runs")
    print()


# ---------------------------------------------------------------------------
# JSON report
# ---------------------------------------------------------------------------

def build_json_report(results: Dict[str, dict],
                      platform_info: Dict[str, Any],
                      cli_version: str,
                      iterations: int,
                      warmup: int) -> dict:
    """Build a JSON-serializable report dictionary."""
    results_out: dict = {}
    for label, mpath, sz in BENCH_FILES:
        fname = mpath.replace("\\", "/").split("/")[-1]
        entry: dict = {"file": fname, "size_bytes": sz}
        if label in results:
            entry["operations"] = {}
            for op, s in results[label].items():
                entry["operations"][op] = {
                    "min_ms": round(s["min_ms"], 2),
                    "max_ms": round(s["max_ms"], 2),
                    "mean_ms": round(s["mean_ms"], 2),
                    "median_ms": round(s["median_ms"], 2),
                    "stdev_ms": round(s["stdev_ms"], 2),
                    "samples": s["samples"],
                    "successes": s["successes"],
                    "memory_kb": s["memory_kb"],
                }
        results_out[label] = entry

    return {
        "benchmark": {
            "tool": "OnexExplorerCli",
            "cli_version": cli_version,
            "iterations": iterations,
            "warmup": warmup,
        },
        "system": platform_info,
        "results": results_out,
    }
