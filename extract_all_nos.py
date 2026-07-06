#!/usr/bin/env python3
"""
Extract all .NOS files listed in nos_files.txt and record results.

Usage: python3 extract_all_nos.py [--resume]
"""

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
ONEX_CLI = ROOT / "build" / "standalone" / "OnexExplorerCli"
EXTRACT_BASE = ROOT / "temp" / "extracted"
RESULTS_FILE = ROOT / "temp" / "extract_results.json"
NOS_LIST_FILE = ROOT / "temp" / "nos_files.txt"


def run_extract(nos_file: Path, output_dir: Path, timeout: int = 120) -> subprocess.CompletedProcess:
    return subprocess.run(
        [str(ONEX_CLI), "extract", str(nos_file), "-o", str(output_dir)],
        capture_output=True,
        text=True,
        timeout=timeout,
        cwd=ROOT,
    )


def count_files(directory: Path) -> str:
    if not directory.is_dir():
        return "0"
    return str(sum(1 for path in directory.rglob("*") if path.is_file()))


def extract_nos(nos_file: Path) -> dict:
    basename = nos_file.stem
    output_dir = EXTRACT_BASE / basename

    if output_dir.exists():
        shutil.rmtree(output_dir)

    result = run_extract(nos_file, output_dir)
    output = result.stdout + result.stderr
    exit_code = result.returncode

    if exit_code == 0 and output_dir.is_dir():
        status = "OK"
    elif "not yet supported" in output or "Cannot open archive" in output:
        status = "UNSUPPORTED"
    elif exit_code != 0:
        status = "FAILED"
    else:
        status = "UNKNOWN"

    return {
        "file": basename,
        "path": str(nos_file.relative_to(ROOT)),
        "status": status,
        "exit_code": exit_code,
        "file_count": count_files(output_dir),
        "output": output[:300],
    }


def load_nos_files() -> list[Path]:
    if not NOS_LIST_FILE.is_file():
        print(f"Error: {NOS_LIST_FILE.relative_to(ROOT)} not found.", file=sys.stderr)
        sys.exit(1)

    base = NOS_LIST_FILE.parent
    paths = []
    for line in NOS_LIST_FILE.read_text().splitlines():
        line = line.strip()
        if line:
            paths.append(base / line)
    return paths


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Extract all .NOS files and record results.")
    parser.add_argument(
        "--resume",
        action="store_true",
        help="Skip files already present in the results file",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    if not ONEX_CLI.is_file():
        print(f"Error: {ONEX_CLI.relative_to(ROOT)} not found. Build the project first.", file=sys.stderr)
        sys.exit(1)

    EXTRACT_BASE.mkdir(parents=True, exist_ok=True)
    RESULTS_FILE.parent.mkdir(parents=True, exist_ok=True)

    nos_files = load_nos_files()
    print(f"Total NOS files to extract: {len(nos_files)}")

    existing_results: list[dict] = []
    if args.resume and RESULTS_FILE.is_file():
        existing_results = json.loads(RESULTS_FILE.read_text())
        done_files = {r["file"] for r in existing_results}
        nos_files = [path for path in nos_files if path.stem not in done_files]
        print(f"Resuming: {len(existing_results)} already done, {len(nos_files)} remaining")

    results = list(existing_results)
    total = len(existing_results) + len(nos_files)

    for i, nos_file in enumerate(nos_files):
        print(
            f"[{len(existing_results) + i + 1}/{total}] Extracting {nos_file.stem}...",
            end=" ",
            flush=True,
        )

        result = extract_nos(nos_file)
        results.append(result)
        RESULTS_FILE.write_text(json.dumps(results, indent=2))

        print(f"{result['status']} ({result['file_count']} files)")

    print("\n" + "=" * 60)
    print("FINAL SUMMARY")
    print("=" * 60)

    ok = [r for r in results if r["status"] == "OK"]
    unsupported = [r for r in results if r["status"] == "UNSUPPORTED"]
    failed = [r for r in results if r["status"] == "FAILED"]
    unknown = [r for r in results if r["status"] == "UNKNOWN"]

    print(f"Total: {len(results)}")
    print(f"  Successfully extracted: {len(ok)}")
    print(f"  Unsupported format:     {len(unsupported)}")
    print(f"  Failed:                   {len(failed)}")
    print(f"  Unknown:                  {len(unknown)}")

    if ok:
        print("\n--- Extracted OK ---")
        for r in sorted(ok, key=lambda x: x["file"]):
            print(f"  {r['file']}: {r['file_count']} files")

    if unsupported:
        print("\n--- Unsupported ---")
        for r in sorted(unsupported, key=lambda x: x["file"]):
            print(f"  {r['file']}")

    if failed:
        print("\n--- Failed ---")
        for r in sorted(failed, key=lambda x: x["file"]):
            print(f"  {r['file']}: {r['output'][:120]}")

    if unknown:
        print("\n--- Unknown ---")
        for r in sorted(unknown, key=lambda x: x["file"]):
            print(f"  {r['file']}: {r['output'][:120]}")


if __name__ == "__main__":
    main()
