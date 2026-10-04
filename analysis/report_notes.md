# Parts 5–7 report notes

These are retrospective findings from saved measurements. Add your private
predictions separately. The notebook exports source paths/checksums and the
underlying numeric tables.

## Configuration and remaining requirements

The new LS6 logs confirm N=4096 for the five-lock sweep, Part 6 chosen-N
comparisons, and the Part 7 same-N tree/hash bucket experiment. N=1 remains the
contention control. Padding intentionally uses 1024. Transfer manifests and
source checksums verify; sweep medians agree with the three saved repetitions.

4096 is a defensible chosen N: it is best among tested counts at T=32 for mutex,
TTAS, and Parking in the same-node shard search. It is the end of the tested
range, not a proven global optimum or an optimum for every workload.

The professor approved LS6 measurements, as confirmed by you. Most curves use
LS6 (AMD EPYC, 64 cores/socket, 128 physical cores); HITM experiments use
Frontera (Intel, 28 cores/socket, 56 physical cores). Both are valid for this
approved protocol. Preserve machine labels and normalize each counter using
operations from the same run; do not pool the two machines' measurements.

One measurement limitation remains:
- N=4096 oversubscription counters have one run per lock and no task-clock.
  N=256 has three raw-perf repetitions with task-clock. To measure busy fraction
  at N=4096, collect task-clock and elapsed time alongside the counters. If using
  the assignment's cycles-based estimate instead, state the assumed clock and
  uncertainty: nominal frequency is not measured active frequency, and the
  resulting estimate can exceed 100%.

The four-writer role configuration follows your confirmation of the runbook;
saved role output records total threads but not WRITERS. Retain that protocol
qualification. Source-matched normal and TSan validation logs are available.

## Part 5

Main figures: p5_lock_scaling, p5_contention_frontera,
p5_oversubscription_N4096, p5_shard_choice. LS6 contention and N=256
utilization are supporting figures.

At N=4096 and T=128, mutex/TAS/TTAS/Ticket/Parking medians are
155.1/152.3/169.8/187.4/186.0 Mops/s. At T=256 they are
154.4/24.8/28.5/51.7/33.6 Mops/s. Mutex sustains throughput under
oversubscription much better in this sweep. Parking beats mutex at 128 threads
but is substantially slower at 256. Explain using the actual implementation's
spin, park, wakeup and scheduling behavior.

The same-node T=32 shard search gives mutex/TTAS/Parking medians of
42.31/63.84/62.97 Mops/s at N=256 and 86.70/101.14/100.22 at N=4096.
The collision model is 1 - (1 - 1/N)^(T-1) for independent uniform choices;
it does not measure simultaneous lock collisions or handoff time.

The relaxed-ordering experiment passes ordinary tests but reports races under
TSan. Include the acquire/release explanation and p5_relaxed_ordering.csv.
Passing ordinary tests does not establish correct synchronization.

## Part 6

Main figures: p6_mix_shard_matrix, p6_relative_to_ttas,
p6_reader_writer_roles. Exact values are in part6_sweeps.csv and
p6_ratios_to_ttas.csv. Older high-thread repeats are N=256 supporting evidence,
not repetitions of the N=4096 experiment.

Pure-read N=1 strongly favors shared-reader locks over TTAS; mixed workloads
do not show universal reader-writer superiority. Compare each mix and thread
count individually. Preserve the large spread near 96 threads; these
measurements alone do not establish its cause.

At N=1 with 28 readers and 4 writers, reader-preferring RW reports median
reader/writer rates of 8.1/0.0 Mops/s; writer-preferring RW reports 0.0/0.5.
Printed 0.0 means below 0.05 at this precision, not proven starvation.
At N=4096 both variants report 109.8/10.6; raw total-throughput medians are
120.389 and 120.346 Mops/s, respectively.

Discuss reader entry/exit atomic costs, concurrent reads, and the waiting-writer
behavior suggested by std::shared_mutex. Throughput alone does not establish
a fairness guarantee.

## Part 7

Main figures: p7_bucket_crossover, p7_chain_length, p7_stripe_choice,
p7_padding_throughput, p7_padding_counters_frontera.

At T=1 and N=L=4096, the tree median is 5.324 Mops/s. Hash throughput is
3.898 at 32,768 buckets and 11.000 at 131,072. The sampled crossover is
between those two bucket counts, different from the older N=256 bracket.
At 2,097,152 buckets the hash median is 16.756 Mops/s.

Using estimated occupancy 524,288, the six-point fitted slope is 0.756 L1
misses/op per additional entry/bucket. The descriptive cycles-versus-misses
slope is 66.43 cycles/miss. These counters include more than traversal; an
L1 miss does not identify its service level. Use long-chain rows and course
latency evidence to discuss the likely hierarchy level; this fit alone cannot
prove LLC versus DRAM service.

At T=32, TTAS/Parking medians are 238.81/265.98 Mops/s at L=1024 and
244.39/282.00 at L=4096. 4096 is best tested; gains are smaller than earlier
in the search and T=1 performance falls slightly. Redo the collision argument
with L and discuss the memory/overhead tradeoff.

At 128 threads, the hash table's largest absolute padding gain is 333.8 Mops/s
(6.14×); observed ranges do not overlap. The sharded tree's largest absolute
gain is 19.5 Mops/s (1.17×), but ranges overlap: acknowledge the noise.
See p7_padding_effect.csv for every point. One-byte stripe locks imply up to
64 unpadded locks per 64-byte line; sharded map objects have different density.

Frontera at T=28/L=1024 gives padded/unpadded throughput medians of
259.33/71.90 Mops/s, L1 misses/op of 5.228/6.094, and HITM/op of
1.168/0.835. Padding is faster despite HIGHER measured load-HITM/op.
Report this as observed. The event counts a particular category of loads across
the workload, not every coherence handoff on the lock lines.

## Assembly

Keep the full report within eight pages including Parts 1–4. Select main
multi-panel plots and compact counter tables; 18 exported figures are an
analysis set, not 18 required report pages. Add predictions, conceptual
explanations, hardware/protocol details, source-matched TSan summary lines,
and Parts 1–4. This notebook is not the final report PDF.
