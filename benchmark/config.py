"""Configuration for OnexExplorerCli benchmark.

Each test entry: (display_name, manifest_path, list_of_commands)
Commands are strings like "list", "info --json", "extract".
Manifest paths use forward slashes (work on both Linux and Windows).
"""

from __future__ import annotations

from pathlib import Path

BENCH_ITERATIONS = 5
BENCH_WARMUP = 1

# (name,          manifest_path,                    commands)
BENCH_TESTS = [
    ("32GBS",       "NostaleData/NS4BbData.NOS",    ["list", "info --json", "extract"]),
    ("NT Data",     "NostaleData/NStpData01.NOS",   ["list", "info --json", "extract"]),
    ("CCINF V1.20", "NostaleData/NSmnData.NOS",     ["list", "info --json", "extract"]),
    ("Text",        "NostaleData/NSgtdData.NOS",     ["list", "info --json", "extract"]),
    ("Map",         "NostaleData/NSmpData04.NOS",   ["list", "info --json", "extract"]),
]

_SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = _SCRIPT_DIR.parent
TEMP_DIR = _SCRIPT_DIR / "temp"
