# Benchmark Results: fpng vs lodepng

## Summary

| File | Phase | lodepng (before) | fpng (after) | Speedup |
|------|-------|-----------------|--------------|---------|
| NS4BbData | encode | 11374ms | 284ms | **40×** |
| NS4BbData | extract total | 12372ms | 879ms | **14×** |
| NStpData01 | encode | 3732ms | 90ms | **41×** |
| NStpData01 | extract total | 3907ms | 292ms | **13×** |
| NSmnData/NSgtdData/NSmpData04 | encode | N/A | N/A | unchanged |

## Detail

========================================
=== /home/user/benchmark/data/NostaleData/NS4BbData.NOS ===
========================================

--- open: /home/user/benchmark/data/NostaleData/NS4BbData.NOS ---
  header read                  cnt=     5  total=     0.01ms  avg=    0.002ms  min=    0.001ms  max=    0.006ms
  detect format                cnt=     5  total=     0.00ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  parse entry table            cnt=     5  total=     1.26ms  avg=    0.253ms  min=    0.202ms  max=    0.330ms
  mmap                         cnt=     5  total=     0.03ms  avg=    0.006ms  min=    0.005ms  max=    0.009ms
  madvise                      cnt=     5  total=     0.17ms  avg=    0.034ms  min=    0.027ms  max=    0.053ms
  open total                   cnt=     5  total=     1.52ms  avg=    0.304ms  min=    0.248ms  max=    0.394ms
--- extract: /home/user/benchmark/data/NostaleData/NS4BbData.NOS ---
  read_entry                   cnt=   106  total=   468.83ms  avg=    4.423ms  min=    0.003ms  max=   38.398ms
  img: parse_image_plan        cnt=   106  total=     0.00ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  img: decode_pixels           cnt=   104  total=    97.64ms  avg=    0.939ms  min=    0.169ms  max=    8.286ms
  img: fpng::encode_image      cnt=   104  total=   282.40ms  avg=    2.715ms  min=    0.588ms  max=   23.210ms
  img: decode_entry_to_png     cnt=   106  total=   380.69ms  avg=    3.591ms  min=    0.000ms  max=   31.966ms
  file write                   cnt=   104  total=    25.06ms  avg=    0.241ms  min=    0.049ms  max=    1.744ms
  entry total                  cnt=   106  total=   876.95ms  avg=    8.273ms  min=    0.003ms  max=   72.990ms

========================================
=== /home/user/benchmark/data/NostaleData/NStpData01.NOS ===
========================================

--- open: /home/user/benchmark/data/NostaleData/NStpData01.NOS ---
  header read                  cnt=     5  total=     0.01ms  avg=    0.002ms  min=    0.001ms  max=    0.005ms
  detect format                cnt=     5  total=     0.00ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  parse entry table            cnt=     5  total=     2.09ms  avg=    0.417ms  min=    0.354ms  max=    0.509ms
  mmap                         cnt=     5  total=     0.03ms  avg=    0.007ms  min=    0.005ms  max=    0.013ms
  madvise                      cnt=     5  total=     0.15ms  avg=    0.029ms  min=    0.027ms  max=    0.034ms
  open total                   cnt=     5  total=     2.34ms  avg=    0.467ms  min=    0.402ms  max=    0.568ms
--- extract: /home/user/benchmark/data/NostaleData/NStpData01.NOS ---
  read_entry                   cnt=   187  total=     6.34ms  avg=    0.034ms  min=    0.000ms  max=    0.165ms
  img: parse_image_plan        cnt=   187  total=     0.00ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  img: decode_pixels           cnt=   187  total=     8.44ms  avg=    0.045ms  min=    0.000ms  max=    0.164ms
  img: fpng::encode_image      cnt=   170  total=    90.50ms  avg=    0.532ms  min=    0.001ms  max=    1.645ms
  img: decode_entry_to_png     cnt=   187  total=    98.97ms  avg=    0.529ms  min=    0.000ms  max=    1.770ms
  file write                   cnt=   170  total=     8.07ms  avg=    0.047ms  min=    0.012ms  max=    0.194ms
  entry total                  cnt=   187  total=   113.41ms  avg=    0.606ms  min=    0.001ms  max=    1.962ms

