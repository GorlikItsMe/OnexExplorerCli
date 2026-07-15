#!/usr/bin/env python3
"""
Benchmark OnexExplorerCli operations (list, info --json, extract) on .NOS archives.

Usage:
    python3 benchmark/bench.py
    python3 benchmark/bench.py --iterations 3 --warmup 0
    python3 benchmark/bench.py --json
"""

from __future__ import annotations

import argparse
import json
import math
import os
import platform
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Config – adjust these at the top for quick changes
# ---------------------------------------------------------------------------

BENCH_ITERATIONS = 5
BENCH_WARMUP = 1

# (label, manifest_path, approximate_size_bytes)
BENCH_FILES: List[Tuple[str, str, int]] = [
    ("Zlib (32GBS)",  "NostaleData\\NS4BbData.NOS",   80_299_180),
    ("Zlib (NT Data)","NostaleData\\NStpData01.NOS",  50_329_132),
    ("CCINF V1.20",   "NostaleData\\NSmnData.NOS",       552_054),
    ("Text",          "NostaleData\\NSgtdData.NOS",    18_105_099),
]

BENCH_OPERATIONS = ["list", "info --json", "extract"]

# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

_SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = _SCRIPT_DIR.parent
TEMP_DIR = _SCRIPT_DIR / "temp"


def _find_cli() -> Path:
    """Locate the OnexExplorerCli binary."""
    if sys.platform == "win32":
        candidates = [
            REPO_ROOT / "build" / "standalone" / "Release" / "OnexExplorerCli.exe",
            REPO_ROOT / "build" / "standalone" / "OnexExplorerCli.exe",
            REPO_ROOT / "build" / "standalone" / "Debug" / "OnexExplorerCli.exe",
        ]
    else:
        candidates = [
            REPO_ROOT / "build" / "standalone" / "OnexExplorerCli",
            REPO_ROOT / "build" / "standalone" / "OnexExplorerCli",
        ]
    for p in candidates:
        if p.is_file():
            return p
    raise FileNotFoundError(
        "OnexExplorerCli binary not found. Build it first:\n"
        f"  cmake --build {REPO_ROOT}/build/standalone -j$(nproc)"
    )


def _resolve_nos_file(manifest_path: str) -> Path:
    """Find a .NOS file in known data directories."""
    # Normalise backslashes from CDN manifest paths
    normalised = manifest_path.replace("\\", "/")
    candidates = [
        REPO_ROOT / "temp" / "nostale" / normalised,
        REPO_ROOT / "temp" / "downloads" / normalised,
    ]
    # Also look under repo root for CI runners that download elsewhere
    for p in candidates:
        if p.is_file():
            return p
    raise FileNotFoundError(
        f"Benchmark file not found: {manifest_path}\n"
        "Download it first with ensure_fixture or the download command."
    )


def _get_cli_version(cli: Path) -> str:
    try:
        r = subprocess.run([str(cli), "--version"], capture_output=True,
                           text=True, timeout=10)
        return r.stdout.strip() or r.stderr.strip() or "unknown"
    except Exception:
        return "unknown"


def _get_cli_help(cli: Path) -> Optional[str]:
    """Return the help text describing cli features, or None."""
    try:
        r = subprocess.run([str(cli), "--help"], capture_output=True,
                           text=True, timeout=10)
        lines = r.stdout.strip().splitlines()
        if lines:
            return lines[0]
        return None
    except Exception:
        return None


