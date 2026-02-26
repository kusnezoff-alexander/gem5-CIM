#!/usr/bin/env python3

import os
import re
import csv

BENCHMARK_DIR = "/home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix/tests/test-progs/cim/benchmark"
OUTPUT_FILE = f"{BENCHMARK_DIR}/results/extracted_metrics.csv"

RESULTS_DIRS = {
    "results_8k": 8000,
    "results_40k": 40000,
}

OP_NAMES = [
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
    "saxpy",
]


def parse_gem5_stats(stats_dir, total_ops):
    stats_file = f"{stats_dir}/stats.txt"
    if not os.path.exists(stats_file):
        return None

    with open(stats_file, "r") as f:
        content = f.read()

    parts = content.split("---------- Begin")
    if len(parts) < 2:
        return None

    last_part = parts[-1]

    tick_match = re.search(r"simTicks\s+(\d+)", last_part)
    runtime_ns = 0
    if tick_match:
        runtime_ns = int(tick_match.group(1)) / 1000

    rank0_energy = 0
    rank1_energy = 0
    rank0_match = re.search(
        r"system\.mem_ctrl\.dram\.rank0\.totalEnergy\s+([\d.]+)", last_part
    )
    rank1_match = re.search(
        r"system\.mem_ctrl\.dram\.rank1\.totalEnergy\s+([\d.]+)", last_part
    )

    if rank0_match:
        rank0_energy = float(rank0_match.group(1))
    if rank1_match:
        rank1_energy = float(rank1_match.group(1))

    total_energy_pj = rank0_energy + rank1_energy
    energy_nj = total_energy_pj / 1000.0

    throughput_gops = total_ops / runtime_ns if runtime_ns > 0 else 0

    power_w = (energy_nj / runtime_ns) * 1e-9 if runtime_ns > 0 else 0

    return {
        "runtime_ns": runtime_ns,
        "throughput_gops_s": throughput_gops,
        "power_w": power_w,
        "energy_nj": energy_nj,
    }


def main():
    results = []

    for dir_name, n_elems in RESULTS_DIRS.items():
        results_dir = f"{BENCHMARK_DIR}/{dir_name}"

        for op_name in OP_NAMES:
            for variant in ["cpu", "pim"]:
                if op_name == "saxpy":
                    dir_path = f"{results_dir}/{op_name}_{variant}"
                else:
                    dir_path = f"{results_dir}/{variant}_{op_name}"

                data = parse_gem5_stats(dir_path, n_elems)

                if data:
                    results.append(
                        {
                            "Size": n_elems,
                            "Kernel": f"{variant.upper()}_{op_name}",
                            "Runtime_ns": data["runtime_ns"],
                            "Throughput_GOps_s": data["throughput_gops_s"],
                            "Power_W": data["power_w"],
                            "Energy_nJ": data["energy_nj"],
                        }
                    )
                else:
                    results.append(
                        {
                            "Size": n_elems,
                            "Kernel": f"{variant.upper()}_{op_name}",
                            "Runtime_ns": "",
                            "Throughput_GOps_s": "",
                            "Power_W": "",
                            "Energy_nJ": "",
                        }
                    )

    for dir_name, n_elems in [("results_8k", 150)]:
        results_dir = f"{BENCHMARK_DIR}/{dir_name}"
        for variant in ["cpu", "pim"]:
            dir_path = f"{results_dir}/knn_{variant}"
            data = parse_gem5_stats(dir_path, n_elems)
            if data:
                results.append(
                    {
                        "Size": n_elems,
                        "Kernel": f"{variant.upper()}_knn",
                        "Runtime_ns": data["runtime_ns"],
                        "Throughput_GOps_s": data["throughput_gops_s"],
                        "Power_W": data["power_w"],
                        "Energy_nJ": data["energy_nj"],
                    }
                )
            else:
                results.append(
                    {
                        "Size": n_elems,
                        "Kernel": f"{variant.upper()}_knn",
                        "Runtime_ns": "",
                        "Throughput_GOps_s": "",
                        "Power_W": "",
                        "Energy_nJ": "",
                    }
                )

    os.makedirs(os.path.dirname(OUTPUT_FILE), exist_ok=True)
    with open(OUTPUT_FILE, "w", newline="") as f:
        fieldnames = [
            "Size",
            "Kernel",
            "Runtime_ns",
            "Throughput_GOps_s",
            "Power_W",
            "Energy_nJ",
        ]
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(results)

    print(f"Metrics written to: {OUTPUT_FILE}")
    print(f"Total rows: {len(results)}")
    print("\nSample rows:")
    print("-" * 80)
    for row in results[:6]:
        print(row)


if __name__ == "__main__":
    main()
