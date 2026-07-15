"""Benchmark runner — download, resolve files, run CLI, collect measurements."""
from __future__ import annotations

import functools
import os
import re
import shlex
import shutil
import statistics
import subprocess
import sys
import time
from pathlib import Path
from typing import Dict, List, Optional, Tuple

from config import BenchTest, REPO_ROOT, TEMP_DIR

# ---------------------------------------------------------------------------
# File resolution & download
# ---------------------------------------------------------------------------


def _filename(manifest_path: str) -> str:
    """Extract just the filename from a manifest path."""
    return Path(manifest_path).name


def _resolve_nos_file(manifest_path: str) -> Path:
    """Find a .NOS file in known data directories."""
    normalized = manifest_path.replace("\\", "/")
    for p in [
        REPO_ROOT / "temp" / "nostale" / normalized,
        REPO_ROOT / "temp" / "downloads" / normalized,
    ]:
        if p.is_file():
            return p
    raise FileNotFoundError(
        f"Benchmark file not found: {manifest_path}\n"
        "Run with --download first, or manually:\n"
        f"  {REPO_ROOT / 'build' / 'standalone' / 'OnexExplorerCli'} download --all"
    )


def ensure_downloaded(cli: Path, manifest_path: str) -> Path:
    """Download the file if not already present, then return its path."""
    try:
        return _resolve_nos_file(manifest_path)
    except FileNotFoundError:
        print(f"    downloading {manifest_path}…", file=sys.stderr, flush=True)
        name = _filename(manifest_path).replace(".NOS", "")
        out = REPO_ROOT / "temp" / "nostale" / "NostaleData"
        out.mkdir(parents=True, exist_ok=True)
        r = subprocess.run(
            [str(cli), "download", "--build-id", "latest",
             "-o", str(out), name],
            capture_output=True, text=True, timeout=300,
        )
        if r.returncode != 0:
            msg = r.stderr.strip() or r.stdout.strip() or "unknown error"
            raise RuntimeError(
                f"download failed for {manifest_path}: {msg}"
            )
        return _resolve_nos_file(manifest_path)


# ---------------------------------------------------------------------------
# CLI helpers (shared with bench.py)
# ---------------------------------------------------------------------------


def find_cli() -> Path:
    """Locate the built OnexExplorerCli binary."""
    if sys.platform == "win32":
        candidates = [
            REPO_ROOT / "build" / "standalone" / "Release" / "OnexExplorerCli.exe",
            REPO_ROOT / "build" / "standalone" / "OnexExplorerCli.exe",
            REPO_ROOT / "build" / "standalone" / "Debug" / "OnexExplorerCli.exe",
        ]
    else:
        candidates = [REPO_ROOT / "build" / "standalone" / "OnexExplorerCli"]
    for p in candidates:
        if p.is_file():
            return p
    raise FileNotFoundError(
        "OnexExplorerCli binary not found. Build it first:\n"
        f"  cmake --build {REPO_ROOT / 'build' / 'standalone'} -j$(nproc)"
    )


def cli_version(cli: Path) -> str:
    """Return the CLI version string."""
    try:
        r = subprocess.run([str(cli), "--version"],
                           capture_output=True, text=True, timeout=10)
        return r.stdout.strip() or r.stderr.strip() or "unknown"
    except (FileNotFoundError, subprocess.TimeoutExpired):
        return "unknown"


# ---------------------------------------------------------------------------
# CLI runner
# ---------------------------------------------------------------------------

MEM_PREFIX = "__MEM__"


@functools.cache
def _check_gnu_time() -> bool:
    """Return True if GNU /usr/bin/time is available."""
    try:
        r = subprocess.run(["/usr/bin/time", "--version"],
                           capture_output=True, timeout=5)
        return b"GNU" in (r.stdout + r.stderr)
    except (FileNotFoundError, subprocess.TimeoutExpired):
        return False


def _build_cli_args(cli: Path, command: str, nos_path: Path,
                    output_dir: Optional[Path] = None,
                    gnu_avail: bool = False) -> list[str]:
    """Build the argument list for a CLI invocation."""
    args: list[str] = []
    if gnu_avail:
        args += ["/usr/bin/time", "--format", f"{MEM_PREFIX}%M"]
    args += [str(cli)]
    args += shlex.split(command)
    args += [str(nos_path)]
    if command == "extract":
        if output_dir is None:
            raise ValueError("output_dir required for extract")
        args += ["-o", str(output_dir)]
    return args


