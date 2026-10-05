# Raw measurements used in the report

This is a byte-for-byte copy of the measurement inputs selected for the current
report draft, with supporting measurement metadata and validation evidence.
Repository-relative paths are preserved. The two mutex sweep CSVs are named
`sharded_mutex_part_4.csv` and `sharded_mutex_part_5.csv` in their respective
Part 4 and Part 5 directories; original source files are unchanged.

Included:
- Part 1 correctness and ThreadSanitizer evidence: `part4/results/test.log`
  and `part4/results/tsan.log` contain the saved checks of the coarse and
  sharded implementations. Part 1 has no separate benchmark CSV.
- Part 2 coarse CSV, raw sweep repetitions, and machine/affinity metadata.
- Part 3 Frontera coarse counter output at T=1, 8, and 28, including HITM,
  with measurement settings, environment, and the derived `part3_table.csv`.
  Earlier Lonestar6 counters in `part3/results/` are supplementary and do
  not contain HITM; the report table uses `part3/results_frontera/`.
- Part 4 matched coarse/sharded CSVs and raw sweep, plus the same-node shard
  search used by Parts 4–5. `part4/results/shard_counts.txt` contains the
  repetitions behind the derived `shard_counts.csv` summary (best tested:
  4,096 shards at T=32). The Part 4 thread sweep uses 256 shards; the Part 5
  mutex sweep uses 4,096 shards, so they are distinct datasets.
- Part 5 N=4096 five-lock sweep, N=4096 oversubscription counters, and all
  three Frontera contention repetitions, plus the relaxed-ordering experiment
  logs/statuses and its experimental diff.
- Part 6 N=1/N=4096 mix sweeps and separate-role measurements.
- Part 7 N=L=4096 bucket measurements and matching tree baseline, stripe
  search, N=L=1024 padding sweeps, lock-size output, and all three Frontera
  padding-counter repetitions.
- Machine, affinity, configuration, and source-checksum metadata from each
  selected partner measurement archive, and source-matched all-parts normal
  and TSan validation from `measurements/ls6-20261004-011909/`.

The .bench.txt files contain the operation counts needed to normalize their
matching .perf.txt files. Matching .configuration.txt files preserve affinity,
events, duration, and delay. These are measurement metadata, not command history.
Raw sweep/perf output may contain the command that produced it; it is preserved
unchanged rather than stripped.

Excluded: build logs, shell command history, scripts, binaries, source archives,
generated plots/notebooks, pilot runs, and unrelated earlier measurements.
The two explicitly identified Part 3/4 summary CSVs accompany their raw inputs;
all original copies remain available in their source folders.

INDEX.csv maps every copied input to its report use and original location,
including separate original and copied paths for the renamed mutex CSVs.
SHA256SUMS verifies the copies. Neither file modifies the source data.

The submission ZIP also provides canonical `sharded_mutex.csv` aliases in the
original Part 4/5 directory paths to satisfy the README's harness-filename
requirement. These aliases are byte-identical to the part-specific copies and
are covered by the ZIP's `PACKAGE_SHA256SUMS`.
