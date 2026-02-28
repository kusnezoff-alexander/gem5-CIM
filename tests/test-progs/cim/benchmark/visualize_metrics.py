#!/usr/bin/env python3
"""
Visualization script for benchmark metrics.
Creates throughput and energy efficiency charts from extracted_metrics.csv.
3 subplots in one figure: int8, int16, int32
"""

import csv
import matplotlib.pyplot as plt
import numpy as np

CSV_FILE = "results/extracted_metrics.csv"
OUTPUT_DIR = "results"

COLORS = {
    "pim_8k": "#1E3A8A",  # Dark blue
    "pim_40k": "#3B82F6",  # Light blue
    "cpu_8k": "#581C87",  # Dark violet
    "cpu_40k": "#A855F7",  # Light violet
}


def load_data():
    data = {}
    with open(CSV_FILE, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            size = int(row["Size"])
            bitwidth = int(row["Bitwidth"])
            kernel = row["Kernel"]

            key = (bitwidth, kernel)
            if key not in data:
                data[key] = {}

            if row["Throughput_GOps_s"]:
                throughput = float(row["Throughput_GOps_s"])
                power = float(row["Power_W"])
                energy_nj = float(row["Energy_nJ"])
                energy_efficiency = throughput / power if power > 0 else 0
            else:
                throughput = 0
                power = 0
                energy_nj = 0
                energy_efficiency = 0

            data[key][size] = {
                "throughput": throughput,
                "power": power,
                "energy_nj": energy_nj,
                "energy_efficiency": energy_efficiency,
            }
    return data


def get_workloads(data):
    workloads = []
    for bitwidth, kernel in data.keys():
        if "_" in kernel:
            parts = kernel.split("_", 1)
            if len(parts) == 2:
                variant, name = parts
                if name not in workloads and name not in ["knn"]:
                    workloads.append(name)
    return sorted(workloads)


def has_knn(data, bitwidth):
    return (bitwidth, "CPU_knn") in data and (bitwidth, "PIM_knn") in data


def plot_metrics(data, workloads, output_file):
    fig, axes = plt.subplots(1, 3, figsize=(24, 8))
    bitwidths = [8, 16, 32]
    titles = ["int8", "int16", "int32"]

    for idx, (bitwidth, title) in enumerate(zip(bitwidths, titles)):
        ax = axes[idx]

        n_workloads = len(workloads)
        has_knn_data = has_knn(data, bitwidth)
        n_total = n_workloads + (1 if has_knn_data else 0)

        x = np.arange(n_total)
        width = 0.18

        pim_8k_vals = []
        cpu_8k_vals = []
        pim_40k_vals = []
        cpu_40k_vals = []

        for w in workloads:
            pim_8k = (
                data.get((bitwidth, f"PIM_{w}"), {}).get(8000, {}).get("throughput", 0)
            )
            cpu_8k = (
                data.get((bitwidth, f"CPU_{w}"), {}).get(8000, {}).get("throughput", 0)
            )
            pim_40k = (
                data.get((bitwidth, f"PIM_{w}"), {}).get(40000, {}).get("throughput", 0)
            )
            cpu_40k = (
                data.get((bitwidth, f"CPU_{w}"), {}).get(40000, {}).get("throughput", 0)
            )

            pim_8k_vals.append(pim_8k)
            cpu_8k_vals.append(cpu_8k)
            pim_40k_vals.append(pim_40k)
            cpu_40k_vals.append(cpu_40k)

        if has_knn_data:
            cpu_knn = (
                data.get((bitwidth, "CPU_knn"), {}).get(150, {}).get("throughput", 0)
            )
            pim_knn = (
                data.get((bitwidth, "PIM_knn"), {}).get(150, {}).get("throughput", 0)
            )

            cpu_8k_vals.append(cpu_knn)
            pim_8k_vals.append(pim_knn)
            pim_40k_vals.append(0)
            cpu_40k_vals.append(0)

        ax.bar(
            x - 1.5 * width,
            pim_8k_vals,
            width,
            label="PIM 8k",
            color=COLORS["pim_8k"],
            alpha=0.8,
        )
        ax.bar(
            x - 0.5 * width,
            cpu_8k_vals,
            width,
            label="CPU 8k",
            color=COLORS["cpu_8k"],
            alpha=0.8,
        )
        ax.bar(
            x + 0.5 * width,
            pim_40k_vals,
            width,
            label="PIM 40k",
            color=COLORS["pim_40k"],
            alpha=0.8,
        )
        ax.bar(
            x + 1.5 * width,
            cpu_40k_vals,
            width,
            label="CPU 40k",
            color=COLORS["cpu_40k"],
            alpha=0.8,
        )

        all_labels = workloads + (["knn"] if has_knn_data else [])

        ax.set_xlabel("Workload", fontsize=10, fontweight="bold")
        ax.set_ylabel("Throughput (GOps/s)", fontsize=10, fontweight="bold")
        ax.set_title(title.upper(), fontsize=14, fontweight="bold")
        ax.set_xticks(x)
        ax.set_xticklabels(all_labels, rotation=45, ha="right", fontsize=8)
        ax.legend(loc="upper right", fontsize=8)
        ax.set_yscale("log")
        ax.grid(axis="y", alpha=0.3)

    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches="tight")
    print(f"Saved: {output_file}")
    plt.close()


