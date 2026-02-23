#!/usr/bin/env python3
"""
Extract per-operation statistics from gem5 stats.txt.

The key insight: each stat dump is cumulative. We need to compute the delta
between consecutive dumps to get per-operation stats.
"""

import re
import csv
import sys

OPERATIONS = [
    "ROWAND",
    "ROWADD",
    "ROWSUB",
    "ROWMUL",
    "ROWDIV",
    "ROWMIN",
    "ROWMAX",
    "ROWEQUAL",
    "ROWGREATER",
    "ROWGREATEREQUAL",
    "ROWIFELSE",
    "ROWABS",
]

N_ELEMS = 3000


def parse_stats_file(stats_path):
    with open(stats_path, "r") as f:
        content = f.read()

    regions = content.split("---------- Begin Simulation Statistics ----------")

    results = []
    for idx, region in enumerate(regions[1:], 1):
        lines = region.strip().split("\n")

        stats = {}
        for line in lines:
            match = re.match(r"^([\w.]+)\s+([\d.e+-]+)\s+#", line)
            if match:
                name = match.group(1)
                value = match.group(2)
                stats[name] = float(value)

        if stats:
            results.append(stats)

    return results


def extract_metrics(stats_list):
    rows = []

    # First dump is initial state (before m5_reset_stats)
    # Second dump is after m5_reset_stats but before first operation
    # So we need to use the state BEFORE m5_reset_stats as baseline for first operation

    for i in range(1, len(stats_list)):  # Skip first dump (initial state)
        curr = stats_list[i]
        prev = stats_list[i - 1]

        op_idx = (i - 1) % 12  # -1 because we skipped the first
        op_name = OPERATIONS[op_idx]

        test_type = "deterministic" if (i - 1) < 12 else "fuzzy"
        test_num = ((i - 1) // 12) + 1 if (i - 1) >= 12 else 1

        # Delta from previous dump
        delta_ticks = curr.get("simTicks", 0) - prev.get("simTicks", 0)
        runtime_us = delta_ticks / 1e6

        # Runtime in seconds
        delta_sec = curr.get("simSeconds", 0) - prev.get("simSeconds", 0)

        # Throughput
        if runtime_us > 0:
            throughput = (N_ELEMS * 1e6) / runtime_us
        else:
            throughput = 0

        cycles = curr.get("system.cpu.numCycles", 0) - prev.get(
            "system.cpu.numCycles", 0
        )
        num_insts = curr.get("system.cpu.commitStats0.numInsts", 0) - prev.get(
            "system.cpu.commitStats0.numInsts", 0
        )

        if num_insts > 0:
            cpi = cycles / num_insts
        else:
            cpi = 0

        # DRAM energy delta
        dram_energy = (
            curr.get("system.mem_ctrl.dram.rank0.totalEnergy", 0)
            - prev.get("system.mem_ctrl.dram.rank0.totalEnergy", 0)
        ) / 1000.0
        dram_energy += (
            curr.get("system.mem_ctrl.dram.rank1.totalEnergy", 0)
            - prev.get("system.mem_ctrl.dram.rank1.totalEnergy", 0)
        ) / 1000.0

        row = {
            "operation": op_name,
            "test_type": test_type,
            "test_num": test_num,
            "region_idx": i - 1,
            "runtime_sec": delta_sec,
            "runtime_us": runtime_us,
            "throughput_ops_per_sec": throughput,
            "num_cycles": int(cycles),
            "num_insts": int(num_insts),
            "cpi": cpi,
            "dram_energy_nj": dram_energy,
        }

        rows.append(row)

    return rows


def write_csv(rows, output_path):
    fieldnames = [
        "operation",
        "test_type",
        "test_num",
        "region_idx",
        "runtime_sec",
        "runtime_us",
        "throughput_ops_per_sec",
        "num_cycles",
        "num_insts",
        "cpi",
        "dram_energy_nj",
    ]

    with open(output_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    print(f"Written CSV to: {output_path}")


def main():
    stats_path = "/home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix/m5out/stats.txt"
    output_path = "/home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix/m5out/per_op_stats.csv"

    if len(sys.argv) > 1:
        stats_path = sys.argv[1]
    if len(sys.argv) > 2:
        output_path = sys.argv[2]

    print(f"Parsing: {stats_path}")
    stats_list = parse_stats_file(stats_path)
    print(f"Found {len(stats_list)} stat regions")

    rows = extract_metrics(stats_list)
    write_csv(rows, output_path)

    print("\n" + "=" * 100)
    print("SUMMARY: Per-operation metrics (averaged across all runs)")
    print("=" * 100)
    print(
        f"{'Operation':<20} {'Runtime(us)':>12} {'Throughput':>15} {'Cycles':>12} {'Energy(nJ)':>15}"
    )
    print("-" * 100)

    op_metrics = {}
    for row in rows:
        op = row["operation"]
        if op not in op_metrics:
            op_metrics[op] = {
                "runtime": [],
                "throughput": [],
                "cycles": [],
                "energy": [],
            }

        op_metrics[op]["runtime"].append(row["runtime_us"])
        op_metrics[op]["throughput"].append(row["throughput_ops_per_sec"])
        op_metrics[op]["cycles"].append(row["num_cycles"])
        op_metrics[op]["energy"].append(row["dram_energy_nj"])

    for op in OPERATIONS:
        if op in op_metrics:
            m = op_metrics[op]
            avg_runtime = sum(m["runtime"]) / len(m["runtime"])
            avg_throughput = sum(m["throughput"]) / len(m["throughput"])
            avg_cycles = sum(m["cycles"]) / len(m["cycles"])
            avg_energy = sum(m["energy"]) / len(m["energy"])

            print(
                f"{op:<20} {avg_runtime:>12.2f} {avg_throughput:>15.2f} {avg_cycles:>12.0f} {avg_energy:>15.2f}"
            )


if __name__ == "__main__":
    main()
