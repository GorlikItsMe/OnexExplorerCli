"""Benchmark entry point for `python3 -m benchmark`."""
import sys
from pathlib import Path

# Add benchmark/ dir to path so sibling modules resolve correctly
sys.path.insert(0, str(Path(__file__).resolve().parent))

from bench import main  # noqa: E402

sys.exit(main())