def _platform_info() -> Dict[str, Any]:
    """Gather system information for the report header."""
    info: Dict[str, Any] = {}

    # OS
    try:
        import distro  # type: ignore[import-untyped]
        os_str = f"{distro.name(pretty=True)}"
    except ImportError:
        os_str = f"{platform.system()} {platform.release()}"
    info["os"] = os_str

    # Kernel / version string
    if platform.system() == "Linux":
        try:
            with open("/proc/version") as f:
                raw = f.read().strip()
                info["kernel"] = raw.split(" (")[0] if "(" in raw else raw
        except OSError:
            pass
    info["platform"] = platform.platform(terse=True)

    # CPU
    cpu_name = None
    if platform.system() == "Linux":
        try:
            with open("/proc/cpuinfo") as f:
                for line in f:
                    if line.startswith("model name"):
                        cpu_name = line.split(":", 1)[1].strip()
                        break
        except OSError:
            pass
    elif platform.system() == "Darwin":
        try:
            r = subprocess.run(["sysctl", "-n", "machdep.cpu.brand_string"],
                               capture_output=True, text=True, timeout=5)
            if r.returncode == 0:
                cpu_name = r.stdout.strip()
        except Exception:
            pass
    elif platform.system() == "Windows":
        try:
            r = subprocess.run(
                ["wmic", "cpu", "get", "name"],
                capture_output=True, text=True, timeout=5
            )
            if r.returncode == 0:
                lines = [l.strip() for l in r.stdout.splitlines() if l.strip()]
                if len(lines) > 1:
                    cpu_name = lines[1]
        except Exception:
            pass
    info["cpu"] = cpu_name or "unknown"
    info["cpu_count"] = os.cpu_count() or 0

    # RAM total
    mem_total = None
    if platform.system() == "Linux":
        try:
            with open("/proc/meminfo") as f:
                for line in f:
                    if line.startswith("MemTotal:"):
                        kb = int(line.split()[1])
                        mem_total = kb // 1024
                        break
        except OSError:
            pass
    elif platform.system() == "Darwin":
        try:
            r = subprocess.run(["sysctl", "-n", "hw.memsize"],
                               capture_output=True, text=True, timeout=5)
            if r.returncode == 0:
                mem_total = int(r.stdout.strip()) // (1024 * 1024)
        except Exception:
            pass
    elif platform.system() == "Windows":
        try:
            r = subprocess.run(
                ["wmic", "memorychip", "get", "Capacity"],
                capture_output=True, text=True, timeout=5
            )
            if r.returncode == 0:
                capacities = []
                for line in r.stdout.splitlines():
                    line = line.strip()
                    if line.isdigit():
                        capacities.append(int(line))
                if capacities:
                    mem_total = sum(capacities) // (1024 * 1024 * 1024)
        except Exception:
            pass
    if mem_total is not None:
        info["ram_mb"] = mem_total
        if mem_total >= 1024:
            info["ram"] = f"{mem_total / 1024:.1f} GB"
        else:
            info["ram"] = f"{mem_total} MB"

    return info


MEM_PREFIX = "__MEM__"


def _can_use_gnu_time() -> bool:
    """Check if GNU time (with --format) is available."""
    try:
        r = subprocess.run(["/usr/bin/time", "--version"],
                           capture_output=True, timeout=5)
        # GNU time outputs "GNU time" in --version
        return b"GNU" in r.stdout or b"GNU" in r.stderr
    except (FileNotFoundError, subprocess.TimeoutExpired):
        return False


def _run_cli(cli: Path, operation: str, nos_path: Path,
             output_dir: Optional[Path] = None,
             measure_memory: bool = False) -> Tuple[subprocess.CompletedProcess, Optional[int]]:
    """Run a single CLI operation.

    Returns (completed_process, memory_kb_or_None).
    When *measure_memory* is True and GNU time is available, memory_kb
    contains the peak RSS of the child process.
    """
    args = []
    mem_prefix = None

    if measure_memory and _can_use_gnu_time():
        mem_prefix = MEM_PREFIX
        args = ["/usr/bin/time", "--format", mem_prefix + "%M"]
    elif measure_memory:
        # getrusage fallback handled by the caller
        pass

    args.append(str(cli))

    if operation == "list":
        args.extend(["list", str(nos_path)])
    elif operation == "info --json":
        args.extend(["info", "--json", str(nos_path)])
    elif operation == "extract":
        if output_dir is None:
            raise ValueError("output_dir required for extract operations")
        args.extend(["extract", str(nos_path), "-o", str(output_dir)])
    else:
        raise ValueError(f"Unknown operation: {operation}")

    proc = subprocess.run(
        args, capture_output=True, text=True, timeout=600
    )

    # Parse memory from GNU time output (always on stderr)
    memory_kb = None
    if mem_prefix is not None:
        for line in proc.stderr.splitlines():
            if line.startswith(mem_prefix):
                digits = line[len(mem_prefix):].strip()
                if digits.isdigit():
                    memory_kb = int(digits)
                break

    return proc, memory_kb


