"""Plot coarse/sharded sweeps and the median shard-count measurements."""

import argparse
import csv
import math
import os
from pathlib import Path
import re
import statistics
import tempfile

os.environ.setdefault('MPLCONFIGDIR', tempfile.mkdtemp(prefix='lab1-mpl-'))
import matplotlib

matplotlib.use('Agg')
import matplotlib.pyplot as plt


def read_sweep(path):
    with path.open(newline='') as source:
        rows = [(int(row['threads']), float(row['mops']))
                for row in csv.DictReader(source)]
    if (not rows or any(t <= 0 or not math.isfinite(m) or m <= 0 for t, m in rows)
            or [t for t, _ in rows] != sorted({t for t, _ in rows})):
        raise ValueError(f'{path}: expected increasing, unique threads and positive throughput')
    return rows


def save(fig, folder, name):
    for extension in ('png', 'svg'):
        path = folder / f'{name}.{extension}'
        fig.savefig(path, dpi=180)
        print(f'Wrote {path}')
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('results', type=Path)
    parser.add_argument('--machine', choices=['ls6', 'frontera'], default='ls6')
    args = parser.parse_args()
    folder = args.results
    cores, boundary = (128, 64) if args.machine == 'ls6' else (56, 28)
    try:
        coarse = read_sweep(folder / 'coarse.csv')
        sharded = read_sweep(folder / 'sharded_mutex.csv')
        if [t for t, _ in coarse] != [t for t, _ in sharded]:
            raise ValueError('The coarse and sharded sweeps must use the same thread counts')
        if boundary not in dict(sharded) or cores not in dict(sharded) or sharded[-1][0] <= cores:
            raise ValueError('The sweep must sample the socket/core boundaries and oversubscription')

        samples = {(t, n): [] for t in (1, 32) for n in (16, 64, 256, 1024, 4096)}
        pattern = re.compile(
            r'^sharded:mutex\s+T=(\d+)\s+shards=(\d+).*?([\d.]+)\s+Mops/s\s*$')
        for line in (folder / 'shard_counts.txt').read_text().splitlines():
            match = pattern.match(line)
            if match:
                t, n = map(int, match.group(1, 2))
                value = float(match.group(3))
                if (t, n) not in samples or not math.isfinite(value) or value <= 0:
                    raise ValueError(f'Unexpected shard-count sample: {line}')
                samples[t, n].append(value)
        if any(len(values) != 3 for values in samples.values()):
            raise ValueError('Expected three shard-count measurements for every T/N pair')
    except (OSError, ValueError, KeyError) as error:
        parser.error(str(error))

    fig, ax = plt.subplots(figsize=(8, 4.5), layout='constrained')
    for rows, label in ((coarse, 'CoarseMap'), (sharded, 'ShardedMap: mutex, N=256')):
        ax.plot(*zip(*rows), 'o-', label=label)
    ax.axvline(boundary, color='0.65', linestyle=':', label=f'Socket boundary: {boundary}')
    ax.axvline(cores, color='0.25', linestyle='--', label=f'Physical cores: {cores}')
    ax.set(xlabel='Worker threads', ylabel='Throughput (Mops/s)',
           title=f'Coarse versus sharded — {args.machine}', ylim=(0, None))
    ax.grid(axis='y', alpha=0.2)
    ax.legend(frameon=False)
    save(fig, folder, 'coarse_vs_sharded')

    counts = [16, 64, 256, 1024, 4096]
    fig, ax = plt.subplots(figsize=(8, 4.5), layout='constrained')
    with (folder / 'shard_counts.csv').open('w', newline='') as output:
        writer = csv.writer(output)
        writer.writerow(['threads', 'shards', 'median_mops', 'min_mops', 'max_mops'])
        for t in (1, 32):
            medians = [statistics.median(samples[t, n]) for n in counts]
            low = [m - min(samples[t, n]) for n, m in zip(counts, medians)]
            high = [max(samples[t, n]) - m for n, m in zip(counts, medians)]
            ax.errorbar(counts, medians, yerr=[low, high], fmt='o-', capsize=4, label=f'T={t}')
            for n, m in zip(counts, medians):
                writer.writerow([t, n, m, min(samples[t, n]), max(samples[t, n])])
    ax.set_xscale('log', base=2)
    ax.set_xticks(counts, labels=[str(n) for n in counts])
    ax.set(xlabel='Shard count N', ylabel='Throughput (Mops/s)',
           title='Shard count: median and min–max of three runs', ylim=(0, None))
    ax.grid(axis='y', alpha=0.2)
    ax.legend(frameon=False)
    save(fig, folder, 'shard_counts')


if __name__ == '__main__':
    main()
