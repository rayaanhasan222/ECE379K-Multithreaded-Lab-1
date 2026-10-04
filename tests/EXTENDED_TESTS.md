# Additional local validation

From the repository root:

```sh
python3 tests/run_extended.py
```

This compiles and runs the original `test_map.cpp`, the original
`test_locks.cpp`, and the additional `test_extended.cpp` in three modes:

- Normal optimized build.
- ThreadSanitizer (TSan).
- AddressSanitizer plus UndefinedBehaviorSanitizer (ASan/UBSan).

Each compilation and test has a 180-second limit. The runner stops at the
first failure or timeout and prints the log location. Binaries, debug
artifacts, and complete logs go to a fresh temporary directory by default.
It does not change the provided tests, Makefile, benchmark, or implementations.

On a Lonestar6 compute node, after loading GCC, you can instead use:

```sh
python3 tests/run_extended.py --cxx g++ --timeout 600 \
  --output validation/extended-ls6
```

The output directory must not already exist, to avoid overwriting evidence.
Select a single mode with `--modes tsan` or `--modes asan`. Run test programs
sequentially and allow enough allocation time for the requested modes.
The existing transfer script's allowlist does not include these new files;
copy them into the remote `tests/` directory before using this runner.

## Additional coverage

- CoarseMap plus both sharded/hashed maps with eight lock types and both
  padding settings: 33 combinations.
- For each combination, 6,000 seeded operations compared against std::map,
  including negative keys, overwrites, missing keys, const lookups, and size.
- Eight threads repeatedly inserting and erasing the same key: exactly one
  successful new insertion and one successful removal per phase.
- 24 concurrent histories per combination, with three threads performing
  three operations each: 792 histories total. An independent sequential model
  searches possible orderings consistent with observed real-time precedence
  and checks return values, lookup outputs, exact size, and final contents.
- The history checker itself is checked against known valid and invalid cases.
- Hash-table head/middle/tail deletion, one bucket, uneven bucket/stripe counts,
  more stripes than buckets, and zero-count rejection. Nine extra configurations
  each also run the 6,000-operation differential check.
- Non-default-constructible values, exceptions during node copying, overwrites
  and lookup output assignment, successful reuse afterward, and a live-object
  counter that checks all values are destroyed when each map is destroyed.
- Both custom reader-writer locks must admit a second reader while an existing
  reader still holds the lock (five-second scheduling deadline).
- Compile-time checks that lock types cannot be copied or moved.

## Limits

These tests do not explore all schedules or prove starvation freedom. The
history checker exhaustively considers legal orders only for each captured
small history, not all executions of the program. A clean TSan run means no
race was reported in that run, not that races are impossible. ASan/UBSan and
the explicit value-lifetime checks complement TSan; platform leak-sanitizer
availability varies. Local correctness timings are not lab throughput data.

The relaxed-memory-order experiment, Frontera/Lonestar6 measurements,
counter analysis, and report are separate tasks and are not performed here.
