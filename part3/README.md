# Part 3 report table

From the lab root, run:

```bash
./part3/generate_table.py part3/results_frontera
```

Python 3 and matplotlib are required. The results argument is optional and
defaults to `part3/results_frontera`, relative to the script's location.

The generator reads `coarse_T1.txt`, `coarse_T8.txt`, and `coarse_T28.txt`.
It validates the workload and required counters, rejects measurement warnings,
and recomputes per-operation values from raw counts and operation totals.
It writes these files alongside the logs:

- `part3_table.png`: a 300-dpi table ready to insert into the report.
- `part3_table.svg`: the same table in vector format.
- `part3_table.csv`: full-precision numeric values for further analysis.
- `part3_table.md`: the table and measurement notes in Markdown.
- `ratios.txt`: the T=8/T=1 comparisons and per-thread HITM, switching, and IPC.

The table image rounds values for readability and includes the operation
counts in its footnote. The underlying CSV retains unrounded calculations.

The Frontera bundle contains the three 30-second runs from node `c208-013`,
with a 222-ms counter-start delay and 123-124-ms warm-ups, plus machine,
compiler, perf, event-availability, and build records. The small delay after
warm-up means counters omit about 0.33% of the timed run while normalization
uses all reported operations. Keep that measurement-window limitation in
the report. The original Lonestar6 runs remain in `part3/results`.

Use the Frontera rows together; do not mix them with the older Lonestar6
counter rows. HITM counts qualifying loads, not every ownership transfer,
and cycles/op aggregates CPU cycles across threads rather than measuring
one operation's elapsed latency. A handoff-cost estimate still needs the
coherence-session results or a separate ownership-transfer experiment.
