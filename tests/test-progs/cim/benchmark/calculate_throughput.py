#!/usr/bin/env python3
"""
Calculate normalized throughput (GOps/s) and energy efficiency (GOps/J)
for all benchmark workloads, and generate comparison plots.

Energy accounting:
- WORKLOAD energy = actEnergy + preEnergy + readEnergy + writeEnergy
  (directly caused by memory accesses)
- EXCLUDED: refreshEnergy, actBackEnergy, preBackEnergy
  (scale with time, not workload)
"""

import os
import re
import json
import csv
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

# Constants
RESULTS_DIR = Path(__file__).parent / "results"
N_ELEMENTS = 3000  # Array size for primitive operations

PRIMITIVE_OPS = [
    "rowand",
    "rowadd",
    "rowsub",
    "rowmult",
    "rowmin",
    "rowmax",
    "rowequal",
    "rowgreater",
    "rowgreater_equal",
    "rowif_else",
    "rowabs",
    "rowbitcount",
]

VARIANTS = ["cpu", "gpu", "pim"]
VARIANT_LABELS = {"cpu": "CPU", "gpu": "GPU", "pim": "PIM"}
VARIANT_COLORS = {"cpu": "#4C72B0", "gpu": "#55A868", "pim": "#C44E52"}


def parse_stats_file(stats_path):
    """Extract key statistics from gem5 stats.txt file."""
    stats = {}

    with open(stats_path, "r") as f:
        content = f.read()

    # Use the LAST stats section if multiple exist (ROI stats)
    parts = content.split("---------- Begin Simulation Statistics ----------")
    if len(parts) > 1:
        content = parts[-1]

    # Timing
    match = re.search(r"simSeconds\s+([\d.eE+-]+)", content)
    if match:
        stats["simSeconds"] = float(match.group(1))

    match = re.search(r"simTicks\s+(\d+)", content)
    if match:
        stats["simTicks"] = int(match.group(1))

    match = re.search(r"simOps\s+(\d+)", content)
    if match:
        stats["simOps"] = int(match.group(1))

    match = re.search(r"simInsts\s+(\d+)", content)
    if match:
        stats["simInsts"] = int(match.group(1))

    # DRAM energy components (in picojoules)
    workload_energy_pJ = 0.0
    total_energy_pJ = 0.0
    background_energy_pJ = 0.0

    for rank_id in range(4):
        rp = f"system.mem_ctrl.dram.rank{rank_id}"

        act = re.search(rf"{rp}\.actEnergy\s+([\d.eE+-]+)", content)
        pre = re.search(rf"{rp}\.preEnergy\s+([\d.eE+-]+)", content)
        read = re.search(rf"{rp}\.readEnergy\s+([\d.eE+-]+)", content)
        write = re.search(rf"{rp}\.writeEnergy\s+([\d.eE+-]+)", content)
        refresh = re.search(rf"{rp}\.refreshEnergy\s+([\d.eE+-]+)", content)
        actback = re.search(rf"{rp}\.actBackEnergy\s+([\d.eE+-]+)", content)
        preback = re.search(rf"{rp}\.preBackEnergy\s+([\d.eE+-]+)", content)
        total = re.search(rf"{rp}\.totalEnergy\s+([\d.eE+-]+)", content)

        if act and pre and read and write:
            wl = (
                float(act.group(1))
                + float(pre.group(1))
                + float(read.group(1))
                + float(write.group(1))
            )
            workload_energy_pJ += wl

        if refresh and actback and preback:
            bg = (
                float(refresh.group(1))
                + float(actback.group(1))
                + float(preback.group(1))
            )
            background_energy_pJ += bg

        if total:
            total_energy_pJ += float(total.group(1))

    stats["workload_energy_pJ"] = workload_energy_pJ
    stats["background_energy_pJ"] = background_energy_pJ
    stats["total_energy_pJ"] = total_energy_pJ

    return stats


def process_all():
    """Process all benchmark results."""
    results = {}

    for op in PRIMITIVE_OPS:
        results[op] = {}
        for variant in VARIANTS:
            stats_file = RESULTS_DIR / f"{variant}_{op}" / "stats.txt"
            if not stats_file.exists():
                results[op][variant] = {"error": "missing"}
                continue

            stats = parse_stats_file(stats_file)
            sim_s = stats.get("simSeconds", 0)
            wl_energy_pJ = stats.get("workload_energy_pJ", 0)
            total_energy_pJ = stats.get("total_energy_pJ", 0)

            entry = {
                "simSeconds": sim_s,
                "simTicks": stats.get("simTicks", 0),
                "simOps": stats.get("simOps", 0),
                "simInsts": stats.get("simInsts", 0),
                "workload_energy_pJ": wl_energy_pJ,
                "background_energy_pJ": stats.get("background_energy_pJ", 0),
                "total_energy_pJ": total_energy_pJ,
                "n_ops": N_ELEMENTS,
            }

            # GOps/s
            if sim_s > 0:
                entry["GOps_s"] = (N_ELEMENTS / sim_s) / 1e9

            # GOps/J (workload energy only)
            wl_energy_J = wl_energy_pJ * 1e-12
            if wl_energy_J > 0:
                entry["GOps_J"] = (N_ELEMENTS / wl_energy_J) / 1e9

            # GOps/J (total energy)
            total_energy_J = total_energy_pJ * 1e-12
            if total_energy_J > 0:
                entry["GOps_J_total"] = (N_ELEMENTS / total_energy_J) / 1e9

            results[op][variant] = entry

    return results


