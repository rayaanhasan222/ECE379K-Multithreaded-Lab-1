# LS6 node commands: Parts 5–7 data collection

Run the blocks in order, individually, on a full CPU compute-node allocation.
These are interactive instructions, not one unattended script. Inspect results
at each checkpoint. Do not run competing tests or benchmarks on the same node.
There are no external command-level timeouts in the instructions below. Tests
run until they finish, fail, are interrupted, or the Slurm allocation ends.
Slurm's allocation wall-time limit still applies. Benchmark run durations
(the default two-second workload and RUN_SECONDS=20) remain intentional parts
of the measurement protocol, not kill timers.

Preserve the existing remote directory and logs when starting a new session.
Do not delete the project to restart a node: create a new results directory.

Keep local and LS6 source in sync before starting. The current hash_map.h has
hash mixing and per-stripe counts: size() locks all stripes and sums their counts
instead of scanning every bucket. This reduces work while writers wait, while
adding count updates and storage. Use this same version throughout the dataset;
do not mix old hashed measurements with new ones. Record the source snapshot and
checksums below; retest and remeasure affected comparisons if you change it.

All command blocks below run on your LS6 compute node, starting with the current
project already transferred. Collect everything in one RESULTS directory, then
copy that entire directory back for analysis. No plotting is needed on the node.

Your latest LS6 run passed both ordinary and both TSan test programs before the
connection closed. Section 2 deliberately repeats them to save fresh logs with
this dataset. The earlier status 143 indicated SIGTERM, not an assertion failure.

HITM will be measured separately on your Intel LRC machine. The LS6 commands
collect the other counters without Intel events. Keep the two machines' datasets
separate: an LRC HITM count must be normalized by the operations from that LRC
run, and cannot be inserted into an LS6 counter row as if it came from LS6.
Record the LRC model and available events when doing that later experiment;
an Intel processor alone does not guarantee a particular HITM event exists.

Use a full-node allocation with several hours available if you want to finish
on one node. A 30-minute allocation is insufficient: Part 6 alone has 26.4 minutes
of timed work, and the Parts 5–7 throughput repetitions total roughly 43 minutes before
warm-ups, perf runs, tests, launches, and possible contention overruns. Several
hours is planning headroom, not a guaranteed completion time. The commands have
no external kill timers, but Slurm still enforces your allocation's end time.

## What you need from your partner (Parts 1–4)

Your partner owns Parts 1–4. This guide does not rerun their coarse-map sweeps,
Part 3 counter tables, or Part 4 mutex shard-count search. You need:

- Their implemented concurrent_map.h, plus the shared interface.h and parts.h,
  synchronized with your current locks.h and hash_map.h. The provided tests and
  benchmark compile these implementations together, so keep all part switches on.
- Their Part 4 chosen shard count N and the data/justification supporting it.
  Use that N consistently for the Part 5 lock comparison, Part 6's many-shard
  setting, and Part 7's equal shard/stripe container comparison. Section 1 uses
  256; replace it with their selected value if available. If their selection is
  pending, you can collect at 256 as a provisional comparison setting, but record
  that fact. A later different choice may require rerunning those comparisons.
- Their Part 4 mutex shard-count results for the final comparison with your TTAS
  and Parking shard-count sweeps. This is an analysis dependency; it does not
  prevent collecting your custom-lock samples now. Your sweeps use T=1 and T=32;
  use matching thread counts when comparing directly with their mutex data.

Two earlier implementations still appear in YOUR measurements by design:
`sharded:mutex` is the required standard-lock baseline among Part 5's five locks,
and `sharded:ttas` is the required tree baseline for Part 7's hash comparison.
Both are included below. No coarse-map performance measurement is needed for
Parts 5–7. Running the complete provided test suite is shared correctness
validation, not taking over your partner's experiments.

Collect the required predictions before measuring (section 3). N/L selection,
collision arguments, plots, and report analysis can be completed after copying
back the data and combining it with your partner's results.

## 1. Start in starter_files and record the environment

Adjust the directory to your actual upload destination:

