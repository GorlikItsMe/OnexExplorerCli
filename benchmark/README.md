# OnexExplorerCli — Benchmark Suite

Benchmark performance of `OnexExplorerCli` on `.NOS` archive files across
multiple formats and operations.

## Quick Start

```bash
# Build the CLI first
cmake --build build/standalone -j$(nproc)

# Run benchmark (default: 5 iterations + 1 warmup per operation)
python3 -m benchmark
# or: python3 benchmark/bench.py

# Quick test (1 iteration, no warmup)
python3 -m benchmark --iterations 1 --warmup 0

# JSON output
python3 -m benchmark --json
```

Files are auto-downloaded via `OnexExplorerCli download --build-id latest` on first run.

## Configuration

All settings in [`config.py`](config.py):

| Setting | Default | Description |
|---------|---------|-------------|
| `BENCH_ITERATIONS` | 5 | Measured runs per operation |
| `BENCH_WARMUP` | 1 | Pre-measurement runs to warm caches |
| `BENCH_TESTS` | — | List of test cases |

## Memory Measurement

On Linux the benchmark uses GNU `/usr/bin/time` to capture peak RSS (KB).
On macOS/Windows memory reporting is skipped (shows `N/A` in the table).

## Notes

- **File sizes** are read from disk at runtime (`os.path.getsize`) — not hardcoded.
- **Extract temp files** go to `benchmark/temp/` and are cleaned up after each run.
- **σ** is sample standard deviation (denominator `n-1`). With 1 iteration it is
  always 0.0.