def _get_memory_kb() -> Optional[int]:
    """Return peak RSS (KB) of terminated child processes, or None on
    platforms that don't support getrusage (e.g. Windows)."""
    try:
        import resource
        return resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss
    except (ImportError, AttributeError):
        return None


# ---------------------------------------------------------------------------
# Benchmark measurement
# ---------------------------------------------------------------------------

@dataclass
class Measurement:
    elapsed_ms: float
    memory_kb: Optional[int]
    success: bool
    exit_code: int


@dataclass
class Stats:
    min_ms: float
    max_ms: float
    mean_ms: float
    median_ms: float
    stdev_ms: float
    samples: int
    successes: int
    memory_kb: Optional[int]


def compute_stats(times_ms: List[float],
                  successes: int,
                  memory_kb: Optional[int] = None) -> Stats:
    """Compute statistics from a list of measured times."""
    n = len(times_ms)
    if n < 2:
        stdev = 0.0
    else:
        stdev = statistics.stdev(times_ms)
    return Stats(
        min_ms=min(times_ms),
        max_ms=max(times_ms),
        mean_ms=statistics.mean(times_ms),
        median_ms=statistics.median(times_ms),
        stdev_ms=stdev,
        samples=n,
        successes=successes,
        memory_kb=memory_kb,
    )


def run_benchmark(cli: Path, files: List[Tuple[str, str, int]],
                  operations: List[str],
                  iterations: int, warmup: int,
                  temp_dir: Path) -> Dict[str, Dict[str, Stats]]:
    """Run the full benchmark suite.

    Returns a nested dict: result[file_label][operation] = Stats
    """
    exists_cache: Dict[str, Path] = {}

    def _resolve(name: str) -> Path:
        if name not in exists_cache:
            exists_cache[name] = _resolve_nos_file(name)
        return exists_cache[name]

    results: Dict[str, Dict[str, Stats]] = {}

    for fmt_label, manifest_path, _size in files:
        sys.stderr.write(f"\n  {fmt_label}  ({manifest_path.split(chr(92))[-1]})")
        sys.stderr.flush()
        nos_path = _resolve(manifest_path)
        file_results: Dict[str, Stats] = {}

        for op in operations:
            sys.stderr.write(f"  · {op}")
            sys.stderr.flush()

            measurements: List[Measurement] = []
            output_dir = None
            if op == "extract":
                output_dir = temp_dir / f"extract_{fmt_label.replace(' ', '_').replace('(', '').replace(')', '')}"

            # Warmup
            for _ in range(warmup):
                if op == "extract" and output_dir is not None and output_dir.exists():
                    shutil.rmtree(output_dir)
                    output_dir.mkdir(parents=True, exist_ok=True)
                _run_cli(cli, op, nos_path, output_dir, measure_memory=False)

            # Measured runs
            for _ in range(iterations):
                if op == "extract" and output_dir is not None and output_dir.exists():
                    shutil.rmtree(output_dir)
                    output_dir.mkdir(parents=True, exist_ok=True)

                start = time.perf_counter()
                proc, mem = _run_cli(cli, op, nos_path, output_dir,
                                     measure_memory=True)
                elapsed = (time.perf_counter() - start) * 1000.0  # ms

                measurements.append(Measurement(
                    elapsed_ms=elapsed,
                    memory_kb=mem,
                    success=proc.returncode == 0,
                    exit_code=proc.returncode,
                ))

            times = [m.elapsed_ms for m in measurements]
            successes = sum(1 for m in measurements if m.success)
            # Peak memory across all measured runs (take max)
            all_mem = [m.memory_kb for m in measurements if m.memory_kb is not None]
            peak_mem = max(all_mem) if all_mem else None

            file_results[op] = compute_stats(times, successes, peak_mem)

            # Print quick result inline (to stderr so JSON output stays clean)
            s = file_results[op]
            sys.stderr.write(f"  min={s.min_ms:.1f}ms  median={s.median_ms:.1f}ms  max={s.max_ms:.1f}ms")
            if s.successes < iterations:
                sys.stderr.write(f"  ⚠ {iterations - s.successes} failures")
            sys.stderr.write("\n")
            sys.stderr.flush()

        results[fmt_label] = file_results

    return results


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def _fmt_ms(ms: float) -> str:
    """Format milliseconds human-readably."""
    if ms >= 1000:
        return f"{ms / 1000:.2f}s"
    return f"{ms:.1f}ms"


