"""Configuration for OnexExplorerCli benchmark.

Each test entry: (display_name, manifest_path, list_of_commands)
Commands are strings like "list", "info --json", "extract".
"""
from __future__ import annotations

from pathlib import Path
from typing import List, Tuple

BENCH_ITERATIONS = 5
BENCH_WARMUP = 1

# (name,  manifest_path,                     commands)
BENCH_TESTS: List[Tuple[str, str, List[str]]] = [
    ("32GBS",       "NostaleData\\NS4BbData.NOS",   ["list", "info --json", "extract"]),
    ("NT Data",     "NostaleData\\NStpData01.NOS",  ["list", "info --json", "extract"]),
    ("CCINF V1.20", "NostaleData\\NSmnData.NOS",    ["list", "info --json", "extract"]),
    ("Text",        "NostaleData\\NSgtdData.NOS",    ["list", "info --json", "extract"]),
]

_SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = _SCRIPT_DIR.parent
TEMP_DIR = _SCRIPT_DIR / "temp"
