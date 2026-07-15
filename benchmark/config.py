"""Benchmark configuration — files to test, iterations, paths."""
from __future__ import annotations

from pathlib import Path
from typing import List, Tuple

BENCH_ITERATIONS = 5
BENCH_WARMUP = 1

# (display_label, manifest_path, approximate_size_bytes)
BENCH_FILES: List[Tuple[str, str, int]] = [
    ("Zlib (32GBS)",  "NostaleData\\NS4BbData.NOS",   80_299_180),
    ("Zlib (NT Data)", "NostaleData\\NStpData01.NOS",  50_329_132),
    ("CCINF V1.20",   "NostaleData\\NSmnData.NOS",       552_054),
    ("Text",          "NostaleData\\NSgtdData.NOS",    18_105_099),
]

# Operations to run. The short name is shown in the report table.
BENCH_OPERATIONS: List[str] = ["list", "info", "extract"]
OP_DISPLAY: dict = {"list": "list", "info": "info --json", "extract": "extract"}

_SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = _SCRIPT_DIR.parent
TEMP_DIR = _SCRIPT_DIR / "temp"

# Short labels used in the Name column.
LABEL_SHORT: dict = {
    "Zlib (32GBS)": "32GBS",
    "Zlib (NT Data)": "NT Data",
    "CCINF V1.20": "CCINF V1.20",
    "Text": "Text",
}