```bash
bash
cd ~/ECE379K-Multithreaded-Lab-1/starter_files
module load gcc
set -o pipefail

LAB=$(cd .. && pwd)
RESULTS="$LAB/measurements/ls6-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$RESULTS"
unset MIX WRITERS BUCKETS EVENTS CPUS DELAY RUN_SECONDS CXXFLAGS MAKEFLAGS MFLAGS
export LC_ALL=C
THREADS="1 2 4 8 16 32 64 96 128 192 256"
N=256  # use your partner's chosen Part 4 N if different

hostname | tee "$RESULTS/hostname.txt"
date -u > "$RESULTS/started_at.txt"
printf 'N=%s\nTHREADS=%s\nHITM=separate LRC dataset\n' "$N" "$THREADS" > "$RESULTS/configuration.txt"
lscpu > "$RESULTS/lscpu.txt"
lscpu -p=CPU,CORE,SOCKET > "$RESULTS/topology.csv"
taskset -pc $$ | tee "$RESULTS/affinity.txt"
g++ --version | tee "$RESULTS/compiler.txt"
module list > "$RESULTS/modules.txt" 2>&1
printf '%s\n' "$SLURM_JOB_ID" > "$RESULTS/job_id.txt"
scontrol show job "$SLURM_JOB_ID" > "$RESULTS/allocation.txt"
squeue -j "$SLURM_JOB_ID" -o "%.18i %.10M %.10L" | tee "$RESULTS/time-remaining-at-start.txt"
sha256sum *.h bench.cpp Makefile sweep.sh perfstat.sh ../tests/test_map.cpp ../tests/test_locks.cpp > "$RESULTS/source-checksums.txt"
tar -C "$LAB" -czf "$RESULTS/source.tar.gz" README.md tests \
  starter_files/concurrent_map.h starter_files/locks.h starter_files/hash_map.h \
  starter_files/parts.h starter_files/interface.h starter_files/bench.cpp \
  starter_files/Makefile starter_files/sweep.sh starter_files/perfstat.sh

mkdir -p "$RESULTS/existing-csvs"
for csv in *.csv; do
  if [ -f "$csv" ]; then cp "$csv" "$RESULTS/existing-csvs/"; fi
done
printf 'Results directory: %s\n' "$RESULTS"
```

Check for 128 physical cores, two sockets, 64 cores per socket, and an allocation
that grants the full node. Confirm CPU numbers 0-63 belong to the first socket
before using the explicit masks below. A mask of 0-127 permits execution on those
CPUs; it does not alone prove an exclusive full-node allocation or absence of a
CPU quota. The earlier `nproc=1` discrepancy was explained by the environment:
without OpenMP settings, nproc reported 128. You can repeat that check:

```bash
nproc
printenv OMP_NUM_THREADS OMP_THREAD_LIMIT
env -u OMP_NUM_THREADS -u OMP_THREAD_LIMIT nproc
```

If the last command again reports 128, this reproduces the resolved discrepancy.
These C++ programs use std::thread, not OpenMP, so those OpenMP settings do not
set their worker count. If the last command instead reports a restricted count,
inspect the allocation/cgroup limits before benchmarking.

Ensure all four HAVE_* switches in parts.h are 1. Retain the instructor's LS6
approval with your report notes. This runbook uses the approved 64-core socket
and 128-core node boundaries; older part2/part4 documentation still contains
Frontera-specific instructions and should not override these choices.

`set -o pipefail` makes a failed command visible even when its output is piped to
`tee`. `tee` displays and saves output. `2>&1` includes diagnostics in the log.
`RESULTS` and `THREADS` are shell variables for reuse below, not Makefile settings.
pipefail does not stop a pasted sequence or loop automatically. Stop and inspect
any failed command; do not continue just because the next shell prompt appears.

## 2. Build five executables and rerun both test suites

```bash
make -B CXX=g++ all test_map_tsan test_locks_tsan \
  2>&1 | tee "$RESULTS/build.log"
statuses=("${PIPESTATUS[@]}")
printf 'make=%s tee=%s\n' "${statuses[0]}" "${statuses[1]}" | tee "$RESULTS/build.status"
```

Proceed only when both statuses are 0. `-B` forces native rebuilding, even if copied Mac
binaries have newer timestamps. `all` expands to bench, test_map, and test_locks.
The other two targets add instrumented test executables. This command compiles;
it does not run the programs. `-O2` optimizes bench and ordinary tests. TSan tests
use `-O1 -g -fsanitize=thread`. They are separate binaries; TSan does not change bench.

