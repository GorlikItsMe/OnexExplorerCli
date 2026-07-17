# Benchmark Results: fpng vs lodepng

fpng with `FPNG_ENCODE_SLOWER` (two-pass). Extract runs: 3 iterations.
lodepng baseline: 1 iteration (see BASELINE.md).

## Summary

| File | Phase | lodepng (before) | fpng (after 3 runs) | Speedup |
|------|-------|------------------|--------------------|---------|
| NS4BbData | encode | 11374ms | 1313ms ÷ 3 = 438ms | **26×** |
| NS4BbData | extract total | 12372ms | 3372ms ÷ 3 = 1124ms | **11×** |
| NStpData01 | encode | 3732ms | 380ms ÷ 3 = 127ms | **29×** |
| NStpData01 | extract total | 3907ms | 472ms ÷ 3 = 157ms | **25×** |

## Detail (3 extract iterations)

========================================
=== /home/user/benchmark/data/NostaleData/NS4BbData.NOS ===
========================================

--- open: /home/user/benchmark/data/NostaleData/NS4BbData.NOS ---
  header read                  cnt=     5  total=     0.01ms  avg=    0.003ms  min=    0.001ms  max=    0.008ms
  detect format                cnt=     5  total=     0.00ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  parse entry table            cnt=     5  total=     1.43ms  avg=    0.286ms  min=    0.205ms  max=    0.360ms
  mmap                         cnt=     5  total=     0.03ms  avg=    0.007ms  min=    0.005ms  max=    0.009ms
  madvise                      cnt=     5  total=     1.05ms  avg=    0.210ms  min=    0.027ms  max=    0.937ms
  open total                   cnt=     5  total=     2.59ms  avg=    0.518ms  min=    0.245ms  max=    1.330ms
--- extract: /home/user/benchmark/data/NostaleData/NS4BbData.NOS ---
  read_entry                   cnt=   318  total=  1671.16ms  avg=    5.255ms  min=    0.002ms  max=   61.784ms
  img: parse_image_plan        cnt=   318  total=     0.02ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  img: decode_pixels           cnt=   312  total=   293.81ms  avg=    0.942ms  min=    0.169ms  max=    9.273ms
  img: fpng::encode_image      cnt=   312  total=  1313.38ms  avg=    4.210ms  min=    0.821ms  max=   45.660ms
  img: decode_entry_to_png     cnt=   318  total=  1607.86ms  avg=    5.056ms  min=    0.000ms  max=   55.346ms
  file write                   cnt=   312  total=    84.22ms  avg=    0.270ms  min=    0.048ms  max=    2.714ms
  entry total                  cnt=   318  total=  3372.42ms  avg=   10.605ms  min=    0.003ms  max=  120.208ms
  total output size             76165593 bytes

========================================
=== /home/user/benchmark/data/NostaleData/NStpData01.NOS ===
========================================

--- open: /home/user/benchmark/data/NostaleData/NStpData01.NOS ---
  header read                  cnt=     5  total=     0.01ms  avg=    0.002ms  min=    0.001ms  max=    0.006ms
  detect format                cnt=     5  total=     0.00ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  parse entry table            cnt=     5  total=     2.54ms  avg=    0.508ms  min=    0.370ms  max=    0.650ms
  mmap                         cnt=     5  total=     0.04ms  avg=    0.009ms  min=    0.005ms  max=    0.019ms
  madvise                      cnt=     5  total=     0.15ms  avg=    0.029ms  min=    0.028ms  max=    0.033ms
  open total                   cnt=     5  total=     2.79ms  avg=    0.558ms  min=    0.417ms  max=    0.694ms
--- extract: /home/user/benchmark/data/NostaleData/NStpData01.NOS ---
  read_entry                   cnt=   561  total=    46.81ms  avg=    0.083ms  min=    0.000ms  max=    4.321ms
  img: parse_image_plan        cnt=   561  total=     0.02ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  img: decode_pixels           cnt=   561  total=    26.31ms  avg=    0.047ms  min=    0.000ms  max=    0.428ms
  img: fpng::encode_image      cnt=   510  total=   379.94ms  avg=    0.745ms  min=    0.004ms  max=    2.433ms
  img: decode_entry_to_png     cnt=   561  total=   406.34ms  avg=    0.724ms  min=    0.000ms  max=    2.607ms
  file write                   cnt=   510  total=    19.06ms  avg=    0.037ms  min=    0.012ms  max=    0.203ms
  entry total                  cnt=   561  total=   472.32ms  avg=    0.842ms  min=    0.001ms  max=    6.306ms
  total output size             15348210 bytes