def _file_label(label: str, filename: str, size: int) -> str:
    """Short label: 'NS4BbData.NOS (80 MB)'"""
    if size >= 1_000_000:
        s = f"{size / 1_000_000:.0f} MB" if size < 100_000_000 else f"{size / 1_000_000:.0f} MB"
    elif size >= 1_000:
        s = f"{size / 1_000:.0f} KB"
    else:
        s = f"{size} B"
    return f"{filename} ({s})"


def print_report(results: Dict[str, Dict[str, Stats]],
                 platform_info: Dict[str, Any],
                 cli_version: str,
                 iterations: int, warmup: int,
                 files: Optional[List[Tuple[str, str, int]]] = None) -> None:
    """Print a formatted benchmark report to stdout."""

    # Build file info lookup
    file_info: Dict[str, Tuple[str, int]] = {}
    if files:
        for label, mpath, sz in files:
            fname = mpath.replace("\\", "/").split("/")[-1]
            file_info[label] = (fname, sz)

    # Header
    print()
    print("═" * 72)
    print("  OnexExplorerCli — Benchmark Report")
    print("═" * 72)
    print()

    # System info
    print("  System")
    print("  ──────")
    print(f"    OS:        {platform_info.get('os', 'N/A')}")
    if "kernel" in platform_info:
        print(f"    Kernel:    {platform_info['kernel']}")
    print(f"    CPU:       {platform_info.get('cpu', 'N/A')}")
    print(f"    Cores:     {platform_info.get('cpu_count', 'N/A')}")
    print(f"    RAM:       {platform_info.get('ram', 'N/A')}")
    print()

    # CLI info
    print("  Tool")
    print("  ────")
    print(f"    Version:   {cli_version}")
    print(f"    Iterations: {iterations}  |  Warmup: {warmup}")
    print()

    # Results table
    print("  Results")
    print("  ───────")
    header = f"{'File':<30} {'Operation':<14} {'Min':>10} {'Median':>10} {'Max':>10} {'σ':>10}  {'Memory':>10}"
    print(f"  {header}")
    print(f"  {'─' * (len(header) + 2)}")

    for fmt_label, ops in results.items():
        fname, fsize = file_info.get(fmt_label, (fmt_label, 0))
        short = _file_label(fmt_label, fname, fsize)

        for op_idx, (op, stats) in enumerate(ops.items()):
            label = short if op_idx == 0 else ""
            mem_str = ""
            if stats.memory_kb is not None:
                mem_str = f"{stats.memory_kb:>6} KB"
            else:
                mem_str = "      N/A"
            print(f"  {label:<30} {op:<14} {_fmt_ms(stats.min_ms):>10} "
                  f"{_fmt_ms(stats.median_ms):>10} {_fmt_ms(stats.max_ms):>10} "
                  f"{stats.stdev_ms:>8.1f}ms  {mem_str}")
            if stats.successes < stats.samples:
                print(f"  {'':<30} {'':<14} {'':>10} {'⚠ failed':>10} "
                      f"{stats.samples - stats.successes}/{stats.samples}")
        # Spacer between format groups
        print()

    # Footer
    print(f"  Measured {sum(len(ops) for ops in results.values())} operations × "
          f"{iterations} iterations = "
          f"{sum(len(ops) for ops in results.values()) * iterations} total runs")
    if warmup:
        print(f"  + {sum(len(ops) for ops in results.values()) * warmup} warmup runs")
    print()


