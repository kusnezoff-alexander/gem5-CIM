#!/usr/bin/env python3
"""
Visualization script for benchmark metrics.
Creates throughput and energy efficiency charts with 3 subplots (int8, int16, int32).
"""

import csv
import matplotlib.pyplot as plt
import numpy as np

CSV_FILE = "results/extracted_metrics.csv"
OUTPUT_DIR = "results"

COLORS = {
    "pim_8k": "#1E3A8A",  # Dark blue
    "cpu_8k": "#7C3AED",  # Violet
    "pim_40k": "#2563EB",  # Blue
    "cpu_40k": "#A855F7",  # Light violet
    "pim_500k": "#60A5FA",  # Light blue
    "cpu_500k": "#C084FC",  # Lighter violet
}


def load_data():
    data = {}
    with open(CSV_FILE, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            if not row["Throughput_GOps_s"] or not row.get("Bitwidth"):
                continue
            size = int(row["Size"])
            bitwidth = int(row["Bitwidth"])
            kernel = row["Kernel"]

            key = (bitwidth, kernel)
            if key not in data:
                data[key] = {}

            throughput = float(row["Throughput_GOps_s"])
            power = float(row["Power_W"])
            energy_nj = float(row["Energy_nJ"])
            energy_efficiency = throughput / power if power > 0 else 0

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


def plot_all_three(data, output_file):
    workloads = get_workloads(data)

    fig, axes = plt.subplots(1, 3, figsize=(30, 8))

    for idx, bitwidth in enumerate([8, 16, 32]):
        ax = axes[idx]
        n_workloads = len(workloads)
        x = np.arange(n_workloads)
        width = 0.14

        pim_8k_vals = []
        cpu_8k_vals = []
        pim_40k_vals = []
        cpu_40k_vals = []
        pim_500k_vals = []
        cpu_500k_vals = []

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
            pim_500k = (
                data.get((bitwidth, f"PIM_{w}"), {})
                .get(500000, {})
                .get("throughput", 0)
            )
            cpu_500k = (
                data.get((bitwidth, f"CPU_{w}"), {})
                .get(500000, {})
                .get("throughput", 0)
            )

            pim_8k_vals.append(pim_8k)
            cpu_8k_vals.append(cpu_8k)
            pim_40k_vals.append(pim_40k)
            cpu_40k_vals.append(cpu_40k)
            pim_500k_vals.append(pim_500k)
            cpu_500k_vals.append(cpu_500k)

        ax.bar(
            x - 2.5 * width,
            pim_8k_vals,
            width,
            label="PIM 8k",
            color=COLORS["pim_8k"],
            alpha=0.8,
        )
        ax.bar(
            x - 1.5 * width,
            cpu_8k_vals,
            width,
            label="CPU 8k",
            color=COLORS["cpu_8k"],
            alpha=0.8,
        )
        ax.bar(
            x - 0.5 * width,
            pim_40k_vals,
            width,
            label="PIM 40k",
            color=COLORS["pim_40k"],
            alpha=0.8,
        )
        ax.bar(
            x + 0.5 * width,
            cpu_40k_vals,
            width,
            label="CPU 40k",
            color=COLORS["cpu_40k"],
            alpha=0.8,
        )
        ax.bar(
            x + 1.5 * width,
            pim_500k_vals,
            width,
            label="PIM 500k",
            color=COLORS["pim_500k"],
            alpha=0.8,
        )
        ax.bar(
            x + 2.5 * width,
            cpu_500k_vals,
            width,
            label="CPU 500k",
            color=COLORS["cpu_500k"],
            alpha=0.8,
        )

        ax.set_xlabel("Workload", fontsize=10, fontweight="bold")
        ax.set_ylabel("Throughput (GOps/s)", fontsize=10, fontweight="bold")
        ax.set_title(f"int{bitwidth}", fontsize=12, fontweight="bold")
        ax.set_xticks(x)
        ax.set_xticklabels(workloads, rotation=45, ha="right", fontsize=8)
        ax.legend(loc="upper right", fontsize=7, ncol=2)
        ax.set_yscale("log")
        ax.grid(axis="y", alpha=0.3)

    plt.suptitle("Throughput Comparison", fontsize=14, fontweight="bold", y=1.02)
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches="tight")
    print(f"Saved: {output_file}")
    plt.close()


def plot_all_three_energy(data, output_file):
    workloads = get_workloads(data)

    fig, axes = plt.subplots(1, 3, figsize=(30, 8))

    for idx, bitwidth in enumerate([8, 16, 32]):
        ax = axes[idx]
        n_workloads = len(workloads)
        x = np.arange(n_workloads)
        width = 0.14

        pim_8k_vals = []
        cpu_8k_vals = []
        pim_40k_vals = []
        cpu_40k_vals = []
        pim_500k_vals = []
        cpu_500k_vals = []

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
            pim_500k = (
                data.get((bitwidth, f"PIM_{w}"), {})
                .get(500000, {})
                .get("energy_efficiency", 0)
            )
            cpu_500k = (
                data.get((bitwidth, f"CPU_{w}"), {})
                .get(500000, {})
                .get("energy_efficiency", 0)
            )

            pim_8k_vals.append(pim_8k)
            cpu_8k_vals.append(cpu_8k)
            pim_40k_vals.append(pim_40k)
            cpu_40k_vals.append(cpu_40k)
            pim_500k_vals.append(pim_500k)
            cpu_500k_vals.append(cpu_500k)

        ax.bar(
            x - 2.5 * width,
            pim_8k_vals,
            width,
            label="PIM 8k",
            color=COLORS["pim_8k"],
            alpha=0.8,
        )
        ax.bar(
            x - 1.5 * width,
            cpu_8k_vals,
            width,
            label="CPU 8k",
            color=COLORS["cpu_8k"],
            alpha=0.8,
        )
        ax.bar(
            x - 0.5 * width,
            pim_40k_vals,
            width,
            label="PIM 40k",
            color=COLORS["pim_40k"],
            alpha=0.8,
        )
        ax.bar(
            x + 0.5 * width,
            cpu_40k_vals,
            width,
            label="CPU 40k",
            color=COLORS["cpu_40k"],
            alpha=0.8,
        )
        ax.bar(
            x + 1.5 * width,
            pim_500k_vals,
            width,
            label="PIM 500k",
            color=COLORS["pim_500k"],
            alpha=0.8,
        )
        ax.bar(
            x + 2.5 * width,
            cpu_500k_vals,
            width,
            label="CPU 500k",
            color=COLORS["cpu_500k"],
            alpha=0.8,
        )

        ax.set_xlabel("Workload", fontsize=10, fontweight="bold")
        ax.set_ylabel(
            "Energy Efficiency (GOps/s per Watt)", fontsize=10, fontweight="bold"
        )
        ax.set_title(f"int{bitwidth}", fontsize=12, fontweight="bold")
        ax.set_xticks(x)
        ax.set_xticklabels(workloads, rotation=45, ha="right", fontsize=8)
        ax.legend(loc="upper right", fontsize=7, ncol=2)
        ax.set_yscale("log")
        ax.grid(axis="y", alpha=0.3)

    plt.suptitle("Energy Efficiency Comparison", fontsize=14, fontweight="bold", y=1.02)
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
    plot_all_three(data, f"{OUTPUT_DIR}/throughput_chart.png")
    plot_all_three_energy(data, f"{OUTPUT_DIR}/energy_efficiency_chart.png")

    print("\nDone!")


if __name__ == "__main__":
    main()