```bash
stdbuf -oL -eL make CXX=g++ test \
  2>&1 | tee "$RESULTS/test.log"
statuses=("${PIPESTATUS[@]}")
printf 'make=%s tee=%s\n' "${statuses[0]}" "${statuses[1]}" | tee "$RESULTS/test.status"
```

Run the TSan block only after the ordinary suite passes:

```bash
stdbuf -oL -eL make CXX=g++ tsan \
  2>&1 | tee "$RESULTS/tsan.log"
statuses=("${PIPESTATUS[@]}")
printf 'make=%s tee=%s\n' "${statuses[0]}" "${statuses[1]}" | tee "$RESULTS/tsan.status"
```

Both suites should finish with make=0, tee=0, and `all checks passed` from each
of their two programs. The statuses assignment must immediately follow the
pipeline: even an echo changes PIPESTATUS. stdbuf enables timely output; it
does not speed up or otherwise repair the tests. A mapping fatal error is a
runtime setup failure; a race warning requires inspecting the reported accesses.
No command above terminates the tests after a fixed number of minutes. Check
allocation time before starting; a Slurm termination is still an incomplete run.

If you want to repeat the isolated progress diagnostic before the full tests:

```bash
g++ -std=c++20 -O2 -Wall -Wextra -pthread -I. \
  ../tests/diagnose_size.cpp -o /tmp/lab1-diagnose-size
/tmp/lab1-diagnose-size 2>&1 | tee "$RESULTS/size-diagnostic.log"
statuses=("${PIPESTATUS[@]}")
printf 'diagnostic=%s tee=%s\n' "${statuses[0]}" "${statuses[1]}" | tee "$RESULTS/size-diagnostic.status"
```

This is optional now that the prior diagnostic passed. Rebuilding picks up
whatever header is currently on LS6. Its progress counters show completed
calls and can perturb scheduling; they are diagnostic, not benchmark data.

`make test` runs map and lock correctness tests. `make tsan` runs the same kinds
of checks while instrumenting memory accesses to detect unsynchronized sharing.
These establish evidence of correctness, not performance. Save the full output.

Do not run `make clean`: this Makefile deletes CSVs. No need for another
`make bench` after this build unless the sources change; make sweep/perf also
depend on bench and rebuild it when necessary.

## 3. Predictions and perf preparation

Before collecting each relevant dataset, record your own expectations in a dated
file. These are required pre-measurement predictions, not the later analysis:

- Part 5 contention: expected largest L1 misses/op and instructions/op, and why.
- Part 7: expected misses versus mean chain length; predicted padding effect.

Do not rewrite a prediction afterward to match observations. If retaining an
existing prediction, identify its original date and any machine adaptation.

```bash
mkdir -p "$RESULTS/part5" "$RESULTS/part6" "$RESULTS/part7"
perf stat -e cycles true 2>&1 | tee "$RESULTS/perf-check.txt"
perf list > "$RESULTS/perf-list.txt"
```

Save your predictions before section 4 (or copy an existing, genuinely earlier
prediction file into RESULTS). This opens a text editor; write the items
listed above, then Ctrl+O, Enter to save and Ctrl+X to exit:

```bash
nano "$RESULTS/predictions.md"
```


Cycles must count successfully. `perf stat` collects hardware/software event
counts while a program runs. `make perf` uses perfstat.sh to pin a CPU set, skip
warm-up using a delay, run longer, divide counts by bench's operation count, and
compute IPC. Its defaults include cycles, instructions, L1 loads/misses, generic
cache misses, context switches, and migrations.

Examples below use RUN_SECONDS=20 and DELAY=200 as an initial setting. Read the
actual warm-up output on EVERY run. Increase DELAY when warm-up approaches or
exceeds it, and rerun contaminated measurements in a new file. Avoid setting a
huge delay blindly: perf counts only part of the timed run but divides by the
whole operation count. Long runs with a small post-warm-up margin reduce this
counting-window mismatch. Record it as a limitation. Counter meanings and cache
sizes must match the AMD machine, not the README's Intel assumptions.