========================================
=== /home/user/benchmark/data/NostaleData/NSmnData.NOS ===
========================================

--- open: /home/user/benchmark/data/NostaleData/NSmnData.NOS ---
  header read                  cnt=     5  total=     0.01ms  avg=    0.002ms  min=    0.001ms  max=    0.004ms
  detect format                cnt=     5  total=     0.00ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  parse entry table            cnt=     5  total=    54.64ms  avg=   10.928ms  min=   10.708ms  max=   11.099ms
  mmap                         cnt=     5  total=     0.08ms  avg=    0.015ms  min=    0.007ms  max=    0.027ms
  madvise                      cnt=     5  total=     0.03ms  avg=    0.007ms  min=    0.005ms  max=    0.009ms
  open total                   cnt=     5  total=    55.40ms  avg=   11.081ms  min=   10.790ms  max=   11.278ms
--- extract: /home/user/benchmark/data/NostaleData/NSmnData.NOS ---
  read_entry                   cnt= 70296  total=     6.63ms  avg=    0.000ms  min=    0.000ms  max=    0.055ms
  file write                   cnt= 70296  total=   754.44ms  avg=    0.011ms  min=    0.010ms  max=    0.295ms
  entry total                  cnt= 70296  total=   766.49ms  avg=    0.011ms  min=    0.010ms  max=    0.295ms
  total output size             555412 bytes

========================================
=== /home/user/benchmark/data/NostaleData/NSgtdData.NOS ===
========================================

--- open: /home/user/benchmark/data/NostaleData/NSgtdData.NOS ---
  header read                  cnt=     5  total=     0.01ms  avg=    0.002ms  min=    0.001ms  max=    0.004ms
  detect format                cnt=     5  total=     0.00ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  parse entry table            cnt=     5  total=     0.24ms  avg=    0.048ms  min=    0.039ms  max=    0.066ms
  mmap                         cnt=     5  total=     0.03ms  avg=    0.007ms  min=    0.005ms  max=    0.012ms
  madvise                      cnt=     5  total=     0.88ms  avg=    0.177ms  min=    0.027ms  max=    0.777ms
  open total                   cnt=     5  total=     1.21ms  avg=    0.242ms  min=    0.078ms  max=    0.863ms
--- extract: /home/user/benchmark/data/NostaleData/NSgtdData.NOS ---
  read_entry                   cnt=   114  total=    62.34ms  avg=    0.547ms  min=    0.000ms  max=    5.224ms
  file write                   cnt=   114  total=    14.14ms  avg=    0.124ms  min=    0.006ms  max=    0.729ms
  entry total                  cnt=   114  total=    76.49ms  avg=    0.671ms  min=    0.006ms  max=    5.911ms
  total output size             18060291 bytes

========================================
=== /home/user/benchmark/data/NostaleData/NSmpData04.NOS ===
========================================

--- open: /home/user/benchmark/data/NostaleData/NSmpData04.NOS ---
  header read                  cnt=     5  total=     0.01ms  avg=    0.002ms  min=    0.001ms  max=    0.004ms
  detect format                cnt=     5  total=     0.00ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  parse entry table            cnt=     5  total=    45.46ms  avg=    9.093ms  min=    8.692ms  max=    9.316ms
  mmap                         cnt=     5  total=     0.05ms  avg=    0.011ms  min=    0.007ms  max=    0.015ms
  madvise                      cnt=     5  total=     0.15ms  avg=    0.029ms  min=    0.028ms  max=    0.030ms
  open total                   cnt=     5  total=    45.73ms  avg=    9.146ms  min=    8.740ms  max=    9.377ms
--- extract: /home/user/benchmark/data/NostaleData/NSmpData04.NOS ---
  read_entry                   cnt=  6144  total=  3569.28ms  avg=    0.581ms  min=    0.007ms  max=    8.747ms
  file write                   cnt=  6144  total=   596.71ms  avg=    0.097ms  min=    0.011ms  max=    3.383ms
  entry total                  cnt=  6144  total=  4166.94ms  avg=    0.678ms  min=    0.019ms  max=   11.290ms
  total output size             654306006 bytes

Done.
