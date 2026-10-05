"""Plot the provided coarse.csv; needs Python 3 and matplotlib."""

import argparse
import csv
import math
import os
from pathlib import Path
import tempfile

# Keep plotting caches writable on laptops and compute nodes.
os.environ.setdefault('MPLCONFIGDIR', tempfile.mkdtemp(prefix='lab1-mpl-'))
import matplotlib

matplotlib.use('Agg')
import matplotlib.pyplot as plt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('csv', type=Path)
    parser.add_argument('--machine', choices=['frontera', 'ls6'], default='ls6')
    args = parser.parse_args()
    if args.machine == 'ls6':
        cores, boundary, after_socket, oversubscribed = 128, 64, 96, 192
        expected = [1, 2, 4, 8, 16, 32, 64, 96, 128, 192, 256]
        machine_name = 'Lonestar6'
    else:
        cores, boundary, after_socket, oversubscribed = 56, 28, 40, 84
        expected = [1, 2, 4, 8, 16, 28, 40, 56, 84, 112]
        machine_name = 'Frontera'
    with args.csv.open(newline='') as source:
        reader = csv.DictReader(source)
        if reader.fieldnames != ['threads', 'mops', 'cpus']:
            parser.error('Expected the sweep.sh header: threads,mops,cpus')
        rows = [(int(row['threads']), float(row['mops'])) for row in reader]
    if [t for t, _ in rows] != expected:
        parser.error(f'Expected Part 2 thread counts in order: {expected}')
    if any(not math.isfinite(mops) or mops <= 0 for _, mops in rows):
        parser.error('Every throughput must be finite and positive')

    threads, mops = zip(*rows)
    fig, ax = plt.subplots(figsize=(8, 4.5), layout='constrained')
    ax.plot(threads, mops, 'o-', color='#1565c0', label='CoarseMap (median of 3)')
    ax.axvline(boundary, color='0.65', linestyle=':', label=f'Socket boundary: {boundary} cores')
    ax.axvline(cores, color='0.25', linestyle='--', label=f'Physical cores: {cores}')
    ax.set(xlabel='Worker threads', ylabel='Throughput (Mops/s)',
           title=f'Throughput (Mops/s) vs ThreadCount — {machine_name}',
           xlim=(0, threads[-1] * 1.04), ylim=(0, max(mops) * 1.15))
    ax.set_xticks([1, expected[4], boundary, after_socket, cores, oversubscribed, threads[-1]])
    ax.grid(axis='y', alpha=0.2)
    ax.legend(frameon=False)
    for extension in ('png', 'svg'):
        output = args.csv.with_suffix('.' + extension)
        fig.savefig(output, dpi=180)
        print(f'Wrote {output}')
    plt.close(fig)

    peak = max(value for _, value in rows)
    peak_threads = ', '.join(str(t) for t, value in rows if value == peak)
    values = dict(rows)
    matches = values[1] == peak or values[2] == peak
    summary = (
        '# Part 2: measured comparison\n\n'
        f'The peak median throughput is {peak:g} Mops/s at T={peak_threads}. '
        f'The predicted peak at one or two threads {"matches" if matches else "does not match"} '
        'the measured maximum.\n\n'
        f'Throughput at T={threads[-1]} is {values[threads[-1]] / values[1]:.3f} times the T=1 value. '
        f'Across the sampled socket boundary, T={boundary} to T={after_socket}, it changes from '
        f'{values[boundary]:g} to {values[after_socket]:g} Mops/s '
        f'({100 * (values[after_socket] / values[boundary] - 1):+.1f}%). '
        f'From T={cores} to T={oversubscribed} it changes from {values[cores]:g} to '
        f'{values[oversubscribed]:g} Mops/s ({100 * (values[oversubscribed] / values[cores] - 1):+.1f}%).\n\n'
        'These sampled changes do not establish an abrupt transition or its cause. '
        'Use sweep.log to inspect the three-run spread before interpreting '
        'small differences. See prediction.md for the hypotheses recorded before '
        'the sweep and lscpu.txt and sweep.log for the machine and pinning protocol.\n'
    )
    args.csv.with_name('comparison.md').write_text(summary)


if __name__ == '__main__':
    main()