Leave EVENTS unset here so perfstat.sh uses its standard non-HITM event list.
The later LRC experiment will collect HITM together with its own operation counts
and supporting counters. LS6 data collection does not wait for that experiment.
Inspect unsupported/not-counted events on every perf run; a cycles-only smoke
check does not establish that all default events work. perfstat.sh retains raw
counts and per-op values in its printed table, but drops perf's multiplexing
percentages; if simultaneous events cannot count reliably, use separately
documented event groups rather than treating missing values as zero.

## 4. Part 5: five locks and two counter experiments

```bash
make CXX=g++ sweep \
  IMPLS="sharded:mutex sharded:tas sharded:ttas sharded:ticket sharded:park" \
  SHARDS="$N" THREADS="$THREADS" 2>&1 | tee "$RESULTS/part5/sweep.log"
cp sharded_mutex.csv sharded_tas.csv sharded_ttas.csv sharded_ticket.csv sharded_park.csv \
  "$RESULTS/part5/"
```

One sweep point is the median of three fresh runs. The log contains the three
values and selected CPU set; the CSV contains the median. The harness warms up
524,288 entries, then workers perform random operations on 1,048,576 possible
keys. Default mix is 80% find, 10% insert, 10% erase. Default timed duration is
two seconds per run, with possible overrun before workers finish.

The given script restricts the process to a CPU set using taskset; it does NOT
bind each individual worker to a distinct CPU. Migrations within the set are
possible. At 192/256 threads, the set still contains 128 CPUs: threads must share
cores. At 64 threads the selected set is on one socket; larger sets span sockets.

After EVERY sweep, before accepting the saved CSVs, check all expected rows
and inspect the log's three samples per point. The Makefile can return success
even if a benchmark inside sweep.sh failed. For this five-lock sweep, run:

```bash
python3 - sharded_mutex.csv sharded_tas.csv sharded_ttas.csv sharded_ticket.csv sharded_park.csv <<'PY'
import csv, math, sys
expected = [1, 2, 4, 8, 16, 32, 64, 96, 128, 192, 256]
for name in sys.argv[1:]:
    with open(name, newline='') as f:
        reader = csv.DictReader(f)
        assert reader.fieldnames == ['threads', 'mops', 'cpus'], name
        rows = list(reader)
    assert [int(r['threads']) for r in rows] == expected, name
    assert all(math.isfinite(float(r['mops'])) and float(r['mops']) >= 0
               and r['cpus'] for r in rows), name
    print(name, 'complete numeric CSV; also inspect all three samples in its log')
PY
```

Reuse this block with the filenames of each later sweep. A rounded zero is not
automatically a failure. Numeric CSVs alone cannot verify every repetition;
blank/missing samples in the log require rerunning the affected experiment.

Container, workload, and shard count stay fixed while waiting strategy changes.
The single-thread region exposes uncontended costs; high thread counts expose
contention, cross-socket effects, and eventually scheduling pressure.

```bash
for lock in mutex tas ttas ticket park; do
  RUN_SECONDS=20 DELAY=200 make CXX=g++ perf IMPL="sharded:$lock" T=8 SHARDS=1 CPUS=0-7 \
    2>&1 | tee "$RESULTS/part5/contention_${lock}.txt"
done

for lock in mutex tas ttas ticket park; do
  RUN_SECONDS=20 DELAY=200 make CXX=g++ perf IMPL="sharded:$lock" T=32 SHARDS="$N" CPUS=0-7 \
    2>&1 | tee "$RESULTS/part5/oversubscribed_${lock}.txt"
done
```

The first experiment forces all operations to contend on one lock, with eight
threads allowed on eight cores. The second forces 32 threads onto eight cores
even though the node has 128. It exposes spinning versus sleeping/yielding.

Approximate busy fraction = cycles/op * Mops/s * 1e6 / (8 * clock_Hz).
Report the clock assumption and effects of frequency variation/counter windows.
This is CPU busy time, not useful-work efficiency: spinning can keep CPUs busy.

Repeat shard-count selection with the two requested custom locks:

```bash
for lock in ttas park; do
  for t in 1 32; do
    for n in 16 64 256 1024 4096; do
      for r in 1 2 3; do
        taskset -c "0-$((t - 1))" ./bench "sharded:$lock" "$t" "$n"
      done
    done
  done 2>&1 | tee "$RESULTS/part5/shard_counts_${lock}.txt"
done
```

## 5. Part 5: relaxed-ordering experiment in an isolated copy

