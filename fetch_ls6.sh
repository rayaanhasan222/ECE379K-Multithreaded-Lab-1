#!/usr/bin/env bash
# Fetch measurement artifacts into the same local paths, excluding source code.
set -euo pipefail

lab_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
source_dir='tud@ls6.tacc.utexas.edu:/home1/11856/tud/Lab1/'
options=(--itemize-changes)
dry_run=false
if [[ ${1:-} == --dry-run && $# == 1 ]]; then
    options+=(--dry-run)
    dry_run=true
elif [[ $# != 0 ]]; then
    echo 'Usage: bash fetch_ls6.sh [--dry-run]' >&2
    exit 1
fi

command -v rsync >/dev/null || { echo 'rsync is required.' >&2; exit 1; }
backup_dir=".ls6-backups/$(date '+%Y%m%d-%H%M%S')-$$"

# Transfer result folders in full (including prediction/comparison markdown)
# and standalone output files. Directory traversal preserves the lab layout.
# No --delete: files that exist only locally remain untouched.
rsync -rltv --checksum --prune-empty-dirs --backup \
    --backup-dir="$backup_dir" "${options[@]}" \
    --exclude='.git/' \
    --exclude='.ls6-backups/' \
    --exclude='.DS_Store' \
    --exclude='*.dSYM/' \
    --exclude='__pycache__/' \
    --exclude='*.h' --exclude='*.cpp' --exclude='*.c' \
    --exclude='*.py' --exclude='*.sh' --exclude='Makefile' \
    --exclude='core' --exclude='core.*' \
    --include='*/' \
    --include='**/results*/***' \
    --include='*.csv' --include='*.tsv' --include='*.log' \
    --include='*.txt' --include='*.json' \
    --include='*.png' --include='*.svg' --include='*.pdf' \
    --exclude='*' \
    "$source_dir" "$lab_dir/"

if [[ "$dry_run" == false ]]; then
    printf '\nDownloaded results into %s\n' "$lab_dir"
    printf 'Any replaced local files were backed up under %s/%s\n' "$lab_dir" "$backup_dir"
fi
