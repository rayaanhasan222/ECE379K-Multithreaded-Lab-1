# Raw measurements used in the report

This is a byte-for-byte copy of the measurement inputs selected for the current
report draft. Original filenames and repository-relative paths are preserved
to avoid mixing machines, shard counts, or repeated filenames.

Included:
- Part 2 coarse CSV and raw sweep repetitions.
- Part 3 coarse counter output at T=1, 8, and 28.
- Part 4 matched coarse/sharded CSVs and raw sweep, plus the same-node shard
  search used by Parts 4–5.
- Part 5 N=4096 five-lock sweep, N=4096 oversubscription counters, and all
  three Frontera contention repetitions.
- Part 6 N=1/N=4096 mix sweeps and separate-role measurements.
- Part 7 N=L=4096 bucket measurements and matching tree baseline, stripe
  search, N=L=1024 padding sweeps, lock-size output, and all three Frontera
  padding-counter repetitions.

The .bench.txt files contain the operation counts needed to normalize their
matching .perf.txt files. Matching .configuration.txt files preserve affinity,
events, duration, and delay. These are measurement metadata, not command history.
Raw sweep/perf output may contain the command that produced it; it is preserved
unchanged rather than stripped.

Excluded: build logs, shell command history, scripts, binaries, source archives,
generated plots/notebooks/summary CSVs, pilot runs, unrelated earlier measurements,
and supplementary N=256 results not used by the draft. Test/TSan evidence and
machine-environment logs remain in their original folders and should be supplied
separately if needed for submission.

INDEX.csv maps every copied input to its report use and original location.
SHA256SUMS verifies the copies. Neither file modifies the source data.

The Part 3 source files do not contain HITM results. This folder preserves the
available evidence; copying it does not fill that missing measurement.
