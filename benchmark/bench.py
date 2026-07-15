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

from config import BENCH_ITERATIONS, BENCH_TESTS, BENCH_WARMUP, REPO_ROOT, TEMP_DIR
from report import ReportConfig, build_json_report, print_report
from runner import cli_version, ensure_downloaded, find_cli, run_benchmark

# ---------------------------------------------------------------------------
# Platform info
# ---------------------------------------------------------------------------


def _platform_info() -> Dict[str, Any]:
    """Gather system information for the report header."""
    info: Dict[str, Any] = {}

    try:
        import distro
        info["os"] = f"{distro.name(pretty=True)}"
    except ImportError:
        info["os"] = f"{platform.system()} {platform.release()}"

    if platform.system() == "Linux":
        info["kernel"] = platform.uname().release
    info["platform"] = platform.platform(terse=True)

    cpu: Optional[str] = None
    if platform.system() == "Linux":
        try:
            with open("/proc/cpuinfo") as f:
                for line in f:
                    if line.startswith("model name"):
                        cpu = line.split(":", 1)[1].strip()
                        break
        except OSError:
            pass
    elif platform.system() == "Darwin":
        try:
            r = subprocess.run(["sysctl", "-n", "machdep.cpu.brand_string"],
                               capture_output=True, text=True, timeout=5)
            if r.returncode == 0:
                cpu = r.stdout.strip()
        except Exception:
            pass
    elif platform.system() == "Windows":
        try:
            r = subprocess.run(["wmic", "cpu", "get", "name"],
                               capture_output=True, text=True, timeout=5)
            if r.returncode == 0:
                lines = [l.strip() for l in r.stdout.splitlines() if l.strip()]
                if len(lines) > 1:
                    cpu = lines[1]
        except Exception:
            pass
    info["cpu"] = cpu or "unknown"
    info["cpu_count"] = os.cpu_count() or 0

    mem: Optional[int] = None
    if platform.system() == "Linux":
        try:
            with open("/proc/meminfo") as f:
                for line in f:
                    if line.startswith("MemTotal:"):
                        mem = int(line.split()[1]) // 1024
                        break
        except OSError:
            pass
    elif platform.system() == "Darwin":
        try:
            r = subprocess.run(["sysctl", "-n", "hw.memsize"],
                               capture_output=True, text=True, timeout=5)
            if r.returncode == 0:
                mem = int(r.stdout.strip()) // (1024 * 1024)
        except Exception:
            pass
    elif platform.system() == "Windows":
        try:
            r = subprocess.run(["wmic", "memorychip", "get", "Capacity"],
                               capture_output=True, text=True, timeout=5)
            if r.returncode == 0:
                caps = [int(l.strip()) for l in r.stdout.splitlines()
                        if l.strip().isdigit()]
                if caps:
                    mem = sum(caps) // (1024 * 1024 * 1024)
        except Exception:
            pass

    if mem is not None:
        info["ram_mb"] = mem
        info["ram"] = f"{mem / 1024:.1f} GB" if mem >= 1024 else f"{mem} MB"
    return info


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Benchmark OnexExplorerCli operations on archive files."
    )
    parser.add_argument("--iterations", type=int, default=BENCH_ITERATIONS,
                        help=f"Measured runs per operation (default: {BENCH_ITERATIONS})")
    parser.add_argument("--warmup", type=int, default=BENCH_WARMUP,
                        help=f"Warmup runs per operation (default: {BENCH_WARMUP})")
    parser.add_argument("--json", action="store_true",
                        help="Output results as JSON")
    parser.add_argument("--no-cleanup", action="store_true",
                        help="Keep temp files after benchmark")
    parser.add_argument("--download-only", action="store_true",
                        help="Only download benchmark files, don't run")
    return parser.parse_args(argv)


def main(argv: Optional[list[str]] = None) -> int:
    args = parse_args(argv)

    try:
        cli = find_cli()
    except FileNotFoundError as e:
        print(f"[ERROR] {e}", file=sys.stderr)
        return 1

    info = _platform_info()
    version = cli_version(cli)

    # Unique manifest paths needed (used for download-only mode)
    all_manifest_paths = {mp for _, mp, _ in BENCH_TESTS}

    if not args.json:
        all_commands = {c for _, _, cmds in BENCH_TESTS for c in cmds}
        print()
        print("  OnexExplorerCli Benchmark")
        print(f"  Binary: {cli}")
        print(f"  Files:  {len(BENCH_TESTS)} ({', '.join(n for n, _, _ in BENCH_TESTS)})")
        print(f"  Ops:    {', '.join(all_commands)}")
        print(f"  Runs:   {args.iterations} iterations"
              f"{' + ' + str(args.warmup) + ' warmup' if args.warmup else ''}")
        print()

    # Prepare temp dir
    temp_dir = TEMP_DIR
    temp_dir.mkdir(parents=True, exist_ok=True)

    # Download-only mode
    if args.download_only:
        for mp in all_manifest_paths:
            ensure_downloaded(cli, mp)
        print("  All files downloaded.", file=sys.stderr)
        return 0

    # Run benchmarks (cleanup is guaranteed via try/finally)
    try:
        results = run_benchmark(
            cli=cli,
            tests=BENCH_TESTS,
            iterations=args.iterations,
            warmup=args.warmup,
            temp_dir=temp_dir,
        )
    except (FileNotFoundError, subprocess.TimeoutExpired, RuntimeError) as e:
        print(f"\n[ERROR] {e}", file=sys.stderr)
        return 1
    finally:
        if not args.no_cleanup and temp_dir.exists():
            shutil.rmtree(temp_dir)

    # Report
    cfg = ReportConfig(
        cli_version=version,
        iterations=args.iterations,
        warmup=args.warmup,
        platform_info=info,
    )
    if args.json:
        json.dump(build_json_report(results, cfg), sys.stdout, indent=2)
        print()
    else:
        print_report(results, cfg)

    return 0


if __name__ == "__main__":
    sys.exit(main())