def print_table(results):
    """Print a summary table to stdout."""
    print(
        f"\n{'Operation':<20} {'Variant':<6} {'Time (s)':<14} {'GOps/s':<14} {'Wkld E (pJ)':<16} {'GOps/J (wkld)':<14}"
    )
    print("=" * 84)

    for op in PRIMITIVE_OPS:
        for variant in VARIANTS:
            d = results.get(op, {}).get(variant, {})
            if "error" in d:
                print(f"{op:<20} {variant.upper():<6} {'MISSING':<14}")
                continue
            sim_s = d.get("simSeconds", 0)
            gops_s = d.get("GOps_s", 0)
            wl_e = d.get("workload_energy_pJ", 0)
            gops_j = d.get("GOps_J", 0)
            print(
                f"{op:<20} {variant.upper():<6} {sim_s:<14.9f} {gops_s:<14.6f} {wl_e:<16.2f} {gops_j:<14.6f}"
            )
        print()


def plot_results(results):
    """Generate comparison bar charts."""
    ops = PRIMITIVE_OPS
    n_ops = len(ops)
    x = np.arange(n_ops)
    bar_width = 0.25

    # Short labels for x-axis
    short_labels = [op.replace("row", "") for op in ops]

    # --- Figure 1: Execution Time ---
    fig1, ax1 = plt.subplots(figsize=(14, 5))
    for i, variant in enumerate(VARIANTS):
        vals = []
        for op in ops:
            d = results.get(op, {}).get(variant, {})
            vals.append(d.get("simSeconds", 0))
        ax1.bar(
            x + i * bar_width,
            vals,
            bar_width,
            label=VARIANT_LABELS[variant],
            color=VARIANT_COLORS[variant],
        )

    ax1.set_xlabel("Operation")
    ax1.set_ylabel("Execution Time (s)")
    ax1.set_title("gem5 Simulated Execution Time per Operation")
    ax1.set_xticks(x + bar_width)
    ax1.set_xticklabels(short_labels, rotation=45, ha="right")
    ax1.legend()
    ax1.set_yscale("log")
    ax1.grid(axis="y", alpha=0.3)
    fig1.tight_layout()
    fig1.savefig(RESULTS_DIR / "plot_exec_time.png", dpi=150)
    print(f"Saved: {RESULTS_DIR / 'plot_exec_time.png'}")

    # --- Figure 2: Throughput (GOps/s) ---
    fig2, ax2 = plt.subplots(figsize=(14, 5))
    for i, variant in enumerate(VARIANTS):
        vals = []
        for op in ops:
            d = results.get(op, {}).get(variant, {})
            vals.append(d.get("GOps_s", 0))
        ax2.bar(
            x + i * bar_width,
            vals,
            bar_width,
            label=VARIANT_LABELS[variant],
            color=VARIANT_COLORS[variant],
        )

    ax2.set_xlabel("Operation")
    ax2.set_ylabel("GOps/s")
    ax2.set_title("Throughput: Giga-Operations per Second")
    ax2.set_xticks(x + bar_width)
    ax2.set_xticklabels(short_labels, rotation=45, ha="right")
    ax2.legend()
    ax2.set_yscale("log")
    ax2.grid(axis="y", alpha=0.3)
    fig2.tight_layout()
    fig2.savefig(RESULTS_DIR / "plot_throughput.png", dpi=150)
    print(f"Saved: {RESULTS_DIR / 'plot_throughput.png'}")

    # --- Figure 3: Energy Efficiency (GOps/J, workload energy only) ---
    fig3, ax3 = plt.subplots(figsize=(14, 5))
    for i, variant in enumerate(VARIANTS):
        vals = []
        for op in ops:
            d = results.get(op, {}).get(variant, {})
            vals.append(d.get("GOps_J", 0))
        ax3.bar(
            x + i * bar_width,
            vals,
            bar_width,
            label=VARIANT_LABELS[variant],
            color=VARIANT_COLORS[variant],
        )

    ax3.set_xlabel("Operation")
    ax3.set_ylabel("GOps/J")
    ax3.set_title("DRAM Energy Efficiency: GOps/J (Workload Energy Only)")
    ax3.set_xticks(x + bar_width)
    ax3.set_xticklabels(short_labels, rotation=45, ha="right")
    ax3.legend()
    ax3.set_yscale("log")
    ax3.grid(axis="y", alpha=0.3)
    fig3.tight_layout()
    fig3.savefig(RESULTS_DIR / "plot_energy_efficiency.png", dpi=150)
    print(f"Saved: {RESULTS_DIR / 'plot_energy_efficiency.png'}")

    # --- Figure 4: Workload DRAM Energy (pJ) ---
    fig4, ax4 = plt.subplots(figsize=(14, 5))
    for i, variant in enumerate(VARIANTS):
        vals = []
        for op in ops:
            d = results.get(op, {}).get(variant, {})
            vals.append(d.get("workload_energy_pJ", 0))
        ax4.bar(
            x + i * bar_width,
            vals,
            bar_width,
            label=VARIANT_LABELS[variant],
            color=VARIANT_COLORS[variant],
        )

    ax4.set_xlabel("Operation")
    ax4.set_ylabel("DRAM Workload Energy (pJ)")
    ax4.set_title("DRAM Workload Energy per Operation (act + pre + read + write)")
    ax4.set_xticks(x + bar_width)
    ax4.set_xticklabels(short_labels, rotation=45, ha="right")
    ax4.legend()
    ax4.set_yscale("log")
    ax4.grid(axis="y", alpha=0.3)
    fig4.tight_layout()
    fig4.savefig(RESULTS_DIR / "plot_dram_energy.png", dpi=150)
    print(f"Saved: {RESULTS_DIR / 'plot_dram_energy.png'}")

    # --- Figure 5: Speedup relative to CPU ---
    fig5, ax5 = plt.subplots(figsize=(14, 5))
    for i, variant in enumerate(["gpu", "pim"]):
        vals = []
        for op in ops:
            cpu_t = results.get(op, {}).get("cpu", {}).get("simSeconds", 0)
            var_t = results.get(op, {}).get(variant, {}).get("simSeconds", 0)
            if cpu_t > 0 and var_t > 0:
                vals.append(cpu_t / var_t)
            else:
                vals.append(0)
        offset = i * bar_width
        ax5.bar(
            x + offset,
            vals,
            bar_width,
            label=f"{VARIANT_LABELS[variant]} vs CPU",
            color=VARIANT_COLORS[variant],
        )

    ax5.axhline(y=1.0, color="gray", linestyle="--", linewidth=1, label="CPU baseline")
    ax5.set_xlabel("Operation")
    ax5.set_ylabel("Speedup (x)")
    ax5.set_title("Speedup Relative to CPU")
    ax5.set_xticks(x + bar_width / 2)
    ax5.set_xticklabels(short_labels, rotation=45, ha="right")
    ax5.legend()
    ax5.set_yscale("log")
    ax5.grid(axis="y", alpha=0.3)
    fig5.tight_layout()
    fig5.savefig(RESULTS_DIR / "plot_speedup.png", dpi=150)
    print(f"Saved: {RESULTS_DIR / 'plot_speedup.png'}")

    plt.close("all")


