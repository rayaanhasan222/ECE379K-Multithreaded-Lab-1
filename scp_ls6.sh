#!/usr/bin/env bash
# Copy an explicit list of lab sources and workflow files, never Git or results.
set -euo pipefail

lab_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
destination='tud@ls6.tacc.utexas.edu:/home1/11856/tud/'
files=(
    README.md
    scp_ls6.sh
    starter_files/Makefile
    starter_files/bench.cpp
    starter_files/interface.h
    starter_files/parts.h
    starter_files/concurrent_map.h
    starter_files/locks.h
    starter_files/hash_map.h
    starter_files/sweep.sh
    starter_files/perfstat.sh
    tests/test_map.cpp
    tests/test_locks.cpp
    part2/README.md
    part2/prediction.md
    part2/run.sh
    part2/plot.py
    part4/README.md
    part4/plot.py
)

for file in "${files[@]}"; do
    if [[ ! -f "$lab_dir/$file" ]]; then
        echo "Missing required file: $file" >&2
        exit 1
    fi
done

if [[ ${1:-} == --dry-run && $# == 1 ]]; then
    printf 'Files to copy into %sLab1/:\n' "$destination"
    printf '  %s\n' "${files[@]}"
    exit 0
elif [[ $# != 0 ]]; then
    echo 'Usage: bash scp_ls6.sh [--dry-run]' >&2
    exit 1
fi

staging_dir=$(mktemp -d "${TMPDIR:-/tmp}/lab1-scp.XXXXXX")
trap 'rm -rf -- "$staging_dir"' EXIT
for file in "${files[@]}"; do
    mkdir -p "$staging_dir/Lab1/$(dirname -- "$file")"
    cp "$lab_dir/$file" "$staging_dir/Lab1/$file"
done

scp -r "$staging_dir/Lab1" "$destination"
printf '\nCopied sources to /home1/11856/tud/Lab1/ on LS6.\n'
printf 'Existing remote results are preserved. Rebuild on the compute node.\n'
