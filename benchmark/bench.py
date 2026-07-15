#!/usr/bin/env python3
"""Benchmark OnexExplorerCli operations on .NOS archives.

Usage:
    python3 -m benchmark
    python3 -m benchmark --iterations 3 --warmup 0
    python3 -m benchmark --json
"""
from __future__ import annotations

import argparse
import json
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional

from config import (BENCH_FILES, BENCH_ITERATIONS, BENCH_OPERATIONS,
                    BENCH_WARMUP, REPO_ROOT, TEMP_DIR, OP_DISPLAY)
from report import build_json_report, print_report
from runner import run_benchmark

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _find_cli() -> Path:
    """Locate the OnexExplorerCli binary."""
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


def _get_cli_version(cli: Path) -> str:
    try:
        r = subprocess.run([str(cli), "--version"],
                           capture_output=True, text=True, timeout=10)
        return r.stdout.strip() or r.stderr.strip() or "unknown"
    except Exception:
        return "unknown"


def _platform_info() -> Dict[str, Any]:
    """Gather system information for the report header."""
    info: Dict[str, Any] = {}

    # OS
    try:
        import distro  # type: ignore[import-untyped]
        info["os"] = f"{distro.name(pretty=True)}"
    except ImportError:
        info["os"] = f"{platform.system()} {platform.release()}"

    # Kernel
    if platform.system() == "Linux":
        try:
            with open("/proc/version") as f:
                raw = f.read().strip()
                info["kernel"] = raw.split(" (")[0] if "(" in raw else raw
        except OSError:
            pass
    info["platform"] = platform.platform(terse=True)

    # CPU
    cpu_name: Optional[str] = None
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
            r = subprocess.run(["wmic", "cpu", "get", "name"],
                               capture_output=True, text=True, timeout=5)
            if r.returncode == 0:
                lines = [l.strip() for l in r.stdout.splitlines() if l.strip()]
                if len(lines) > 1:
                    cpu_name = lines[1]
        except Exception:
            pass
    info["cpu"] = cpu_name or "unknown"
    info["cpu_count"] = os.cpu_count() or 0

    # RAM
    mem_total: Optional[int] = None
    if platform.system() == "Linux":
        try:
            with open("/proc/meminfo") as f:
                for line in f:
                    if line.startswith("MemTotal:"):
                        mem_total = int(line.split()[1]) // 1024
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
            r = subprocess.run(["wmic", "memorychip", "get", "Capacity"],
                               capture_output=True, text=True, timeout=5)
            if r.returncode == 0:
                capacities = [int(l.strip()) for l in r.stdout.splitlines()
                              if l.strip().isdigit()]
                if capacities:
                    mem_total = sum(capacities) // (1024 * 1024 * 1024)
        except Exception:
            pass

    if mem_total is not None:
        info["ram_mb"] = mem_total
        info["ram"] = f"{mem_total / 1024:.1f} GB" if mem_total >= 1024 else f"{mem_total} MB"

    return info


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args(argv: Optional[List[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Benchmark OnexExplorerCli operations on archive files."
    )
    parser.add_argument("--iterations", type=int, default=BENCH_ITERATIONS,
                        help=f"Measured runs per operation (default: {BENCH_ITERATIONS})")
    parser.add_argument("--warmup", type=int, default=BENCH_WARMUP,
                        help=f"Warmup runs per operation (default: {BENCH_WARMUP})")
    parser.add_argument("--json", action="store_true",
                        help="Output results as JSON (implies --no-cleanup)")
    parser.add_argument("--no-cleanup", action="store_true",
                        help="Keep temp files after benchmark")
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

    # Preamble
    if not args.json:
        ops_str = ", ".join(OP_DISPLAY[op] for op in BENCH_OPERATIONS)
        print()
        print(f"  OnexExplorerCli Benchmark")
        print(f"  Binary: {cli}")
        print(f"  Files:  {len(BENCH_FILES)} ({', '.join(f[0] for f in BENCH_FILES)})")
        print(f"  Ops:    {ops_str}")
        print(f"  Runs:   {args.iterations} iterations"
              f"{' + ' + str(args.warmup) + ' warmup' if args.warmup else ''}")
        print()

    # Prepare temp directory
    temp_dir = TEMP_DIR
    temp_dir.mkdir(parents=True, exist_ok=True)
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
        report = build_json_report(results, info, cli_version,
                                   args.iterations, args.warmup)
        json.dump(report, sys.stdout, indent=2)
        print()
    else:
        print_report(results, info, cli_version, args.iterations, args.warmup)

    # Cleanup
    if not args.no_cleanup and temp_dir.exists():
        shutil.rmtree(temp_dir)

    return 0


if __name__ == "__main__":
    sys.exit(main())