This deliberately makes TASLock's protected-data synchronization incorrect.
Never benchmark this copy or copy it over the correct implementation.

```bash
RELAXED="$RESULTS/part5/relaxed"
mkdir -p "$RELAXED/starter_files" "$RELAXED/tests"
cp *.h Makefile "$RELAXED/starter_files/"
cp ../tests/test_map.cpp ../tests/test_locks.cpp "$RELAXED/tests/"
python3 - "$RELAXED/starter_files/locks.h" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
source = p.read_text()
start = source.index('class TASLock {')
end = source.index('class TTASLock {', start)
block = source[start:end]
assert block.count('std::memory_order_acquire') == 1
assert block.count('std::memory_order_release') == 1
block = block.replace('std::memory_order_acquire', 'std::memory_order_relaxed')
block = block.replace('std::memory_order_release', 'std::memory_order_relaxed')
p.write_text(source[:start] + block + source[end:])
PY
diff -u locks.h "$RELAXED/starter_files/locks.h" > "$RELAXED/change.diff"
make -B -C "$RELAXED/starter_files" CXX=g++ test_map test_locks test_map_tsan test_locks_tsan \
  2>&1 | tee "$RELAXED/build.log"
```

diff's status 1 just means it found the intended difference. Verify the build
succeeded before running these separately:

```bash
stdbuf -oL -eL \
  make -C "$RELAXED/starter_files" CXX=g++ test 2>&1 | tee "$RELAXED/plain.log"
statuses=("${PIPESTATUS[@]}")
printf 'make=%s tee=%s\n' "${statuses[0]}" "${statuses[1]}" | tee "$RELAXED/plain.status"

stdbuf -oL -eL \
  make -C "$RELAXED/starter_files" CXX=g++ tsan 2>&1 | tee "$RELAXED/tsan.log"
statuses=("${PIPESTATUS[@]}")
printf 'make=%s tee=%s\n' "${statuses[0]}" "${statuses[1]}" | tee "$RELAXED/tsan.status"
```

Record the actual outcomes. A plain pass is possible but does not make the lock
correct. Relaxed operations keep the flag atomic but omit the acquire/release
handoff for ordinary protected data. A TSan race report is expected evidence for
the experiment. make tsan stops if the first program fails; to also capture the
standalone lock diagnostic:

```bash
TSAN_OPTIONS=halt_on_error=1 \
  "$RELAXED/starter_files/test_locks_tsan" 2>&1 | tee "$RELAXED/locks-tsan.log"
statuses=("${PIPESTATUS[@]}")
printf 'test=%s tee=%s\n' "${statuses[0]}" "${statuses[1]}" | tee "$RELAXED/locks-tsan.status"
```

The original starter_files/locks.h was never changed, so no production restore
is needed. Keep this copy with the experiment evidence, outside the submission
source files.

## 6. Part 6: mixes, shared locks, and reader/writer roles

```bash
for mix in 80/10/10 50/25/25 100/0/0; do
  for n in 1 "$N"; do
    out="$RESULTS/part6/mix-${mix//\//-}_shards-$n"
    mkdir -p "$out"
    MIX="$mix" make CXX=g++ sweep \
      IMPLS="sharded:ttas sharded:rw sharded:rwp sharded:shared_mutex" \
      SHARDS="$n" THREADS="$THREADS" 2>&1 | tee "$out/sweep.log"
    cp sharded_ttas.csv sharded_rw.csv sharded_rwp.csv sharded_shared_mutex.csv "$out/"
  done
done
```

Four locks * three mixes * two shard counts = 24 sweeps. With 11 points, three
repetitions, and a nominal two seconds per run, timed work alone is about 26.4
minutes, plus warm-ups, launches, and contention overruns. Plan allocation time
accordingly; split by mix if necessary. Never keep waiting on a stalled run
without checking remaining allocation time and saving the incomplete result.

MIX is find/insert/erase percentage. All-reader traffic can benefit from sharing;
one shard maximizes contention; many shards reduce how often sharing helps.
RWLockWP may favor writers at the expense of reader/total throughput.

Separate-role comparison (example: four writers and 28 readers):

