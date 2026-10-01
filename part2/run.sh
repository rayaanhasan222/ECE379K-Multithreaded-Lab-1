#!/usr/bin/env bash
# Run inside an interactive allocation on one full compute node.
set -euo pipefail
task_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
lab_dir=$(cd -- "$task_dir/.." && pwd)
result_dir="$task_dir/results"
machine=${1:-frontera}
case "$machine" in
    frontera) cores=56; socket_cores=28; threads="1 2 4 8 16 28 40 56 84 112" ;;
    ls6) cores=128; socket_cores=64; threads="1 2 4 8 16 32 64 96 128 192 256" ;;
    *) echo 'Usage: bash part2/run.sh [frontera|ls6]' >&2; exit 1 ;;
esac

if [[ $(uname -s) != Linux || -z ${SLURM_JOB_ID:-} ]]; then
    echo 'Run this script inside a full compute-node allocation.' >&2
    exit 1
fi

# Verify the topology and the CPU-number pin order used by sweep.sh.
python3 - "$cores" "$socket_cores" <<'PY'
import os
import subprocess
import sys

expected_cores, socket_cores = map(int, sys.argv[1:])

rows = subprocess.check_output(
    ['lscpu', '-p=CPU,CORE,SOCKET'], text=True
)
cpus = sorted(tuple(map(int, row.split(','))) for row in rows.splitlines()
              if row and not row.startswith('#'))
cores = {(socket, core) for _, core, socket in cpus}
packages = [socket for _, _, socket in cpus]
if (len(cpus) != expected_cores or len(cores) != expected_cores
        or packages != [0] * socket_cores + [1] * socket_cores):
    raise SystemExit(f'Expected {expected_cores} cores, no SMT, socket 0 then socket 1 in CPU order.')
if {cpu for cpu, _, _ in cpus} != set(os.sched_getaffinity(0)):
    raise SystemExit(f'The allocation must expose all {expected_cores} CPUs to this process.')
PY

if [[ -e "$result_dir" || -e "$lab_dir/starter_files/coarse.csv" ]]; then
    echo 'Existing results or coarse.csv found; archive them before a new sweep.' >&2
    exit 1
fi
mkdir -p "$result_dir"
cp "$task_dir/prediction.md" "$result_dir/prediction.md"
date -u '+%Y-%m-%dT%H:%M:%SZ' > "$result_dir/started_at.txt"
hostname > "$result_dir/hostname.txt"
lscpu > "$result_dir/lscpu.txt"
lscpu -p=CPU,CORE,SOCKET > "$result_dir/cpu_topology.csv"
printf 'SLURM_JOB_ID=%s\n' "$SLURM_JOB_ID" > "$result_dir/allocation.txt"

cd "$lab_dir/starter_files"
# Force a native rebuild in case scp included a laptop binary.
make -B bench 2>&1 | tee "$result_dir/build.log"
# Preserve the provided harness: pinned, default mix, median of three runs.
env -u MIX -u WRITERS -u BUCKETS make sweep IMPLS=coarse SHARDS=1 \
    THREADS="$threads" \
    2> >(tee "$result_dir/sweep.log" >&2) | tee "$result_dir/sweep.stdout.log"
cp coarse.csv "$result_dir/coarse.csv"
printf 'Sweep saved to %s\n' "$result_dir"
printf 'Plot with: python3 "%s/plot.py" "%s/coarse.csv" --machine %s\n' "$task_dir" "$result_dir" "$machine"
