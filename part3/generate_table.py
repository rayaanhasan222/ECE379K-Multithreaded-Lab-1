#!/usr/bin/env python3
"""Generate CSV, Markdown, PNG/SVG tables and ratios; images need matplotlib."""

import argparse
import csv
import math
import os
from pathlib import Path
import re
import tempfile


EVENTS = {
    "cycles": "cycles_per_op",
    "instructions": "instructions_per_op",
    "L1-dcache-load-misses": "L1_misses_per_op",
    "cache-misses": "LLC_misses_per_op",
    "mem_load_l3_hit_retired.xsnp_hitm": "HITM_per_op",
    "context-switches": "context_switches_per_op",
}
COLUMNS = [
    "threads", "operations", "Mops_per_second", "cycles_per_op",
    "instructions_per_op", "IPC", "L1_misses_per_op", "LLC_misses_per_op",
    "HITM_per_op", "context_switches_per_op",
]


def read_run(folder, threads):
    path = folder / f"coarse_T{threads}.txt"
    text = path.read_text()
    if re.search(r"<not supported>|<not counted>|WARNING|FATAL|perfstat:.*failed", text):
        raise ValueError(f"{path}: counter error or measurement warning; inspect the log")
    bench = re.findall(
        r"^coarse\s+T=(\d+)\s+shards=1\s+mix=80/10/10\s+"
        r"(\d+)\s+ops\s+(\d+)\s+ms\s+([\d.]+)\s+Mops/s\s*$",
        text, re.MULTILINE,
    )
    if len(bench) != 1 or int(bench[0][0]) != threads:
        raise ValueError(f"{path}: expected one default-mix coarse run at T={threads}")
    _, ops, _, mops = bench[0]
    ops = int(ops)
    if ops <= 0 or float(mops) <= 0:
        raise ValueError(f"{path}: operations and throughput must be positive")

    counts = {}
    for line in text.splitlines():
        fields = line.split()
        if fields and fields[0] in EVENTS:
            event = fields[0]
            if event in counts or len(fields) != 3:
                raise ValueError(f"{path}: duplicate or malformed counter row: {line}")
            count = float(fields[1])
            if not math.isfinite(count) or count < 0:
                raise ValueError(f"{path}: invalid counter count: {line}")
            counts[event] = count
    missing = set(EVENTS) - set(counts)
    if missing:
        raise ValueError(f"{path}: missing counters: {', '.join(sorted(missing))}")
    if counts["cycles"] == 0:
        raise ValueError(f"{path}: cannot compute IPC with zero cycles")

    row = {"threads": threads, "operations": ops, "Mops_per_second": float(mops)}
    # Recompute from raw counts rather than the wrapper's rounded per-op column.
    row.update({label: counts[event] / ops for event, label in EVENTS.items()})
    row["IPC"] = counts["instructions"] / counts["cycles"]
    return row