def save_csv(results):
    """Save results to CSV."""
    csv_path = RESULTS_DIR / "benchmark_results.csv"
    with open(csv_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(
            [
                "operation",
                "variant",
                "simSeconds",
                "simTicks",
                "simOps",
                "simInsts",
                "workload_energy_pJ",
                "background_energy_pJ",
                "total_energy_pJ",
                "GOps_s",
                "GOps_J_workload",
                "GOps_J_total",
            ]
        )
        for op in PRIMITIVE_OPS:
            for variant in VARIANTS:
                d = results.get(op, {}).get(variant, {})
                if "error" in d:
                    continue
                writer.writerow(
                    [
                        op,
                        variant,
                        d.get("simSeconds", ""),
                        d.get("simTicks", ""),
                        d.get("simOps", ""),
                        d.get("simInsts", ""),
                        d.get("workload_energy_pJ", ""),
                        d.get("background_energy_pJ", ""),
                        d.get("total_energy_pJ", ""),
                        d.get("GOps_s", ""),
                        d.get("GOps_J", ""),
                        d.get("GOps_J_total", ""),
                    ]
                )
    print(f"Saved: {csv_path}")


def main():
    print("=" * 60)
    print("Benchmark Analysis: CPU vs GPU vs PIM")
    print("=" * 60)

    results = process_all()

    # Save JSON
    json_path = RESULTS_DIR / "throughput_analysis.json"
    with open(json_path, "w") as f:
        json.dump(results, f, indent=2)
    print(f"Saved: {json_path}")

    # Print table
    print_table(results)

    # Save CSV
    save_csv(results)

    # Generate plots
    print("\nGenerating plots...")
    plot_results(results)

    print("\nDone!")


if __name__ == "__main__":
    main()
