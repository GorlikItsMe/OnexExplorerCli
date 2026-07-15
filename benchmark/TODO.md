# Benchmark system — plan implementacji

## Struktura plików

```
benchmark/
  bench.py        ← główny skrypt
  temp/           ← output extract (gitignored)
```

## Config (na górze skryptu)

```python
ITERATIONS = 5
WARMUP = 1
FILES = [
    ("Zlib (32GBS)", "NostaleData\\NS4BbData.NOS",   80_000_000),
    ("Zlib (NT Data)", "NostaleData\\NStpData01.NOS", 48_000_000),
    ("CCINF V1.20",    "NostaleData\\NSmnData.NOS",     540_000),
    ("Text",           "NostaleData\\NSgtdData.NOS",   18_000_000),
]
OPERATIONS = ["list", "info --json", "extract"]
```

## Kroki implementacji

### 1. `bench.py` — szkielet
- argparse: `--iterations`, `--warmup`, `--json`, `--no-cleanup`
- config constants na górze (łatwo zmienić)
- `main()`: `print_system_info()` → `run_benchmark()` → `print_report()`

### 2. Funkcje pomocnicze
- `find_cli()` — szuka binarki (Linux: `build/standalone/OnexExplorerCli`, Windows: dodaje `.exe`, albo `build/Release/...`)
- `resolve_file(name)` — znajduje plik NOS w `temp/nostale/NostaleData/` lub `downloads/NostaleData/`
- `run_command(cmd, *args)` → `(elapsed_ms, memory_kb)` — uruchamia CLI, mierzy czas
- `get_memory()` — `resource.getrusage().ru_maxrss` (Unix) / None (Windows)

### 3. Pętla benchmarkowa
```
for each file:
    for each operation:
        warmup (1 run, results discarded)
        for i in range(iterations):
            measure time + memory
            store result
```

### 4. Statystyki per (file, operation)
- min, max, median, mean, stdev (z `statistics`)

### 5. System info w raporcie
- OS: `platform.system() + platform.release()` (np. "Linux 5.15.0")
- CPU: z `/proc/cpuinfo` (Linux), `wmic` (Windows), `sysctl` (macOS)
- CPU count: `os.cpu_count()`
- RAM: z `/proc/meminfo` (Linux), `wmic` (Windows) — total
- CLI version: `--version`

### 6. Raport
- **Tabela w konsoli**: formatowana, z separatorami
- **JSON**: `--json` flag, output do stdout

### 7. .gitignore
- Dodać `benchmark/temp/`

### 8. Testowanie
- Uruchomić `python3 benchmark/bench.py`
- Sprawdzić: wszystkie 4 pliki, 3 operacje, statystyki
- Przetestować `--json`
