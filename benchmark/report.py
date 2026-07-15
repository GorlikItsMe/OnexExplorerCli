"""Report formatting — console table and JSON output."""
from __future__ import annotations

import json
from typing import Any, Dict

# ---------------------------------------------------------------------------
# Console report
# ---------------------------------------------------------------------------


def _fmt_ms(ms: float) -> str:
    return f"{ms / 1000:.2f}s" if ms >= 1000 else f"{ms:.1f}ms"


def _fmt_size(size: int) -> str:
    if size >= 1_000_000:
        return f"{size / 1_000_000:.0f} MB"
    if size >= 1_000:
        return f"{size / 1_000:.0f} KB"
    return f"{size} B"


def print_report(results: Dict[str, dict],
                 platform_info: Dict[str, Any],
                 cli_version: str,
                 iterations: int,
                 warmup: int) -> None:
    """Print a formatted benchmark report to stdout."""
    # Header
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

    # Results table
    print("  Results")
    print("  ───────")
    cols = {"name": 46, "op": 14, "num": 10, "mem": 10}
    hdr = (f"{'Name':<{cols['name']}} {'Operation':<{cols['op']}} "
           f"{'Min':>{cols['num']}} {'Median':>{cols['num']}} "
           f"{'Max':>{cols['num']}} {'σ':>{cols['num']}}  {'Memory':>{cols['mem']}}")
    print(f"  {hdr}")
    print(f"  {'─' * (len(hdr) + 2)}")

    for name, entry in results.items():
        # Build the Name column value
        label = f"{name} | {entry['file']} ({_fmt_size(entry['size'])})"

        for idx, (cmd, s) in enumerate(entry["operations"].items()):
            left = label if idx == 0 else ""
            mem = (f"{s['memory_kb'] / 1024:.1f} MB" if s.get("memory_kb")
                   else "      N/A")
            print(f"  {left:<{cols['name']}} {cmd:<{cols['op']}} "
                  f"{_fmt_ms(s['min_ms']):>{cols['num']}} "
                  f"{_fmt_ms(s['median_ms']):>{cols['num']}} "
                  f"{_fmt_ms(s['max_ms']):>{cols['num']}} "
                  f"{s['stdev_ms']:>{cols['num']}.1f}ms  {mem:>{cols['mem']}}")
            if s["successes"] < s["samples"]:
                print(f"  {'':<{cols['name']}} {'':<{cols['op']}} "
                      f"{'':>{cols['num']}} {'⚠ failed':>{cols['num']}} "
                      f"{s['samples'] - s['successes']}/{s['samples']}")
        print()

    # Footer
    total = sum(len(e["operations"]) for e in results.values())
    print(f"  Measured {total} operations × {iterations} iterations = "
          f"{total * iterations} total runs")
    if warmup:
        print(f"  + {total * warmup} warmup runs")
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
    for name, entry in results.items():
        ops = {}
        for cmd, s in entry["operations"].items():
            ops[cmd] = {
                "min_ms": round(s["min_ms"], 2),
                "max_ms": round(s["max_ms"], 2),
                "mean_ms": round(s["mean_ms"], 2),
                "median_ms": round(s["median_ms"], 2),
                "stdev_ms": round(s["stdev_ms"], 2),
                "samples": s["samples"],
                "successes": s["successes"],
                "memory_kb": s["memory_kb"],
            }
        results_out[name] = {
            "file": entry["file"],
            "size_bytes": entry["size"],
            "operations": ops,
        }

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
