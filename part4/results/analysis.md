# Part 4 results and shard-count choice

Choose **4,096 shards** for the measured default 80/10/10 workload at 32
threads. This is the fastest of the five tested counts, not a claim of a
global optimum or the best setting for every thread count and key distribution.

Measurements were collected on Lonestar6 node `c302-005`, starting October
4, 2026 at 20:14:12 UTC. The recorded topology is two AMD EPYC 7763 sockets,
64 physical cores per socket, 128 total, with SMT disabled. The throughput
sweeps use CPU-number order, cross the socket boundary after 64 threads,
and oversubscribe at 192 and 256. All sweep points are medians of three runs.

## Verified data

- Both sweep CSVs contain all 11 requested points, through 256 threads.
  Every median matches the three samples in `sweep.log`.
- All 30 shard-count samples match the medians and ranges in `shard_counts.csv`.
- `test.log` and `tsan.log` each contain two `all checks passed` summaries.
  Neither contains compiler warnings, test failures, or TSan race reports.
- Plots were regenerated from the downloaded data. `coarse_vs_sharded`
  uses `coarse.csv` and `sharded_mutex.csv`. `shard_counts` uses the raw
  `shard_counts.txt` samples and writes the summary CSV.

## Shard-count experiment

| Shards | T=1 median Mops/s | T=32 median Mops/s | T=32 min–max Mops/s | T=32 speedup over T=1 |
|---:|---:|---:|---:|---:|
| 16 | 2.8 | 10.2 | 10.1–10.3 | 3.64× |
| 64 | 3.0 | 22.6 | 22.5–22.8 | 7.53× |
| 256 | 3.2 | 42.2 | 42.1–42.5 | 13.19× |
| 1,024 | 3.9 | 66.4 | 66.3–66.5 | 17.03× |
| 4,096 | 4.7 | 86.2 | 85.9–86.5 | 18.34× |

At 32 threads, 4,096 shards improve throughput by **29.8% over 1,024**
and **104.3% over 256**. The 4,096-shard range, 85.9–86.5, is well above
the 1,024-shard range, 66.3–66.5. The full spread at 4,096 is 0.6 Mops/s,
about 0.7% of its median. These ranges describe only three runs and the
benchmark reports throughput rounded to one decimal place.

The single-thread result also improves by 20.5% from 1,024 to 4,096 shards,
despite having no competing map thread. Consequently the multi-thread gain
cannot be attributed wholly to reduced lock contention: smaller trees and
their cache behavior also contribute. The speedup relative to each count's
own single-thread baseline increases from 17.03× to 18.34×.

## Collision model and its limits

For T=32 independent uniform simultaneous shard choices, a pair collides
with probability 1/N. The expected number of colliding pairs is 496/N.
For a particular choice, the probability that at least one of the other
31 choices selects the same shard is `1-(1-1/N)^31`.

| Shards | Expected colliding pairs | Probability a choice shares its shard |
|---:|---:|---:|
| 16 | 31.0000 | 86.476% |
| 64 | 7.7500 | 38.627% |
| 256 | 1.9375 | 11.426% |
| 1,024 | 0.4844 | 2.983% |
| 4,096 | 0.1211 | 0.754% |

The model predicts substantially fewer opportunities for contention at
4,096, consistent with the measured improvement. It is not a measurement
of actual waiting probability: threads spend time outside locks, and lock
occupancy is affected by operation duration, scheduling, and workload mix.

At 4,096 shards the single-thread throughput corresponds to about 212.8 ns
per operation. At 32 threads, `32 / aggregate throughput` corresponds to
about 371.2 ns of average worker time per operation, a difference of 158.5 ns.
That difference combines contention, scheduling, and cache effects; it is
not a direct measurement of the penalty of one mutex collision.

The report still needs a justified estimate or measurement of the collision
penalty to quantify `collision probability × penalty` against uncontended
cost. These throughput logs alone do not isolate that penalty.

## Scaling sweep at N=256

The separate scaling sweep used **256 shards**, not the chosen 4,096.
It peaks at 54.6 Mops/s at 64 threads, versus 0.7 Mops/s for coarse at the
same thread count (78×). At 96 and 128 threads sharded throughput is 47.6
and 45.2 Mops/s; at 192 and 256 it is 50.7 and 53.8 Mops/s. The coarse
curve is reported near 0.3 Mops/s at 128–256 threads.

Do not label the N=256 curve as a measurement of N=4,096. A further sweep
at 4,096 would be needed to establish its behavior across the full thread
range. Its recommendation here is grounded in the measured 32-thread
shard-count experiment.

The report must identify Lonestar6 explicitly; the assignment specifies
Frontera for graded measurements, so acceptance of this machine remains
an instructor decision. The final writeup and collision-penalty justification
remain to be completed.
