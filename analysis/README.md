# Parts 5–7 plots

Open `parts5_7.ipynb` in Jupyter or VS Code. The saved notebook includes executed
outputs. Select a Python environment with the dependencies below and use **Run
All** to reproduce the figures. Start from this directory or the repository root.

```sh
python3 -m pip install -r analysis/requirements.txt
```

Run that installation command from the repository root, preferably in a virtual
environment. Exact package versions used for the saved execution are recorded in
`tables/environment.json`.

The notebook produces 18 figures, each as PDF, SVG, and PNG, under `figures/`.
PDF and SVG are suitable for the report; PNG is convenient for quick review.
`tables/figure_index.csv` lists their captions. Parsed data, summary tables, fits,
and source paths/checksums are under `tables/`.

| Part | Figures |
|---|---|
| 5 | Five-lock scaling; LS6 and Frontera contention counters; oversubscription and CPU use; shard search; collision model and worker-time estimate |
| 6 | Six-condition mix/shard matrix; performance relative to TTAS; independent high-thread-count repeat; separate reader/writer throughput |
| 7 | Bucket crossover; chain length and counter slopes; stripe search; padding throughput; relative padding effect; separate Frontera and LS6 padding counters |

The relaxed-ordering experiment is represented as an outcome table, not an
invented numerical plot. The notebook also exports counter tables for the report.

## Interpretation

- N=4096 is the main comparison setting and best tested at T=32, not a
  proven global optimum. Earlier N=256 data remains supplementary. Padding
  uses 1024 shards/stripes. Searches show every tested count through 4096.
- Error bands/bars are observed min–max ranges of three runs, not confidence
  intervals. Single-run counter measurements have no artificial error bars.
- Original and follow-up Part 6 results are kept separate. The source-selection
  explanation identifies which run supplies each primary panel.
- The professor approved LS6 for the main measurements; HITM uses Frontera.
  Frontera HITM results and LS6 counters remain separate datasets. Each counter
  is normalized by the operation count from its own run.
- Printed zero rates are rounded, not proof of zero progress. Raw benchmark
  throughput is recomputed from operations and measured milliseconds where saved.
- Busy fraction uses task-clock and perf elapsed time, rather than a nominal CPU
  frequency. Hardware-counter multiplexing and delayed counting are documented.
- Mean chain length uses an approximate resident population of 524,288. Fitted
  slopes describe the measured data and are not private predictions or direct
  measurements of isolated cache-miss latency.
- The plots preserve the observed Frontera result: padding improves throughput
  while load-HITM/op increases. They do not force the results to match a prediction.

Use selected panels in the eight-page report; the remaining figures and CSVs
provide supporting analysis. The notebook does not modify measurement files,
source headers, the Makefile, or the professor's scripts.

See [report notes](report_notes.md) for measured findings and remaining requirements.
Browse all figures in [the gallery](figures/index.html).
Run from the lab root with: python3 analysis/run_notebook.py
