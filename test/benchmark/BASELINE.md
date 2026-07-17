# Baseline Benchmark Results

Run on: $(date)
Commit: $(git rev-parse HEAD)

## Command
./build/standalone/OnexExplorerBench \
  /home/user/benchmark/data/NostaleData/NS4BbData.NOS \
  /home/user/benchmark/data/NostaleData/NStpData01.NOS \
  /home/user/benchmark/data/NostaleData/NSmnData.NOS \
  /home/user/benchmark/data/NostaleData/NSgtdData.NOS \
  /home/user/benchmark/data/NostaleData/NSmpData04.NOS

## Results

========================================
=== /home/user/benchmark/data/NostaleData/NS4BbData.NOS ===
========================================

--- open: /home/user/benchmark/data/NostaleData/NS4BbData.NOS ---
  header read                  cnt=     5  total=     0.01ms  avg=    0.002ms  min=    0.001ms  max=    0.006ms
  detect format                cnt=     5  total=     0.00ms  avg=    0.001ms  min=    0.000ms  max=    0.003ms
  parse entry table            cnt=     5  total=     1.15ms  avg=    0.230ms  min=    0.195ms  max=    0.289ms
  mmap                         cnt=     5  total=     0.05ms  avg=    0.010ms  min=    0.005ms  max=    0.029ms
  madvise                      cnt=     5  total=     0.15ms  avg=    0.031ms  min=    0.027ms  max=    0.041ms
  open total                   cnt=     5  total=     1.41ms  avg=    0.282ms  min=    0.235ms  max=    0.353ms
--- extract: /home/user/benchmark/data/NostaleData/NS4BbData.NOS ---
  read_entry                   cnt=   106  total=   474.86ms  avg=    4.480ms  min=    0.003ms  max=   42.275ms
  img: parse_image_plan        cnt=   106  total=     0.01ms  avg=    0.000ms  min=    0.000ms  max=    0.001ms
  img: decode_pixels           cnt=   104  total=    95.30ms  avg=    0.916ms  min=    0.169ms  max=    5.801ms
  img: lodepng::encode         cnt=   104  total= 11398.87ms  avg=  109.605ms  min=   21.327ms  max=  594.305ms
  img: decode_entry_to_png     cnt=   106  total= 11494.38ms  avg=  108.438ms  min=    0.000ms  max=  600.108ms
  file write                   cnt=   104  total=    21.75ms  avg=    0.209ms  min=    0.044ms  max=    1.736ms
  entry total                  cnt=   106  total= 11993.88ms  avg=  113.150ms  min=    0.003ms  max=  645.624ms

========================================
=== /home/user/benchmark/data/NostaleData/NStpData01.NOS ===
========================================

--- open: /home/user/benchmark/data/NostaleData/NStpData01.NOS ---
  header read                  cnt=     5  total=     0.01ms  avg=    0.002ms  min=    0.001ms  max=    0.005ms
  detect format                cnt=     5  total=     0.00ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  parse entry table            cnt=     5  total=     2.05ms  avg=    0.411ms  min=    0.366ms  max=    0.492ms
  mmap                         cnt=     5  total=     0.04ms  avg=    0.007ms  min=    0.005ms  max=    0.013ms
  madvise                      cnt=     5  total=     0.14ms  avg=    0.028ms  min=    0.028ms  max=    0.031ms
  open total                   cnt=     5  total=     2.28ms  avg=    0.456ms  min=    0.408ms  max=    0.551ms
--- extract: /home/user/benchmark/data/NostaleData/NStpData01.NOS ---
  read_entry                   cnt=   187  total=     6.28ms  avg=    0.034ms  min=    0.000ms  max=    0.170ms
  img: parse_image_plan        cnt=   187  total=     0.01ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  img: decode_pixels           cnt=   187  total=     9.04ms  avg=    0.048ms  min=    0.000ms  max=    0.164ms
  img: lodepng::encode         cnt=   170  total=  3727.63ms  avg=   21.927ms  min=    0.027ms  max=  137.212ms
  img: decode_entry_to_png     cnt=   187  total=  3736.73ms  avg=   19.983ms  min=    0.000ms  max=  137.343ms
  file write                   cnt=   170  total=     7.43ms  avg=    0.044ms  min=    0.014ms  max=    0.324ms
  entry total                  cnt=   187  total=  3750.49ms  avg=   20.056ms  min=    0.001ms  max=  137.515ms

