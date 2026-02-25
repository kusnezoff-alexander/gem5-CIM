#!/usr/bin/env python3

import os
import re
import csv

BENCHMARK_DIR = "/home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix/tests/test-progs/cim/benchmark"
OUTPUT_DIR = f"{BENCHMARK_DIR}/results"

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
]

N_ELEMS = 3000
ELEMENT_SIZE = 2
TOTAL_BYTES = N_ELEMS * ELEMENT_SIZE


def parse_gem5_stats(variant, op_name):
    """Parse gem5 stats for any variant (cpu_serial, cpu_simd, cim)"""
    stats_dir = f"{OUTPUT_DIR}/{variant}_{op_name}"
    stats_file = f"{stats_dir}/stats.txt"

    if not os.path.exists(stats_file):
        return None

    with open(stats_file, "r") as f:
        content = f.read()

    parts = content.split("---------- Begin")
    if len(parts) < 2:
        return None

    last_part = parts[-1]

    tick_match = re.search(r"sim_ticks\s+(\d+)", last_part)
    runtime_ns = 0
    if tick_match:
        runtime_ns = int(tick_match.group(1)) / 1000

    rank0_energy = 0
    rank1_energy = 0
    rank0_match = re.search(
        r"system\.mem_ctrl\.dram\.rank0\.totalEnergy\s+(\d+)", last_part
    )
    rank1_match = re.search(
        r"system\.mem_ctrl\.dram\.rank1\.totalEnergy\s+(\d+)", last_part
    )

    if rank0_match:
        rank0_energy = int(rank0_match.group(1))
    if rank1_match:
        rank1_energy = int(rank1_match.group(1))

    total_energy_pj = rank0_energy + rank1_energy

    throughput_gbs = (TOTAL_BYTES / runtime_ns) * 1e-9 if runtime_ns > 0 else 0

    return {
        "runtime_ns": runtime_ns,
        "throughput_gbs": throughput_gbs,
        "energy_pj": total_energy_pj,
    }


def parse_native_runtime(op_name, variant):
    """Parse native (non-gem5) runtime for comparison"""
    filepath = f"{OUTPUT_DIR}/{variant}_{op_name}.txt"
    if os.path.exists(filepath):
        with open(filepath, "r") as f:
            content = f.read()
        match = re.search(r"Runtime:\s+(\d+)\s+ns", content)
        if match:
            runtime_ns = int(match.group(1))
            throughput_gbs = (TOTAL_BYTES / runtime_ns) * 1e-9 if runtime_ns > 0 else 0
            return {"runtime_ns": runtime_ns, "throughput_gbs": throughput_gbs}
    return None


def main():
    results = []

    for op_name in OP_NAMES:
        row = {"operation": op_name}

        for variant in ["cpu_serial", "cpu_simd", "cim"]:
            data = parse_gem5_stats(variant, op_name)

            prefix = variant.replace("cpu_", "")

            if data:
                row[f"{prefix}_runtime_ns"] = data["runtime_ns"]
                row[f"{prefix}_throughput_gbs"] = data["throughput_gbs"]
                row[f"{prefix}_energy_pj"] = data["energy_pj"]
            else:
                row[f"{prefix}_runtime_ns"] = ""
                row[f"{prefix}_throughput_gbs"] = ""
                row[f"{prefix}_energy_pj"] = ""

        results.append(row)

    csv_file = f"{OUTPUT_DIR}/comparison.csv"
    with open(csv_file, "w", newline="") as f:
        fieldnames = [
            "operation",
            "serial_runtime_ns",
            "serial_throughput_gbs",
            "serial_energy_pj",
            "simd_runtime_ns",
            "simd_throughput_gbs",
            "simd_energy_pj",
            "cim_runtime_ns",
            "cim_throughput_gbs",
            "cim_energy_pj",
        ]
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(results)

    print(f"Comparison CSV written to: {csv_file}")
    print("\nResults Summary (from gem5):")
    print("=" * 120)
    print(
        f"{'Operation':<20} {'Serial':<25} {'SIMD':<25} {'CIM':<25} {'CIM Energy':<15}"
    )
    print(
        f"{'':<20} {'Runtime(ns)  GB/s':<25} {'Runtime(ns)  GB/s':<25} {'Runtime(ns)  GB/s':<25} {'(pJ)':<15}"
    )
    print("=" * 120)

    for row in results:
        serial_rt = str(row.get("serial_runtime_ns", ""))
        serial_tp = str(row.get("serial_throughput_gbs", ""))
        simd_rt = str(row.get("simd_runtime_ns", ""))
        simd_tp = str(row.get("simd_throughput_gbs", ""))
        cim_rt = str(row.get("cim_runtime_ns", ""))
        cim_tp = str(row.get("cim_throughput_gbs", ""))
        cim_en = str(row.get("cim_energy_pj", ""))

        serial_fmt = f"{serial_rt:<12} {serial_tp:<12}" if serial_rt else "N/A"
        simd_fmt = f"{simd_rt:<12} {simd_tp:<12}" if simd_rt else "N/A"
        cim_fmt = f"{cim_rt:<12} {cim_tp:<12}" if cim_rt else "N/A"

        print(
            f"{row['operation']:<20} {serial_fmt:<25} {simd_fmt:<25} {cim_fmt:<25} {cim_en:<15}"
        )


if __name__ == "__main__":
    main()