========================================
=== /home/user/benchmark/data/NostaleData/NSmnData.NOS ===
========================================

--- open: /home/user/benchmark/data/NostaleData/NSmnData.NOS ---
  header read                  cnt=     5  total=     0.01ms  avg=    0.002ms  min=    0.001ms  max=    0.004ms
  detect format                cnt=     5  total=     0.00ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  parse entry table            cnt=     5  total=    55.11ms  avg=   11.023ms  min=   10.949ms  max=   11.227ms
  mmap                         cnt=     5  total=     0.04ms  avg=    0.008ms  min=    0.006ms  max=    0.014ms
  madvise                      cnt=     5  total=     0.03ms  avg=    0.006ms  min=    0.005ms  max=    0.007ms
  open total                   cnt=     5  total=    55.43ms  avg=   11.085ms  min=   11.000ms  max=   11.325ms
--- extract: /home/user/benchmark/data/NostaleData/NSmnData.NOS ---
  read_entry                   cnt= 23432  total=     1.68ms  avg=    0.000ms  min=    0.000ms  max=    0.004ms
  file write                   cnt= 23432  total=   256.46ms  avg=    0.011ms  min=    0.010ms  max=    0.098ms
  entry total                  cnt= 23432  total=   259.90ms  avg=    0.011ms  min=    0.010ms  max=    0.098ms

========================================
=== /home/user/benchmark/data/NostaleData/NSgtdData.NOS ===
========================================

--- open: /home/user/benchmark/data/NostaleData/NSgtdData.NOS ---
  header read                  cnt=     5  total=     0.01ms  avg=    0.002ms  min=    0.001ms  max=    0.004ms
  detect format                cnt=     5  total=     0.00ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  parse entry table            cnt=     5  total=     0.25ms  avg=    0.050ms  min=    0.040ms  max=    0.065ms
  mmap                         cnt=     5  total=     0.03ms  avg=    0.006ms  min=    0.005ms  max=    0.009ms
  madvise                      cnt=     5  total=     0.14ms  avg=    0.028ms  min=    0.027ms  max=    0.030ms
  open total                   cnt=     5  total=     0.47ms  avg=    0.095ms  min=    0.081ms  max=    0.117ms
--- extract: /home/user/benchmark/data/NostaleData/NSgtdData.NOS ---
  read_entry                   cnt=    38  total=    18.60ms  avg=    0.490ms  min=    0.000ms  max=    2.726ms
  file write                   cnt=    38  total=     4.87ms  avg=    0.128ms  min=    0.006ms  max=    0.861ms
  entry total                  cnt=    38  total=    23.48ms  avg=    0.618ms  min=    0.006ms  max=    3.588ms

========================================
=== /home/user/benchmark/data/NostaleData/NSmpData04.NOS ===
========================================

--- open: /home/user/benchmark/data/NostaleData/NSmpData04.NOS ---
  header read                  cnt=     5  total=     0.01ms  avg=    0.002ms  min=    0.001ms  max=    0.004ms
  detect format                cnt=     5  total=     0.00ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  parse entry table            cnt=     5  total=    44.39ms  avg=    8.877ms  min=    8.758ms  max=    9.026ms
  mmap                         cnt=     5  total=     0.04ms  avg=    0.009ms  min=    0.007ms  max=    0.013ms
  madvise                      cnt=     5  total=     0.15ms  avg=    0.030ms  min=    0.028ms  max=    0.032ms
  open total                   cnt=     5  total=    44.64ms  avg=    8.929ms  min=    8.805ms  max=    9.089ms
--- extract: /home/user/benchmark/data/NostaleData/NSmpData04.NOS ---
  read_entry                   cnt=  2048  total=  1027.42ms  avg=    0.502ms  min=    0.007ms  max=    4.857ms
  file write                   cnt=  2048  total=   170.84ms  avg=    0.083ms  min=    0.012ms  max=    0.724ms
  entry total                  cnt=  2048  total=  1198.51ms  avg=    0.585ms  min=    0.019ms  max=    5.426ms

Done.
