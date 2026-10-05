# Lab 1 submission audit

Reviewed against README.md, Deliverables, Checklist, and the questions in Parts
1–7. The supplied final `Lab1 Report.pdf` is preserved byte-for-byte. It has
**8 pages**, as counted by PDFKit; all eight pages were rendered and inspected.
This checklist is a supporting file, not an extra page added to the report.

## Package contents

- Final report, including the contribution paragraph on page 8.
- All four required headers in `starter_files/`: `concurrent_map.h`, `locks.h`,
  `hash_map.h`, `parts.h`. All four HAVE_* switches are 1.
- Supporting interface, benchmark, Makefile, sweep/perf scripts, and tests,
  preserving a buildable layout (`make -C starter_files test`).
- `raw_measurements/`: report measurement CSVs, individual benchmark repetitions,
  per-operation counter inputs with operation counts, machine/affinity metadata,
  and normal/TSan validation. INDEX.csv records provenance and SHA256SUMS checks
  the archived copies.
- Part 4/5 submission CSVs retain the requested part-specific names. The ZIP also
  includes byte-identical canonical `sharded_mutex.csv` aliases in separate
  original directories, matching the harness filenames required by README.
- Part 5 relaxed-ordering logs, status files, and experimental diff. This
  intentionally incorrect experimental lock is NOT the submitted locks.h.
- `PACKAGE_SHA256SUMS` covers the ZIP's report, source, evidence, and checklist.
  Git files, executable build products, caches, and source tarballs are excluded.

## Required answers and figures

| Requirement | Report location | Review |
|---|---|---|
| Part 1: size() synchronization; copies instead of references/iterators | p. 1 | Present |
| Part 2: prediction, measured linear coarse curve, core/socket markers, comparison with prediction, socket-boundary and oversubscription observations | p. 1 | Present for approved LS6 measurements |
| Part 3: contended line, MESI states, handoff estimate, why worse rather than flat | pp. 1–2 | Present; lecture estimate explicitly qualified |
| Part 3: T=1/8/28 Mops/s, cycles/op, instructions/op, IPC, L1/LLC misses/op, HITM/op, context switches/op, operation counts | p. 2 | Present in embedded table and footnote |
| Part 3: T=1 tree/cache explanation, growth factors, HITM versus handoffs, context switches | p. 2 | Present, including numerical lecture L2 versus true-sharing comparison |
| Part 4: exact size(), deadlock order versus another size() and insert, approximate size() | p. 2 | Present |
| Part 4: matched coarse/sharded curve; shard search at T=1 and T=32; chosen N and numerical collision argument | pp. 2–3 | Present; fitted penalty explicitly qualified |
| Part 5: correctness; relaxed-ordering experiment outcomes and acquire/release explanation | p. 3 | Present; raw evidence included |
| Part 5: five-lock sweep beyond core count; prediction of misses and instructions | pp. 3–4 | Present |
| Part 5: one-shard table, TAS/TTAS MESI/traffic explanation, spin hint, ticket yield | p. 4 | Present |
| Part 5: 32-on-8 table, busy fraction and context switches, different oversubscription behavior | p. 4 | Present; assumed 3.5 GHz explicitly disclosed |
| Part 5: parking versus mutex at one thread/core and beyond | pp. 4–5 | Present; explanation of why implementations differ is limited |
| Part 5: redo TTAS/parking shard search and collision argument | p. 5 | Present, including fitted penalty/product versus base cost |
| Part 6: four locks, three mixes, N=1/chosen N; when RW wins/loses and entry/exit costs | p. 5 | Present |
| Part 6: std::shared_mutex and waiting-writer behavior; reader/writer/total effect of writer preference | pp. 5–6 | Present; exact library policy appropriately not claimed from aggregate throughput |
| Part 7: bucket/stripe mapping and memory shared by different stripes | p. 6 | Present |
| Part 7: single-thread hash/tree crossover; prediction and measured L1-miss slope versus chain length; cost and cache level | pp. 6–7 | Present with operation counts and latency qualification |
| Part 7: TTAS/parking stripe search, chosen L, redone numerical collision argument | p. 7 | Present, including fitted penalty/product versus base cost |
| Part 7: locks/line prediction, padded/unpadded tree and hash sweep, largest differences, bounds, spread | pp. 7–8 | Present; tree spread only described as overlapping, not stated numerically |
| Part 7: Frontera one-socket T=28/L=1024 HITM and L1 table, relation to throughput, lock-time interpretation | p. 8 | Present; saved configuration confirms even CPUs only, one socket |
| TSan summary lines for both programs with all parts enabled | p. 8 | Present in screenshot; full source-matched text logs included |
| Machine lscpu, pin order, socket boundary | p. 8 and raw evidence | Present; distinguish numeric-order Part 3 from even-CPU Part 5/7 affinity |
| Who did what | p. 8 | Present |
| Eight-page maximum | Entire PDF | Met: exactly 8 pages |