```bash
for n in 1 "$N"; do
  for lock in rw rwp; do
    for r in 1 2 3; do
      WRITERS=4 taskset -c 0-31 ./bench "sharded:$lock" 32 "$n"
    done 2>&1 | tee "$RESULTS/part6/roles_${lock}_shards-${n}.txt"
  done
done
```

Four writers is an experimental choice, not a mandated count. WRITERS overrides
MIX. Writer threads split inserts/erases; others only read. Save raw output with
rd, wr, and total throughput: sweep CSVs retain only the total.

## 7. Part 7: bucket count and chain length

The hash mixing changes stripe distribution, bucket collisions, and locality.
Rerun hashed measurements made with the original hash and document the new
mapping. Use one implementation version for all these comparisons.

Measure the one-thread tree baseline at the same shard/stripe count:

```bash
for r in 1 2 3; do
  taskset -c 0 ./bench sharded:ttas 1 "$N"
done 2>&1 | tee "$RESULTS/part7/tree_baseline.txt"
```

Collect all six allowed factor-of-four bucket sizes so the crossover and slope
can be analyzed after copying the folder back. The last is 2048; the next value,
512, would violate the README's 1024-bucket floor. Long chains make later warm-ups
slower. This loop saves three throughput samples and one perf run at each B.

```bash
for B in 2097152 524288 131072 32768 8192 2048; do
  for r in 1 2 3; do
    BUCKETS="$B" taskset -c 0 ./bench hashed:ttas 1 "$N"
  done 2>&1 | tee "$RESULTS/part7/buckets_${B}.txt"

  DELAY_MS=$(awk '/^warm-up [0-9]+ ms/ {if ($2 > max) max=$2; seen=1}
    END {if (!seen) exit 1; print max+100}' "$RESULTS/part7/buckets_${B}.txt") || break
  BUCKETS="$B" RUN_SECONDS=20 DELAY="$DELAY_MS" \
    make CXX=g++ perf IMPL=hashed:ttas T=1 SHARDS="$N" CPUS=0 \
    2>&1 | tee "$RESULTS/part7/buckets_${B}_perf.txt" || break
done
```

awk reads the three observed warm-up times and adds a 100 ms margin to their
maximum for the perf delay. It changes only when counters start, not the harness.
Warm-up varies: inspect each perf log for a warning and check that its actual
warm-up remains slightly below DELAY. If it exceeds DELAY, rerun that B's perf
command with a larger delay, writing a new `_perf_retry.txt` file. Conversely,
a delay far above the actual warm-up skips too much timed work; reduce it and
rerun. Preserve both attempts. A failed command or missing warm-up stops this
loop early; verify that all six bucket files and all six valid perf runs exist.

Mean chain length is entries/B, NOT the number of possible keys/B. Warm-up adds
524,288 entries and balanced inserts/erases keep occupancy near that scale.
Explain the approximation and consider successful versus unsuccessful lookups.
Later we will plot measured L1 misses/op against mean chain length, estimate the
slope and cycles/miss behavior, and compare with the actual machine's hierarchy.

## 8. Part 7: stripe count and padding

Keep BUCKETS at its default while varying stripes, to isolate lock granularity:

```bash
for lock in ttas park; do
  for t in 1 32; do
    for l in 16 64 256 1024 4096; do
      for r in 1 2 3; do
        taskset -c "0-$((t - 1))" ./bench "hashed:$lock" "$t" "$l"
      done
    done
  done 2>&1 | tee "$RESULTS/part7/stripe_counts_${lock}.txt"
done
```

Save the L samples for later selection and justification, separately for each
lock. The padding comparison below uses a fixed count of 1024, matching its
targeted counter experiment; no final L decision is needed while on the node:

Before running, obtain TTAS lock size from the actual compiler, then record the
predicted number of unpadded hash stripe locks per 64-byte line. The sharded map
stores map metadata with its lock, so do not assume identical layout there.

```bash
cat > "$RESULTS/part7/lock_size.cpp" <<'CPP'
#include <cstdio>
#include "locks.h"
int main() {
    std::printf("TTASLock size=%zu alignment=%zu CACHE_LINE=%zu\n",
                sizeof(TTASLock), alignof(TTASLock), CACHE_LINE);
}
CPP
g++ -std=c++20 -O2 -pthread -I. "$RESULTS/part7/lock_size.cpp" -o "$RESULTS/part7/lock_size"
"$RESULTS/part7/lock_size" | tee "$RESULTS/part7/lock_size.txt"
```