def _parse_memory_kb(stderr: str, prefix: str) -> Optional[int]:
    """Parse peak RSS (KB) from GNU time stderr."""
    for line in stderr.splitlines():
        if line.startswith(prefix):
            digits = line[len(prefix):].strip()
            if digits.isdigit():
                return int(digits)
            break
    return None


def _run_cli(cli: Path, command: str, nos_path: Path,
             output_dir: Optional[Path] = None,
             gnu_avail: bool = False) -> Tuple[subprocess.CompletedProcess, Optional[int]]:
    """Run a single CLI operation.

    *command* is a string like "list", "info --json", or "extract".
    It is split on space to build the argument list.
    *gnu_avail* is True once GNU /usr/bin/time has been verified and
    the warning printed — if False, memory is never tracked.

    Note: timing via time.perf_counter() around subprocess.run()
    includes 1-5 ms of Python overhead per invocation. This is
    consistent across all operations and doesn't affect relative
    comparisons.
    """
    args = _build_cli_args(cli, command, nos_path, output_dir, gnu_avail)
    proc = subprocess.run(args, capture_output=True, text=True, timeout=600)
    memory_kb = _parse_memory_kb(proc.stderr, MEM_PREFIX) if gnu_avail else None
    return proc, memory_kb


# ---------------------------------------------------------------------------
# Main benchmark loop
# ---------------------------------------------------------------------------


def _prepare_extract_dir(name: str, temp_dir: Path) -> Path:
    """Create (or recreate) a clean extract output directory."""
    safe = re.sub(r'[^\w.-]+', '_', name).strip("_")
    d = temp_dir / f"extract_{safe}"
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True, exist_ok=True)
    return d


def _compute_stats(measurements: List[Tuple[float, Optional[int], bool]]) -> dict:
    """Compute aggregate statistics from measurement tuples."""
    times = [m[0] for m in measurements]
    successes = sum(1 for m in measurements if m[2])
    all_mem = [m[1] for m in measurements if m[1] is not None]
    peak_mem = max(all_mem, default=None)
    n = len(times)
    return {
        "min_ms": min(times),
        "max_ms": max(times),
        "mean_ms": statistics.mean(times),
        "median_ms": statistics.median(times),
        "stdev_ms": statistics.stdev(times) if n >= 2 else 0.0,
        "samples": n,
        "successes": successes,
        "memory_kb": peak_mem,
    }


def run_benchmark(cli: Path,
                  tests: List[BenchTest],
                  iterations: int,
                  warmup: int,
                  temp_dir: Path) -> Dict[str, dict]:
    """Run the full benchmark suite.

    Returns a nested dict:
        result[test_name] = {
            "file": "NS4BbData.NOS",
            "size": 80299180,
            "operations": {
                "list": { stats_dict },
                ...
            }
        }
    """
    # One-time check: warn once, pass flag downstream so _run_cli
    # never needs to call _check_gnu_time() itself.
    can_measure = _check_gnu_time()
    if not can_measure:
        print("[WARN] GNU /usr/bin/time not found — memory tracking disabled",
              file=sys.stderr)

    results: Dict[str, dict] = {}

    for name, manifest_path, commands in tests:
        nos_path = ensure_downloaded(cli, manifest_path)
        filename = _filename(manifest_path)
        file_size = os.path.getsize(nos_path)

        print(f"\n  {name}  ({filename})", file=sys.stderr, flush=True)

        entry: dict = {"file": filename, "size": file_size, "operations": {}}

        for command in commands:
            print(f"  · {command}", end="", file=sys.stderr, flush=True)

            measurements: List[Tuple[float, Optional[int], bool]] = []
            out_dir = _prepare_extract_dir(name, temp_dir) if command == "extract" else None

            # Warmup (no memory measurement)
            for _ in range(warmup):
                _run_cli(cli, command, nos_path, out_dir, gnu_avail=False)

            # Measured runs
            for _ in range(iterations):
                start = time.perf_counter()
                proc, mem = _run_cli(cli, command, nos_path, out_dir,
                                     gnu_avail=can_measure)
                elapsed = (time.perf_counter() - start) * 1000.0
                measurements.append((elapsed, mem, proc.returncode == 0))

            stats = _compute_stats(measurements)
            entry["operations"][command] = stats
            print(f"  min={stats['min_ms']:.1f}ms  "
                  f"median={stats['median_ms']:.1f}ms  "
                  f"max={stats['max_ms']:.1f}ms",
                  file=sys.stderr, flush=True)

        results[name] = entry

    return results
