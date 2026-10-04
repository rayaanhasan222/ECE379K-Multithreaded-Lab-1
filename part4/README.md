# Part 4: sharded map

`ShardedMap` stores a fixed, positive number of independent maps. The hash
selects one shard. Inserts and erases take its exclusive lock; finds use
`ReadGuard`, allowing shared locking when supported. Padded shards start
on cache-line boundaries and occupy a whole number of cache lines.

## Exact size and lock order

`size()` acquires all locks in increasing index order and keeps them until
the sum is complete. Once the final lock is acquired, every shard is frozen,
so the sum is the map's size at that instant. Two size callers cannot form
a cycle because both acquire locks in the same order. An insert, find, or
erase holds only one shard lock and never waits for a second one, so it
cannot complete a lock-order cycle with size. RAII releases acquired locks
if a later acquisition throws.

Locking, counting, and releasing one shard at a time would combine values
from different instants. For example, moving an entry from a later shard
to an already-counted shard can make the sum omit that entry; the reverse
can double-count it. Such a sum need not equal the total at any instant.

## Lonestar6 measurements

Run on a full 128-core CPU compute-node allocation. From the lab root:

```bash
make -C starter_files test
make -C starter_files tsan
mkdir -p part4/results
cp part2/results/coarse.csv part4/results/coarse.csv
lscpu > part4/results/lscpu.txt
hostname > part4/results/hostname.txt
date -u > part4/results/started_at.txt
cd starter_files
set -o pipefail
make -B bench
env -u MIX -u WRITERS -u BUCKETS make sweep IMPLS=sharded:mutex SHARDS=256 \
  THREADS="1 2 4 8 16 32 64 96 128 192 256" \
  2>&1 | tee ../part4/results/sweep.log
cp sharded_mutex.csv ../part4/results/sharded_mutex.csv
```

Archive previous results before rerunning. The existing harness runs each
thread count three times and reports the median; keep its log for the
individual samples and pin order. If comparing runs from different days
or nodes, record that limitation or remeasure coarse on the same allocation.
The README specifies Frontera for graded measurements; identify Lonestar6
explicitly and confirm its acceptance with the instructor.

Then, still in `starter_files`, measure shard counts at one and 32 threads,
three repetitions each. Restrict workers to the first socket's first 32 CPUs:

```bash
for t in 1 32; do
  for n in 16 64 256 1024 4096; do
    for r in 1 2 3; do
      env -u MIX -u WRITERS -u BUCKETS \
        taskset -c "0-$((t - 1))" ./bench sharded:mutex "$t" "$n"
    done
  done
done 2>&1 | tee ../part4/results/shard_counts.txt
```

Generate the comparison and shard-count plots on a machine with matplotlib:

```bash
cd ..
python3 part4/plot.py part4/results --machine ls6
```

## Choosing N from the measurements

For independent uniform shard choices, a particular pair chooses the same
shard with probability `1/N`. With T simultaneous choices, the expected
number of colliding pairs is `T(T-1)/(2N)`. A particular operation shares its
shard with at least one other choice with probability `1-(1-1/N)^(T-1)`.
These are models of simultaneous choices, not measured lock-wait rates;
actual occupancy depends on time spent outside and inside the lock.

Use the one-thread curve to quantify how smaller trees change the
uncontended operation cost. Compare that baseline with the 32-thread curve
to assess contention, and choose N using the measured throughput and spread.
Estimate a collision penalty and compare `collision probability * penalty`
with the uncontended cost; explain how the penalty was obtained and the
limits of your model. Throughput alone does not isolate mutex wait latency.

For a latency-like throughput estimate, `1000 / Mops` gives nanoseconds per
operation at one thread, and `1000*T / aggregate_Mops` gives average worker
time per operation under a balanced T-thread load. The latter includes
waiting, scheduling, and cache effects. Do not label their difference a
direct measurement of one lock handoff.

The downloaded measurements select 4,096 shards for the tested 32-thread
workload. See `results/analysis.md` for the evidence and limits of this choice.