def plot_energy_efficiency(data, workloads, output_file):
    fig, axes = plt.subplots(1, 3, figsize=(24, 8))
    bitwidths = [8, 16, 32]
    titles = ["int8", "int16", "int32"]

    for idx, (bitwidth, title) in enumerate(zip(bitwidths, titles)):
        ax = axes[idx]

        n_workloads = len(workloads)
        has_knn_data = has_knn(data, bitwidth)
        n_total = n_workloads + (1 if has_knn_data else 0)

        x = np.arange(n_total)
        width = 0.18

        pim_8k_vals = []
        cpu_8k_vals = []
        pim_40k_vals = []
        cpu_40k_vals = []

        for w in workloads:
            pim_8k = (
                data.get((bitwidth, f"PIM_{w}"), {})
                .get(8000, {})
                .get("energy_efficiency", 0)
            )
            cpu_8k = (
                data.get((bitwidth, f"CPU_{w}"), {})
                .get(8000, {})
                .get("energy_efficiency", 0)
            )
            pim_40k = (
                data.get((bitwidth, f"PIM_{w}"), {})
                .get(40000, {})
                .get("energy_efficiency", 0)
            )
            cpu_40k = (
                data.get((bitwidth, f"CPU_{w}"), {})
                .get(40000, {})
                .get("energy_efficiency", 0)
            )

            pim_8k_vals.append(pim_8k)
            cpu_8k_vals.append(cpu_8k)
            pim_40k_vals.append(pim_40k)
            cpu_40k_vals.append(cpu_40k)

        if has_knn_data:
            cpu_knn = (
                data.get((bitwidth, "CPU_knn"), {})
                .get(150, {})
                .get("energy_efficiency", 0)
            )
            pim_knn = (
                data.get((bitwidth, "PIM_knn"), {})
                .get(150, {})
                .get("energy_efficiency", 0)
            )

            cpu_8k_vals.append(cpu_knn)
            pim_8k_vals.append(pim_knn)
            pim_40k_vals.append(0)
            cpu_40k_vals.append(0)

        ax.bar(
            x - 1.5 * width,
            pim_8k_vals,
            width,
            label="PIM 8k",
            color=COLORS["pim_8k"],
            alpha=0.8,
        )
        ax.bar(
            x - 0.5 * width,
            cpu_8k_vals,
            width,
            label="CPU 8k",
            color=COLORS["cpu_8k"],
            alpha=0.8,
        )
        ax.bar(
            x + 0.5 * width,
            pim_40k_vals,
            width,
            label="PIM 40k",
            color=COLORS["pim_40k"],
            alpha=0.8,
        )
        ax.bar(
            x + 1.5 * width,
            cpu_40k_vals,
            width,
            label="CPU 40k",
            color=COLORS["cpu_40k"],
            alpha=0.8,
        )

        all_labels = workloads + (["knn"] if has_knn_data else [])

        ax.set_xlabel("Workload", fontsize=10, fontweight="bold")
        ax.set_ylabel(
            "Energy Efficiency (GOps/s per Watt)", fontsize=10, fontweight="bold"
        )
        ax.set_title(title.upper(), fontsize=14, fontweight="bold")
        ax.set_xticks(x)
        ax.set_xticklabels(all_labels, rotation=45, ha="right", fontsize=8)
        ax.legend(loc="upper right", fontsize=8)
        ax.set_yscale("log")
        ax.grid(axis="y", alpha=0.3)

    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches="tight")
    print(f"Saved: {output_file}")
    plt.close()


def main():
    print("Loading data from CSV...")
    data = load_data()

    workloads = get_workloads(data)
    print(f"Found workloads: {workloads}")

    print("\nGenerating charts...")

    plot_metrics(data, workloads, f"{OUTPUT_DIR}/throughput_chart.png")
    plot_energy_efficiency(data, workloads, f"{OUTPUT_DIR}/energy_efficiency_chart.png")

    print("\nDone!")


if __name__ == "__main__":
    main()