The size probe is not a benchmark-harness modification.

```bash
PAD_COUNT=1024
make CXX=g++ sweep \
  IMPLS="hashed:ttas hashed:ttas:nopad sharded:ttas sharded:ttas:nopad" \
  SHARDS="$PAD_COUNT" THREADS="$THREADS" 2>&1 | tee "$RESULTS/part7/padding_sweep.log"
cp hashed_ttas.csv hashed_ttas_nopad.csv sharded_ttas.csv sharded_ttas_nopad.csv \
  "$RESULTS/part7/"

for impl in hashed:ttas hashed:ttas:nopad; do
  RUN_SECONDS=20 DELAY=200 make CXX=g++ perf IMPL="$impl" T=28 SHARDS=1024 CPUS=0-27 \
    2>&1 | tee "$RESULTS/part7/${impl//:/_}_padding_perf.txt"
done
```

Padding keeps adjacent lock words from sharing a cache line, but does not remove
all possible false sharing in bucket heads or allocated nodes. Keep all three
throughput samples from the log, report spread, and explain whether the difference
exceeds noise during analysis. The matching HITM experiment will be collected
separately on LRC, together with that machine's own baseline counters.

## 9. Check and save the folder for copying back

Before leaving the node, inspect the logs for errors, unsupported counters,
warm-up warnings, and incomplete sweeps. Missing data is easier to rerun while
still on the node. The sweep CSVs should each have 11 data rows plus their header.
The three raw samples behind each median remain in that experiment's sweep log.

```bash
python3 - "$RESULTS" <<'CHECK'
import csv, math, sys
from pathlib import Path
root = Path(sys.argv[1])
expected = [1, 2, 4, 8, 16, 32, 64, 96, 128, 192, 256]
files = sorted(p for part in range(5, 8) for p in (root / f'part{part}').rglob('*.csv'))
assert len(files) == 33, f'Expected 33 Parts 5–7 sweep CSVs, found {len(files)}'
for p in files:
    with p.open(newline='') as f:
        reader = csv.DictReader(f)
        assert reader.fieldnames == ['threads', 'mops', 'cpus'], p
        rows = list(reader)
    assert [int(r['threads']) for r in rows] == expected, p
    assert all(math.isfinite(float(r['mops'])) and float(r['mops']) >= 0
               and r['cpus'] for r in rows), p
    print('CSV complete:', p.relative_to(root))
print('All saved sweep CSVs complete; inspect repetition logs and perf output too.')
CHECK
```

This checks the 33 expected sweep CSVs for your work: five Part 5, 24 Part 6,
and four Part 7. Your partner's Part 1–4 files are not required by this check.
It does not prove every counter run succeeded or every repetition was valid.
Do not mark an incomplete dataset as complete merely because this check passes.

Once you have checked the output, save the end time and a file inventory:

```bash
date -u > "$RESULTS/finished_at.txt"
python3 - "$RESULTS" <<'MANIFEST'
import hashlib, sys
from pathlib import Path
root = Path(sys.argv[1])
with (root / 'manifest.sha256').open('w') as out:
    for p in sorted(root.rglob('*')):
        if p.is_file() and p.name != 'manifest.sha256':
            digest = hashlib.sha256(p.read_bytes()).hexdigest()
            out.write(f'{digest}  {p.relative_to(root)}\n')
MANIFEST
printf '\nCopy this entire results directory back:\n%s\n' "$RESULTS"
```

The folder contains source/checksums, machine details, build/test logs, CSVs, raw
benchmark samples, perf operation counts and counter tables, and the isolated
relaxed-lock experiment. Preserve all of it. Keep the deliberately incorrect
relaxed source under the measurement folder, not in your submission headers.
After copying it back, we can analyze the curves, compute spreads and counter
ratios, choose N/L, and prepare plots and report text.

If the allocation ends early, completed files remain in your home directory.
On another node, repeat section 1 for a new RESULTS directory and section 3's
folder creation/perf check. Retain your original predictions and the same N
recorded in configuration.txt. Run only
the unfinished experiment blocks, rerunning an interrupted sweep in full. Full
tests need not be repeated solely because the node changed. Keep both result
directories for analysis; do not overwrite previous evidence.
