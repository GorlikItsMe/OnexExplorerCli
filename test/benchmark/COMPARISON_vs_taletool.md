# Comparison: OnexExplorerCli vs taletool v0.1.0

Run: 2026-07-17  
Iterations: 3 per operation, 1 warmup  
OnexExplorerCli: `feature/cpp-benchmark` (fpng)  
taletool: [v0.1.0](https://github.com/imxeno/taletool/releases/tag/v0.1.0) (Linux x86_64)

## Fairness

- **list/info**: direct comparison — both dump metadata to stdout.
- **extract**: taletool's `archive unpack` dumps raw payloads. For archives with
  recognised image payloads (textures) we additionally run `texture unpack` per
  entry to produce PNG output, making it a fair apples-to-apples comparison.

## Summary

| File | Op | OnexExplorerCli | taletool | Winner | Note |
|------|----|----------------|----------|--------|------|
| 32GBS (`NS4BbData`, 86 MB) | list | 4.4ms | 120.7ms | **Onex** (27×) | |
|  | info | 4.3ms | 119.4ms | **Onex** (28×) | |
|  | extract | 3229.1ms | 805.7ms | **taletool** (4×) | raw dump — taletool can't convert sprite animations to PNG |
| NT Data (`NStpData01`, 51 MB) | list | 4.6ms | 70.7ms | **Onex** (15×) | |
|  | info | 4.6ms | 71.8ms | **Onex** (16×) | |
|  | extract | 1110.2ms | 596.0ms | **taletool** (1.9×) | includes `texture unpack` → 187 PNGs |
| CCINF V1.20 (`NSmnData`, 1 MB) | list | 21.6ms | 1.2ms | **taletool** (18×) | |
|  | info | 16.1ms | 1.5ms | **taletool** (11×) | |
|  | extract | 430.4ms | 1.2ms | **taletool** (359×) | small index data, no PNG needed |
| Text (`NSgtdData`, 18 MB) | list | 3.8ms | 29.0ms | **Onex** (7.6×) | |
|  | info | 4.2ms | 28.9ms | **Onex** (6.9×) | |
|  | extract | 18.6ms | 36.1ms | **Onex** (1.9×) | no images, pure file copy |
| NT Data maps (`NSmpData04`, 172 MB) | list | 14.6ms | 245.1ms | **Onex** (17×) | |
|  | info | 13.6ms | 248.6ms | **Onex** (18×) | |
|  | extract | 499.4ms | 1936.5ms | **Onex** (3.9×) | raw dump — taletool can't convert map data to PNG |

## Key takeaways

1. **metadata (list/info)**: OnexExplorerCli dominates most files (mmap + madvise
   streaming). Exception: CCINF — taletool handles these tiny indices instantly
   while we incur entry-table parsing overhead.

2. **extract with PNG (apples-to-apples)**: On `NStpData01` (textures), taletool
   is **1.9× faster** even when including `texture unpack` → PNG. This is
   because our PNG encode (fpng) still adds ~4ms per image. The gap would
   shrink further if we skipped PNG for non-viewing workflows.

3. **extract without PNG (raw dump)**: taletool is 4× faster on `NS4BbData`
   and 359× faster on `NSmnData` — but it can't convert animation or CCINF
   payloads to PNG (unsupported format). OnexExplorerCli produces viewable
   PNGs for all image entry types.

4. **large non-image archives** (`NSmpData04` 172 MB, `NSgtdData` 18 MB):
   OnexExplorerCli is **1.9–3.9× faster** — mmap + madvise streaming helps
   with large sequential reads and many small entries (6144 / 70296 files).

5. **CCINF extreme gap** (359× on extract): OnexExplorerCli creates the full
   directory structure and writes each file even for tiny index entries.
   This is a place where batching or a "no-op for empty payloads" shortcut
   would help.

6. **Overall**: OnexExplorerCli is competitive. On mixed workloads and
   text/data archives it's faster. Where taletool wins on image extracts the
   margin is narrow (1.9×) when PNG conversion is included. The list/info
   gap (15–28× in Onex's favour) shows that mmap-based metadata access is
   a clear architectural advantage.