## Audit findings and updated report status

The updated report remains eight pages. Items 1, 3, 4, 5, and 6 below are now
addressed in the supplied PDF. Items 2, 7, and 8 remain as qualifications or
possible improvements; the report is preserved unchanged when packaging.

1. **Addressed in updated PDF.** **CPU model typo (p. 1):** LS6 logs say AMD EPYC **7763**, not 7753.
2. **Machine interpretation (p. 1):** replace the assertion that CPU similarity
   lets counters support the curve with a statement that Frontera counters
   illustrate the same contention mechanism but do not quantitatively measure
   the LS6 curve. User confirmed approval to use LS6; no replacement LS6 runs
   are required by this audit.
3. **Addressed in updated PDF.** **Part 3 latency comparison (p. 2):** the README specifically asks to compare
   what an L2-served miss costs with obtaining a line Modified in another core.
   The updated report includes the L2 number. Lecture 6's local
   L2 measurement was about 4.26 ns/14 cycles, versus the cited roughly
   12 ns/40 cycles same-socket true-sharing estimate. Treat these as lecture
   illustrations rather than measured costs of these mutex operations.
4. **Addressed in updated PDF.** **Part 5 collision terms (p. 5):** supply numerical collision penalty and
   probability-times-penalty versus the uncontended cost, as required by the
   redo of Part 4. With the report's independent uniform-choice approximation
   p=0.0075407, the saved TTAS rates imply 189.1 ns base, 316.4 ns worker time,
   127.3 ns excess, and a fitted 16.9 microsecond penalty; p times that penalty
   is 127.3 ns, about 67.3% of base. Parking gives 186.7 ns base, 319.3 ns worker
   time, 132.6 ns excess, fitted 17.6 microseconds, and 71.0% of base.
   These are fitted total-overhead estimates, NOT isolated mutex wait times.
5. **Addressed in updated PDF.** **Part 7 collision terms (p. 7):** using the same qualified fitted model and
   the saved L=4096 rates, TTAS gives 60.28 ns base, 130.94 ns worker time,
   70.66 ns excess, fitted 9.37 microseconds, and p times penalty equal to
   117.2% of base. Parking gives 60.81 ns base, 113.47 ns worker time,
   52.66 ns excess, fitted 6.98 microseconds, and 86.6% of base. The updated report now includes these numerical terms.
6. **Addressed in updated PDF.** **Frontera affinity (p. 8):** Part 3 used CPU-number order, so T=8 and T=28
   already span sockets. Part 5 contention used CPUs 0,2,...,14; Part 7 padding
   used 0,2,...,54, both entirely on socket 0. These two experiments satisfy
   the one-socket condition. A numeric-order sweep on this topology has no
   socket transition at T=28, even if sweep.sh prints "first socket full".
7. **Parking explanation (p. 5):** the README asks why parking differs from
   mutex. The report describes spin/wait/notify and the measured difference,
   but can more explicitly connect short spinning to CPU work, sleeping to
   relinquishing a CPU, and wakeup/reacquisition costs under contention.
8. **Tree padding spread (p. 8):** give its numerical min–max ranges as well as
   saying they overlap; raw padding_sweep.log contains all three repetitions.

## Validation performed for packaging

- Checked all required submission headers exist and switches equal 1.
- Compared every archived evidence file with its original source bytes/hash.
- Saved all-parts normal and TSan logs from ls6-20261004-011909 match current
  submitted headers, harness, and standard tests by source-checksums.txt.
  Both TSan programs end in `all checks passed`. The relaxed experiment's
  TSan failures are separately labeled intentional evidence.
- Preserved report PDF bytes and rendered/read all eight pages. The reader/writer
  table spans pages 5–6 and some figure labels are small, but no missing page or
  overlapping/clipped body text was observed.
- No source changes or new benchmark results were introduced by packaging.
- ZIP CRC check and byte-for-byte entry verification are performed by build_zip.py.
