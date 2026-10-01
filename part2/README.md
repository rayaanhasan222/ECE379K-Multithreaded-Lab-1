# Part 2 workflow

For Lonestar6, use a full 128-core CPU compute node. Run
`bash part2/run.sh ls6`, then
`python3 part2/plot.py part2/results/coarse.csv --machine ls6`.
This uses threads `1 2 4 8 16 32 64 96 128 192 256` and marks 64 and 128
on the plot. The original README specifies Frontera for graded measurements;
Lonestar6 results should identify the different machine explicitly.

The prediction is recorded in `prediction.md`. Measurement is still required
on one full Frontera compute node (56 physical cores, two sockets, no SMT).

Copy this lab to Frontera and enter a full-node interactive allocation using
your normal allocation/account settings. On the compute node, from the lab root:

```bash
bash part2/run.sh
```

The runner builds the benchmark and invokes the provided, unchanged sweep:

```bash
make sweep IMPLS=coarse SHARDS=1 THREADS="1 2 4 8 16 28 40 56 84 112"
```

It saves `coarse.csv` in `starter_files` and copies it to `part2/results`.
The results directory also contains the prediction copied before the run,
the start time, hostname, allocation ID, `lscpu` output, CPU topology, build
log, and sweep log. The sweep log records the pin order, socket boundary,
and the three throughput samples behind every median. Existing results are
never overwritten by the runner. Do not run another sweep simultaneously.

Plot on the compute node or copy the results back to your laptop:

```bash
python3 part2/plot.py part2/results/coarse.csv
```

The plotting script requires matplotlib and creates `coarse.png`,
`coarse.svg`, and `comparison.md` beside the CSV. Both axes are linear;
the plot marks the socket boundary at 28 and core count at 56. Inspect
the comparison and the run-to-run spread when writing your report.
The comparison describes observed changes; Part 3 supplies their explanation.

No measured CSV or throughput plot is included until the Frontera run is done.
