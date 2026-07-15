"""Configuration for OnexExplorerCli benchmark.

Each test entry: (display_name, manifest_path, list_of_commands)
Commands are strings like "list", "info --json", "extract".
Manifest paths use forward slashes (work on both Linux and Windows).
"""

from __future__ import annotations

from pathlib import Path
from typing import Dict, List, Optional, Tuple, TypedDict

# ---------------------------------------------------------------------------
# Types
# ---------------------------------------------------------------------------

BenchTest = Tuple[str, str, List[str]]


class OpStats(TypedDict):
    """Statistics for one operation on one file."""
    min_ms: float
    max_ms: float
    mean_ms: float
    median_ms: float
    stdev_ms: float
    samples: int
    successes: int
    memory_kb: Optional[int]


class BenchEntry(TypedDict):
    """Result entry for one benchmarked file."""
    file: str
    size: int
    operations: Dict[str, OpStats]


# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------

BENCH_ITERATIONS = 5
BENCH_WARMUP = 1

# (name,          manifest_path,                    commands)
BENCH_TESTS: List[BenchTest] = [
    ("32GBS",       "NostaleData/NS4BbData.NOS",    ["list", "info --json", "extract"]),
    ("NT Data",     "NostaleData/NStpData01.NOS",   ["list", "info --json", "extract"]),
    ("CCINF V1.20", "NostaleData/NSmnData.NOS",     ["list", "info --json", "extract"]),
    ("Text",        "NostaleData/NSgtdData.NOS",     ["list", "info --json", "extract"]),
    ("Map",         "NostaleData/NSmpData04.NOS",   ["list", "info --json", "extract"]),
]

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------

_SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = _SCRIPT_DIR.parent
TEMP_DIR = _SCRIPT_DIR / "temp"