========================================
=== /home/user/benchmark/data/NostaleData/NSmnData.NOS ===
========================================

--- open: /home/user/benchmark/data/NostaleData/NSmnData.NOS ---
  header read                  cnt=     5  total=     0.01ms  avg=    0.002ms  min=    0.001ms  max=    0.004ms
  detect format                cnt=     5  total=     0.00ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  parse entry table            cnt=     5  total=    54.78ms  avg=   10.956ms  min=   10.942ms  max=   10.984ms
  mmap                         cnt=     5  total=     0.04ms  avg=    0.007ms  min=    0.006ms  max=    0.012ms
  madvise                      cnt=     5  total=     0.03ms  avg=    0.006ms  min=    0.005ms  max=    0.007ms
  open total                   cnt=     5  total=    55.03ms  avg=   11.007ms  min=   10.992ms  max=   11.046ms
--- extract: /home/user/benchmark/data/NostaleData/NSmnData.NOS ---
  read_entry                   cnt= 23432  total=     2.04ms  avg=    0.000ms  min=    0.000ms  max=    0.057ms
  file write                   cnt= 23432  total=   245.94ms  avg=    0.010ms  min=    0.010ms  max=    0.181ms
  entry total                  cnt= 23432  total=   250.03ms  avg=    0.011ms  min=    0.010ms  max=    0.181ms

========================================
=== /home/user/benchmark/data/NostaleData/NSgtdData.NOS ===
========================================

--- open: /home/user/benchmark/data/NostaleData/NSgtdData.NOS ---
  header read                  cnt=     5  total=     0.01ms  avg=    0.002ms  min=    0.001ms  max=    0.006ms
  detect format                cnt=     5  total=     0.00ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  parse entry table            cnt=     5  total=     0.23ms  avg=    0.046ms  min=    0.039ms  max=    0.074ms
  mmap                         cnt=     5  total=     0.03ms  avg=    0.006ms  min=    0.005ms  max=    0.013ms
  madvise                      cnt=     5  total=     0.15ms  avg=    0.029ms  min=    0.027ms  max=    0.033ms
  open total                   cnt=     5  total=     0.46ms  avg=    0.091ms  min=    0.079ms  max=    0.133ms
--- extract: /home/user/benchmark/data/NostaleData/NSgtdData.NOS ---
  read_entry                   cnt=    38  total=    16.52ms  avg=    0.435ms  min=    0.000ms  max=    2.371ms
  file write                   cnt=    38  total=     4.61ms  avg=    0.121ms  min=    0.006ms  max=    0.826ms
  entry total                  cnt=    38  total=    21.13ms  avg=    0.556ms  min=    0.006ms  max=    3.095ms

========================================
=== /home/user/benchmark/data/NostaleData/NSmpData04.NOS ===
========================================

--- open: /home/user/benchmark/data/NostaleData/NSmpData04.NOS ---
  header read                  cnt=     5  total=     0.01ms  avg=    0.002ms  min=    0.001ms  max=    0.004ms
  detect format                cnt=     5  total=     0.00ms  avg=    0.000ms  min=    0.000ms  max=    0.000ms
  parse entry table            cnt=     5  total=    45.42ms  avg=    9.084ms  min=    9.011ms  max=    9.259ms
  mmap                         cnt=     5  total=     0.04ms  avg=    0.009ms  min=    0.006ms  max=    0.015ms
  madvise                      cnt=     5  total=     0.15ms  avg=    0.029ms  min=    0.028ms  max=    0.031ms
  open total                   cnt=     5  total=    45.67ms  avg=    9.134ms  min=    9.056ms  max=    9.321ms
--- extract: /home/user/benchmark/data/NostaleData/NSmpData04.NOS ---
  read_entry                   cnt=  2048  total=  1021.10ms  avg=    0.499ms  min=    0.007ms  max=    4.910ms
  file write                   cnt=  2048  total=   173.44ms  avg=    0.085ms  min=    0.011ms  max=    5.228ms
  entry total                  cnt=  2048  total=  1194.89ms  avg=    0.583ms  min=    0.019ms  max=    6.305ms

Done.
