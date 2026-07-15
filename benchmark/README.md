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

# Only download files (no benchmark)
python3 -m benchmark --download-only

# Keep temp files (don't clean extract dirs)
python3 -m benchmark --no-cleanup
```

Files are auto-downloaded via `download --build-id latest` on first run.
Use `--download-only` to pre-fetch them.

## Configuration

All settings in [`config.py`](config.py):

| Setting | Default | Description |
|---------|---------|-------------|
| `BENCH_ITERATIONS` | 5 | Measured runs per operation |
| `BENCH_WARMUP` | 1 | Pre-measurement runs to warm caches |
| `BENCH_TESTS` | — | List of `(name, manifest_path, [commands])` |

Add a new test by appending to `BENCH_TESTS`:

```python
("My Format", "NostaleData/MyFile.NOS", ["list", "info --json", "extract"]),
```

## Architecture

```
benchmark/
  README.md       # this file
  config.py       # constants: files to test, iterations, paths
  runner.py       # download + CLI execution + measurement loop
  report.py       # console table + JSON output formatting
  bench.py        # CLI entrypoint (argparse + main)
  __init__.py     # package marker
  __main__.py     # allows `python3 -m benchmark`
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
  ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  32GBS | NS4BbData.NOS (86 MB)                  list                4.8ms      4.9ms      5.6ms        0.3ms     11.8 MB
                                                 info --json         7.2ms      7.3ms     14.5ms        3.2ms     11.9 MB
                                                 extract            32.86s     32.89s     32.93s       29.6ms     50.4 MB

  NT Data | NStpData01.NOS (51 MB)               list                5.4ms      5.5ms      5.6ms        0.1ms     12.0 MB
                                                 info --json         9.8ms      9.8ms      9.8ms        0.0ms     12.0 MB
                                                 extract            10.92s     10.93s     10.94s        9.2ms     16.1 MB

  CCINF V1.20 | NSmnData.NOS (555 KB)            list               32.2ms     32.5ms     33.0ms        0.3ms     13.3 MB
                                                 info --json       556.9ms    559.2ms    565.8ms        3.6ms     38.7 MB
                                                 extract           733.3ms    753.3ms    768.2ms       14.8ms     13.2 MB

  Text | NSgtdData.NOS (18 MB)                   list                4.2ms      4.4ms      4.5ms        0.1ms     12.0 MB
                                                 info --json         5.2ms      5.2ms      5.4ms        0.1ms     11.9 MB
                                                 extract           515.0ms    517.2ms    520.6ms        2.4ms     19.8 MB

  Map | NSmpData04.NOS (172 MB)                  list               64.2ms     64.6ms     66.5ms        1.0ms     12.2 MB
                                                 info --json       110.5ms    111.3ms    111.6ms        0.4ms     13.9 MB
                                                 extract             5.41s      6.50s      7.16s      774.5ms     20.2 MB
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
