# OnexExplorerCli — Benchmark Suite

Benchmark performance of `OnexExplorerCli` on `.NOS` archive files across
multiple formats and operations.

## Quick Start

```bash
# Build the CLI first
cmake --build build/standalone -j$(nproc)

# Run benchmark (default: 5 iterations + 1 warmup per operation)
python3 benchmark/bench.py

# Quick test (1 iteration, no warmup)
python3 benchmark/bench.py --iterations 1 --warmup 0

# JSON output
python3 benchmark/bench.py --json
```

Files are auto-downloaded via `download --build-id latest` on first run.
Use `--download-only` to pre-fetch them:

```bash
python3 benchmark/bench.py --download-only
```

## Configuration

All settings in [`config.py`](config.py):

| Setting | Default | Description |
|---------|---------|-------------|
| `BENCH_ITERATIONS` | 5 | Measured runs per operation |
| `BENCH_WARMUP` | 1 | Pre-measurement runs to warm caches |
| `BENCH_TESTS` | — | List of `(name, manifest_path, [commands])` |

Add a new test by appending to `BENCH_TESTS`:

```python
("My Format", "NostaleData\\MyFile.NOS", ["list", "info --json", "extract"]),
```

## Architecture

```
benchmark/
  README.md       # this file
  config.py       # constants: files to test, iterations, paths
  runner.py       # download + CLI execution + measurement loop
  report.py       # console table + JSON output formatting
  bench.py        # CLI entrypoint (argparse + main)
  __init__.py     # makes benchmark/ a package
```

### `config.py`
Pure data — no logic. Defines which files to test, which operations to run,
iteration counts, and helper paths (`REPO_ROOT`, `TEMP_DIR`).

### `runner.py`
Heavy lifting: downloads missing files via `download` command, resolves local
`.NOS` paths, runs CLI operations with `/usr/bin/time` for memory measurement,
and computes aggregate statistics (min, max, median, stdev).

### `report.py`
Output formatting. `print_report()` builds a human-readable table with aligned
columns. `build_json_report()` produces a machine-readable JSON document.

### `bench.py`
Entry point. Parses CLI flags, dispatches to runner and reporter, handles
cleanup. Supports `--iterations`, `--warmup`, `--json`, `--no-cleanup`,
`--download-only`.

## Operations

Each test file runs a list of commands:

| Command | CLI Invocation | Measures |
|---------|---------------|----------|
| `list` | `list <file>` | Entry listing |
| `info --json` | `info --json <file>` | JSON metadata read |
| `extract` | `extract <file> -o <dir>` | Full extraction |

Commands are strings split with `shlex` — so `"info --json"` passes
`["info", "--json"]` to the CLI.

## Memory Measurement

On Linux the benchmark uses GNU `/usr/bin/time` to capture peak RSS (KB).
On macOS/Windows memory reporting is skipped (shows `N/A` in the table).

The GNU time check is cached — it runs once per process, not per invocation.

## Output

### Console

```
Name                                           Operation             Min     Median        Max          σ      Memory
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
32GBS | NS4BbData.NOS (86 MB)                  list                5.2ms      5.3ms      7.1ms        0.8ms  11.9 MB
                                               info --json         7.5ms      7.8ms      8.2ms        0.3ms  11.9 MB
                                               extract            32.91s     33.08s     33.20s        0.12s  50.3 MB
```

### JSON

```bash
python3 benchmark/bench.py --json > results.json
```

The JSON document contains the same data in a structured format with
`benchmark`, `system`, and `results` top-level keys.

## Notes

- **File sizes** are read from disk at runtime (`os.path.getsize`) — not hardcoded.
- **Extract temp files** go to `benchmark/temp/` and are cleaned up after each run
  (use `--no-cleanup` to inspect them).
- **σ** is sample standard deviation (denominator `n-1`). With 1 iteration it is
  always 0.0.
