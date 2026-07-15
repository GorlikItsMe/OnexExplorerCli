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
from report import build_json_report, print_report
from runner import ensure_downloaded, run_benchmark

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _find_cli() -> Path:
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


def _cli_version(cli: Path) -> str:
    try:
        r = subprocess.run([str(cli), "--version"],
                           capture_output=True, text=True, timeout=10)
        return r.stdout.strip() or r.stderr.strip() or "unknown"
    except Exception:
        return "unknown"


def _platform_info() -> Dict[str, Any]:
    """Gather system information for the report header."""
    info: Dict[str, Any] = {}

    try:
        import distro
        info["os"] = f"{distro.name(pretty=True)}"
    except ImportError:
        info["os"] = f"{platform.system()} {platform.release()}"

    if platform.system() == "Linux":
        try:
            with open("/proc/version") as f:
                r = f.read().strip()
                info["kernel"] = r.split(" (")[0] if "(" in r else r
        except OSError:
            pass
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

def parse_args(argv: Optional[List[str]] = None) -> argparse.Namespace:
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


def main(argv: Optional[List[str]] = None) -> int:
    args = parse_args(argv)

    try:
        cli = _find_cli()
    except FileNotFoundError as e:
        print(f"❌ {e}", file=sys.stderr)
        return 1

    info = _platform_info()
    version = _cli_version(cli)

    # Gather which unique files are needed
    all_manifest_paths = dict.fromkeys(mp for _, mp, _ in BENCH_TESTS)
    all_commands = dict.fromkeys(c for _, _, cmds in BENCH_TESTS for c in cmds)

    if not args.json:
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
    for item in temp_dir.iterdir():
        if item.is_dir():
            shutil.rmtree(item)

    # Download-only mode
    if args.download_only:
        for mp in all_manifest_paths:
            ensure_downloaded(cli, mp)
        print("  All files downloaded.", file=sys.stderr)
        return 0

    # Run benchmarks
    try:
        results = run_benchmark(
            cli=cli,
            tests=BENCH_TESTS,
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
    except RuntimeError as e:
        print(f"\n❌ {e}", file=sys.stderr)
        if not args.no_cleanup and temp_dir.exists():
            shutil.rmtree(temp_dir)
        return 1

    # Report
    if args.json:
        report = build_json_report(results, info, version, args.iterations, args.warmup)
        json.dump(report, sys.stdout, indent=2)
        print()
    else:
        print_report(results, info, version, args.iterations, args.warmup)

    # Cleanup
    if not args.no_cleanup and temp_dir.exists():
        shutil.rmtree(temp_dir)

    return 0


if __name__ == "__main__":
    sys.exit(main())
