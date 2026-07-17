# Comparison: OnexExplorerCli vs taletool v0.1.0

Run: 2026-07-17  
Iterations: 5 per operation, 1 warmup  
OnexExplorerCli: `feature/cpp-benchmark` (fpng)  
taletool: [v0.1.0](https://github.com/imxeno/taletool/releases/tag/v0.1.0) (Linux x86_64)

## Summary

| File | Op | OnexExplorerCli | taletool | Winner |
|------|----|----------------|----------|--------|
| 32GBS (`NS4BbData.NOS`, 82 MB) | list | 4.2ms | 134.3ms | **Onex** (32×) |
|  | info | 4.0ms | 125.6ms | **Onex** (31×) |
|  | extract | 3176.4ms | 846.9ms | **taletool** (3.8×) |
| NT Data (`NStpData01.NOS`, 49 MB) | list | 4.3ms | 75.6ms | **Onex** (18×) |
|  | info | 4.2ms | 74.6ms | **Onex** (18×) |
|  | extract | 1087.5ms | 95.3ms | **taletool** (11×) |
| CCINF V1.20 (`NSmnData.NOS`, 541 KB) | list | 21.2ms | 1.2ms | **taletool** (17×) |
|  | info | 15.9ms | 1.2ms | **taletool** (14×) |
|  | extract | 429.8ms | 1.1ms | **taletool** (384×) |
| Text (`NSgtdData.NOS`, 18 MB) | list | 3.8ms | 29.2ms | **Onex** (7.7×) |
|  | info | 3.7ms | 28.7ms | **Onex** (7.7×) |
|  | extract | 18.8ms | 36.0ms | **Onex** (1.9×) |
| NT Data maps (`NSmpData04.NOS`, 164 MB) | list | 14.1ms | 249.4ms | **Onex** (18×) |
|  | info | 13.5ms | 249.0ms | **Onex** (18×) |
|  | extract | 456.3ms | 2039.8ms | **Onex** (4.5×) |

## Key takeaways

1. **metadata (list/info)**: OnexExplorerCli dominates on most files thanks to mmap + madvise. Exception: CCINF files where taletool handily beats us because we spend extra time on entry parsing. File-open times are similar.

2. **extract (image-heavy)**: taletool wins on the two image-dominant archives (`NS4BbData`, `NStpData01`) because it doesn't re-encode to PNG — it dumps raw records. OnexExplorerCli pays a ~4ms per image encode cost even with fpng.

3. **extract (text/data)**: OnexExplorerCli is faster on `NSgtdData` (text entries, no PNG overhead) and `NSmpData04` (very large file, mmap streaming helps).

4. **CCINF**: The massive 384× gap on extract shows OnexExplorerCli has an overhead on small entries — we create directories, write files, and track per-entry state. taletool handles this near-instantly.

5. **Overall**: OnexExplorerCli is competitive on mixed workloads and faster on text/data archives. Where taletool wins on image extracts, the gap would shrink if we added a raw dump mode (skip PNG encode). For the use case (extracting to PNG for viewing), the encode cost is inherent.

## Raw data

### OnexExplorerCli

| File | Op | min | median | max |
|------|----|----|--------|-----|
| 32GBS | list | 4.0ms | 4.2ms | 5.0ms |
| 32GBS | info | 4.0ms | 4.0ms | 5.0ms |
| 32GBS | extract | 3174.5ms | 3176.4ms | 3252.4ms |
| NT Data | list | 4.2ms | 4.3ms | 4.4ms |
| NT Data | info | 4.1ms | 4.2ms | 4.5ms |
| NT Data | extract | 1085.3ms | 1087.5ms | 1096.9ms |
| CCINF V1.20 | list | 21.0ms | 21.2ms | 21.6ms |
| CCINF V1.20 | info | 15.3ms | 15.9ms | 16.4ms |
| CCINF V1.20 | extract | 423.7ms | 429.8ms | 431.1ms |
| Text | list | 3.7ms | 3.8ms | 4.1ms |
| Text | info | 3.7ms | 3.7ms | 3.8ms |
| Text | extract | 17.2ms | 18.8ms | 19.5ms |
| NT Data maps | list | 13.5ms | 14.1ms | 14.6ms |
| NT Data maps | info | 13.2ms | 13.5ms | 14.1ms |
| NT Data maps | extract | 425.4ms | 456.3ms | 917.8ms |

### taletool v0.1.0

| File | Op | min | median | max |
|------|----|----|--------|-----|
| 32GBS | list | 124.1ms | 134.3ms | 227.7ms |
| 32GBS | info | 121.8ms | 125.6ms | 127.4ms |
| 32GBS | extract | 836.0ms | 846.9ms | 998.9ms |
| NT Data | list | 71.3ms | 75.6ms | 82.2ms |
| NT Data | info | 69.8ms | 74.6ms | 79.3ms |
| NT Data | extract | 91.7ms | 95.3ms | 96.8ms |
| CCINF V1.20 | list | 1.1ms | 1.2ms | 2.4ms |
| CCINF V1.20 | info | 1.1ms | 1.2ms | 1.2ms |
| CCINF V1.20 | extract | 1.1ms | 1.1ms | 1.2ms |
| Text | list | 28.7ms | 29.2ms | 29.7ms |
| Text | info | 28.2ms | 28.7ms | 30.1ms |
| Text | extract | 34.1ms | 36.0ms | 37.2ms |
| NT Data maps | list | 242.5ms | 249.4ms | 267.6ms |
| NT Data maps | info | 243.5ms | 249.0ms | 264.2ms |
| NT Data maps | extract | 1957.5ms | 2039.8ms | 2368.5ms |