def build_json_report(results: Dict[str, Dict[str, Stats]],
                      platform_info: Dict[str, Any],
                      cli_version: str,
                      args: argparse.Namespace,
                      files: Optional[List[Tuple[str, str, int]]] = None) -> dict:
    """Build a JSON-serializable report dictionary."""
    def _stats_to_dict(s: Stats) -> dict:
        return {
            "min_ms": round(s.min_ms, 2),
            "max_ms": round(s.max_ms, 2),
            "mean_ms": round(s.mean_ms, 2),
            "median_ms": round(s.median_ms, 2),
            "stdev_ms": round(s.stdev_ms, 2),
            "samples": s.samples,
            "successes": s.successes,
            "memory_kb": s.memory_kb,
        }

    results_out: dict = {}
    if files:
        for label, mpath, sz in files:
            fname = mpath.replace("\\", "/").split("/")[-1]
            entry = {"file": fname, "size_bytes": sz}
            if label in results:
                entry["operations"] = {
                    op: _stats_to_dict(stats) for op, stats in results[label].items()
                }
            results_out[label] = entry
    else:
        # fallback: flat format like before
        results_out = {
            fmt: {op: _stats_to_dict(stats) for op, stats in ops.items()}
            for fmt, ops in results.items()
        }

    return {
        "benchmark": {
            "tool": "OnexExplorerCli",
            "cli_version": cli_version,
            "iterations": args.iterations,
            "warmup": args.warmup,
        },
        "system": platform_info,
        "results": results_out,
    }


# ---------------------------------------------------------------------------
# CLI entry-point
# ---------------------------------------------------------------------------

def parse_args(argv: Optional[List[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Benchmark OnexExplorerCli operations on archive files."
    )
    parser.add_argument(
        "--iterations", type=int, default=BENCH_ITERATIONS,
        help=f"Number of measured runs per operation (default: {BENCH_ITERATIONS})"
    )
    parser.add_argument(
        "--warmup", type=int, default=BENCH_WARMUP,
        help=f"Number of warmup runs per operation (default: {BENCH_WARMUP})"
    )
    parser.add_argument(
        "--json", action="store_true",
        help="Output results as JSON (implies --no-cleanup)"
    )
    parser.add_argument(
        "--no-cleanup", action="store_true",
        help="Keep temp files after benchmark"
    )
    return parser.parse_args(argv)


def main(argv: Optional[List[str]] = None) -> int:
    args = parse_args(argv)

    # Resolve CLI binary
    try:
        cli = _find_cli()
    except FileNotFoundError as e:
        print(f"❌ {e}", file=sys.stderr)
        return 1

    # Gather system / tool info
    info = _platform_info()
    cli_version = _get_cli_version(cli)

    # Print preamble (skip in JSON mode)
    if not args.json:
        print()
        print("  OnexExplorerCli Benchmark")
        print(f"  Binary: {cli}")
        print(f"  Files:  {len(BENCH_FILES)} ({', '.join(f[0] for f in BENCH_FILES)})")
        print(f"  Ops:    {', '.join(BENCH_OPERATIONS)}")
        print(f"  Runs:   {args.iterations} iterations"
              f"{' + ' + str(args.warmup) + ' warmup' if args.warmup else ''}")
        print()

    # Prepare temp directory
    temp_dir = TEMP_DIR
    temp_dir.mkdir(parents=True, exist_ok=True)
    # Clean any previous run data
    for item in temp_dir.iterdir():
        if item.is_dir():
            shutil.rmtree(item)

    # Run benchmarks
    try:
        results = run_benchmark(
            cli=cli,
            files=BENCH_FILES,
            operations=BENCH_OPERATIONS,
            iterations=args.iterations,
            warmup=args.warmup,
            temp_dir=temp_dir,
        )
    except FileNotFoundError as e:
        print(f"\n❌ {e}", file=sys.stderr)
        # Cleanup
        if not args.no_cleanup and temp_dir.exists():
            shutil.rmtree(temp_dir)
        return 1
    except subprocess.TimeoutExpired as e:
        print(f"\n❌ Timeout: {e}", file=sys.stderr)
        if not args.no_cleanup and temp_dir.exists():
            shutil.rmtree(temp_dir)
        return 1

    # Report
    if args.json:
        report = build_json_report(results, info, cli_version, args, files=BENCH_FILES)
        json.dump(report, sys.stdout, indent=2)
        print()
    else:
        print_report(results, info, cli_version, args.iterations, args.warmup,
                     files=BENCH_FILES)

    # Cleanup
    if not args.no_cleanup and temp_dir.exists():
        shutil.rmtree(temp_dir)

    return 0


if __name__ == "__main__":
    sys.exit(main())