def render_table(rows, folder):
    # A writable cache is needed on both laptops and cluster nodes.
    os.environ.setdefault("MPLCONFIGDIR", tempfile.mkdtemp(prefix="lab1-table-mpl-"))
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    headings = ["Threads", "Mops/s", "Cycles\n/ op", "Instructions\n/ op", "IPC",
                "L1 misses\n/ op", "LLC misses\n/ op", "HITM\n/ op", "Context switches\n/ op"]
    cells = []
    for row in rows:
        cells.append([
            str(row["threads"]), f'{row["Mops_per_second"]:.1f}',
            f'{row["cycles_per_op"]:,.0f}', f'{row["instructions_per_op"]:,.1f}',
            f'{row["IPC"]:.3f}', f'{row["L1_misses_per_op"]:.2f}',
            f'{row["LLC_misses_per_op"]:.3f}', f'{row["HITM_per_op"]:.4g}',
            f'{row["context_switches_per_op"]:.4g}',
        ])

    fig, ax = plt.subplots(figsize=(10, 3.1))
    fig.subplots_adjust(left=0, right=1, top=1, bottom=0)
    ax.axis("off")
    ax.text(0.02, 0.94, "CoarseMap performance counters - Frontera",
            transform=ax.transAxes, fontsize=15, fontweight="bold", color="#172b4d",
            va="top")
    ax.text(0.02, 0.82, "Default workload: 80% find, 10% insert, 10% erase",
            transform=ax.transAxes, fontsize=11, color="#475569", va="top")
    table = ax.table(
        cellText=cells, colLabels=headings, cellLoc="center", colLoc="center",
        colWidths=[0.08, 0.085, 0.12, 0.14, 0.07, 0.11, 0.11, 0.12, 0.165],
        bbox=[0.015, 0.29, 0.97, 0.45],
    )
    table.auto_set_font_size(False)
    table.set_fontsize(11.5)
    for (r, c), cell in table.get_celld().items():
        cell.set_edgecolor("#cbd5e1")
        cell.set_linewidth(0.6)
        if r == 0:
            cell.set_facecolor("#172b4d")
            cell.set_text_props(color="white", weight="bold", fontsize=10.5)
            cell.set_height(cell.get_height() * 1.35)
        else:
            cell.set_facecolor("#f1f5f9" if r % 2 else "white")
            cell.set_text_props(color="#172b4d")

    ops = ";  ".join(f"T={row['threads']}: {row['operations']:,}" for row in rows)
    footnotes = [
        "Operations: " + ops,
        "Per-op values = raw event counts / operations; IPC = instructions / cycles.",
        "LLC: cache-misses. HITM: mem_load_l3_hit_retired.xsnp_hitm (qualifying loads).",
    ]
    for y, line in zip([0.22, 0.145, 0.07], footnotes):
        ax.text(0.02, y, line, transform=ax.transAxes, fontsize=9.5,
                color="#475569", va="top")
    for extension in ("png", "svg"):
        path = folder / f"part3_table.{extension}"
        fig.savefig(path, dpi=300, facecolor="white")
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "results", nargs="?", type=Path,
        default=Path(__file__).resolve().parent / "results_frontera",
        help="directory containing coarse_T1.txt, coarse_T8.txt, coarse_T28.txt",
    )
    args = parser.parse_args()
    try:
        rows = [read_run(args.results, t) for t in (1, 8, 28)]
    except (OSError, ValueError) as error:
        parser.error(str(error))

    try:
        render_table(rows, args.results)
    except ImportError:
        parser.error("Image export requires matplotlib: python3 -m pip install matplotlib")

    with (args.results / "part3_table.csv").open("w", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=COLUMNS)
        writer.writeheader()
        writer.writerows(rows)

    headings = ["Threads", "Operations", "Mops/s", "Cycles/op", "Instructions/op",
                "IPC", "L1 misses/op", "LLC misses/op", "HITM/op", "Context switches/op"]
    table = "| " + " | ".join(headings) + " |\n"
    table += "| " + " | ".join(["---:"] * len(headings)) + " |\n"
    for row in rows:
        values = [str(row["threads"]), f'{row["operations"]:,}',
                  f'{row["Mops_per_second"]:.1f}']
        values += [f'{row[key]:.6g}' for key in COLUMNS[3:]]
        table += "| " + " | ".join(values) + " |\n"
    caption = (
        "\nCoarseMap on Frontera. Event counts are divided by the full reported "
        "operation count; IPC is instructions/cycles. LLC misses use perf's "
        "cache-misses event, and HITM uses mem_load_l3_hit_retired.xsnp_hitm. "
        "Counters start after the configured delay, so document the small "
        "difference between counting and benchmark windows. Cycles/op sum "
        "cycles across threads and are not operation wall-clock latency.\n"
    )
    (args.results / "part3_table.md").write_text(table + caption)

    one, eight, _ = rows
    summary = f"T=1 L1 misses/op: {one['L1_misses_per_op']:.6g}\n"
    for key, label in [("cycles_per_op", "cycles/op"),
                       ("L1_misses_per_op", "L1 misses/op"),
                       ("instructions_per_op", "instructions/op")]:
        if one[key] == 0:
            summary += f"T=8 / T=1 {label}: undefined (zero T=1 baseline)\n"
        else:
            summary += f"T=8 / T=1 {label}: {eight[key] / one[key]:.6g}x\n"
    for row in rows:
        summary += (f"T={row['threads']}: HITM/op={row['HITM_per_op']:.6g}, "
                    f"context switches/op={row['context_switches_per_op']:.6g}, "
                    f"IPC={row['IPC']:.6g}\n")
    summary += "\nHITM is a load event, not a direct count of every lock handoff.\n"
    (args.results / "ratios.txt").write_text(summary)
    print(table)
    print(summary)
    print(f"Wrote part3_table.csv, part3_table.md, part3_table.png, "
          f"part3_table.svg, and ratios.txt in {args.results}")


if __name__ == "__main__":
    main()
