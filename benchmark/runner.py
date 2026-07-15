"""Benchmark runner — resolve files, run CLI commands, collect measurements."""
from __future__ import annotations

import shutil
import statistics
import subprocess
import sys
import time
from pathlib import Path
from typing import Dict, List, Optional, Tuple

from config import REPO_ROOT, TEMP_DIR

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

MEM_PREFIX = "__MEM__"


def _resolve_nos_file(manifest_path: str) -> Path:
    """Find a .NOS file in known data directories."""
    normalised = manifest_path.replace("\\", "/")
    for p in [REPO_ROOT / "temp" / "nostale" / normalised,
              REPO_ROOT / "temp" / "downloads" / normalised]:
        if p.is_file():
            return p
    raise FileNotFoundError(
        f"Benchmark file not found: {manifest_path}\n"
        "Download it first with ensure_fixture or the download command."
    )


def _run_cli(cli: Path, op: str, nos_path: Path,
             output_dir: Optional[Path] = None,
             measure_memory: bool = False) -> Tuple[subprocess.CompletedProcess, Optional[int]]:
    """Run a single CLI operation, optionally measuring peak RSS via GNU time."""
    # Check for GNU time
    gnu_avail = False
    if measure_memory:
        try:
            r = subprocess.run(["/usr/bin/time", "--version"],
                               capture_output=True, timeout=5)
            gnu_avail = b"GNU" in (r.stdout + r.stderr)
        except (FileNotFoundError, subprocess.TimeoutExpired):
            pass

    args: list = []
    mem_prefix: Optional[str] = None
    if gnu_avail:
        mem_prefix = MEM_PREFIX
        args += ["/usr/bin/time", "--format", f"{MEM_PREFIX}%M"]

    args += [str(cli)]
    if op == "list":
        args += ["list", str(nos_path)]
    elif op == "info":
        args += ["info", "--json", str(nos_path)]
    elif op == "extract":
        if output_dir is None:
            raise ValueError("output_dir required for extract")
        args += ["extract", str(nos_path), "-o", str(output_dir)]

    proc = subprocess.run(args, capture_output=True, text=True, timeout=600)

    memory_kb: Optional[int] = None
    if mem_prefix:
        for line in proc.stderr.splitlines():
            if line.startswith(mem_prefix):
                digits = line[len(mem_prefix):].strip()
                if digits.isdigit():
                    memory_kb = int(digits)
                break

    return proc, memory_kb


# ---------------------------------------------------------------------------
# Main benchmark loop
# ---------------------------------------------------------------------------

def run_benchmark(cli: Path,
                  files: List[Tuple[str, str, int]],
                  operations: List[str],
                  iterations: int,
                  warmup: int,
                  temp_dir: Path) -> Dict[str, dict]:
    """Run the full benchmark suite.

    Returns a nested dict:  result[file_label][op_name] = stats_dict
    where each stats dict has: min_ms, max_ms, mean_ms, median_ms, stdev_ms,
    samples, successes, memory_kb.
    """
    results: Dict[str, dict] = {}

    for fmt_label, manifest_path, _size in files:
        filename = manifest_path.replace("\\", "/").split("/")[-1]
        print(f"\n  {fmt_label}  ({filename})", file=sys.stderr, flush=True)
        nos_path = _resolve_nos_file(manifest_path)
        file_results: dict = {}

        for op in operations:
            print(f"  · {op}", end="", file=sys.stderr, flush=True)
            measurements: List[Tuple[float, Optional[int], bool]] = []
            out_dir: Optional[Path] = None
            if op == "extract":
                safe = fmt_label.replace(" ", "_").replace("(", "").replace(")", "")
                out_dir = temp_dir / f"extract_{safe}"

            # Warmup
            for _ in range(warmup):
                if op == "extract" and out_dir and out_dir.exists():
                    shutil.rmtree(out_dir)
                    out_dir.mkdir(parents=True, exist_ok=True)
                _run_cli(cli, op, nos_path, out_dir, measure_memory=False)

            # Measured runs
            for _ in range(iterations):
                if op == "extract" and out_dir and out_dir.exists():
                    shutil.rmtree(out_dir)
                    out_dir.mkdir(parents=True, exist_ok=True)

                start = time.perf_counter()
                proc, mem = _run_cli(cli, op, nos_path, out_dir, measure_memory=True)
                elapsed = (time.perf_counter() - start) * 1000.0
                measurements.append((elapsed, mem, proc.returncode == 0))

            # Compute stats inline (no dataclass)
            times = [m[0] for m in measurements]
            successes = sum(1 for m in measurements if m[2])
            all_mem = [m[1] for m in measurements if m[1] is not None]
            peak_mem = max(all_mem) if all_mem else None

            n = len(times)
            stats = {
                "min_ms": min(times),
                "max_ms": max(times),
                "mean_ms": statistics.mean(times),
                "median_ms": statistics.median(times),
                "stdev_ms": statistics.stdev(times) if n >= 2 else 0.0,
                "samples": n,
                "successes": successes,
                "memory_kb": peak_mem,
            }

            file_results[op] = stats
            print(f"  min={stats['min_ms']:.1f}ms  median={stats['median_ms']:.1f}ms  "
                  f"max={stats['max_ms']:.1f}ms",
                  file=sys.stderr, flush=True)

        results[fmt_label] = file_results

    return results
