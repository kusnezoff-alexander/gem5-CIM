#!/usr/bin/env python3
"""
Extract and compare statistics from CPU SIMD, CIM, and GPU benchmarks.
Generates detailed comparison tables and graphs.
"""

import os
import re
import json
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np
from pathlib import Path

# Configuration
RESULTS_DIR = Path("results")
OPERATIONS = [
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

VARIANTS = {
    "cpu_simd": "CPU SIMD",
    "cim": "CIM",
    "gpu_test": "GPU",
}


def extract_stat(stats_file, pattern):
    """Extract a statistic value using regex pattern."""
    try:
        with open(stats_file, "r") as f:
            content = f.read()
            match = re.search(pattern, content, re.MULTILINE)
            if match:
                value = match.group(1).strip()
                # Remove any comments
                value = value.split("#")[0].strip()
                # Handle scientific notation and regular numbers
                try:
                    return float(value)
                except:
                    return value
            return None
    except FileNotFoundError:
        return None


def extract_all_stats(stats_file):
    """Extract key statistics from a stats.txt file."""
    stats = {}

    # Simulation time
    stats["simTicks"] = extract_stat(stats_file, r"^simTicks\s+(\S+)")
    stats["simSeconds"] = extract_stat(stats_file, r"^simSeconds\s+(\S+)")

    # Instructions
    stats["simInsts"] = extract_stat(stats_file, r"^simInsts\s+(\S+)")
    stats["simOps"] = extract_stat(stats_file, r"^simOps\s+(\S+)")

    # Host performance
    stats["hostSeconds"] = extract_stat(stats_file, r"^hostSeconds\s+(\S+)")
    stats["hostInstRate"] = extract_stat(stats_file, r"^hostInstRate\s+(\S+)")

    # CPU cycles (try different patterns)
    stats["cpuCycles"] = extract_stat(stats_file, r"^system\.cpu\.numCycles\s+(\S+)")
    if not stats["cpuCycles"]:
        stats["cpuCycles"] = extract_stat(
            stats_file, r"^system\.cpu\[0\]\.numCycles\s+(\S+)"
        )

    # Memory stats
    stats["totalReads"] = extract_stat(
        stats_file, r"^system\.mem_ctrls.*\.readReqs\s+(\S+)"
    )
    stats["totalWrites"] = extract_stat(
        stats_file, r"^system\.mem_ctrls.*\.writeReqs\s+(\S+)"
    )

    # Ruby cache stats (if available)
    stats["l1Hits"] = extract_stat(stats_file, r"L1_Load_hit.*\s+(\d+)")
    stats["l1Misses"] = extract_stat(stats_file, r"L1_Load_miss.*\s+(\d+)")

    return stats


def collect_all_data():
    """Collect statistics from all variants and operations."""
    data = {variant: {} for variant in VARIANTS.keys()}

    for variant, variant_name in VARIANTS.items():
        print(f"Collecting data for {variant_name}...")
        for op in OPERATIONS:
            stats_file = RESULTS_DIR / f"{variant}_{op}" / "stats.txt"
            if stats_file.exists():
                stats = extract_all_stats(stats_file)
                data[variant][op] = stats
                print(f"  ✓ {op}")
            else:
                print(f"  ✗ {op} - not found")

    return data


def create_comparison_table(data):
    """Create a detailed comparison table."""
    print("\n" + "=" * 100)
    print("PERFORMANCE COMPARISON - SIMULATED TIME (Ticks)")
    print("=" * 100)

    # Header
    header = f"{'Operation':<20}"
    for variant_name in VARIANTS.values():
        header += f"{variant_name:>20}"
    print(header)
    print("-" * 100)

    # Data rows
    for op in OPERATIONS:
        row = f"{op:<20}"
        for variant in VARIANTS.keys():
            if op in data[variant] and data[variant][op]["simTicks"]:
                ticks = int(data[variant][op]["simTicks"])
                row += f"{ticks:>20,}"
            else:
                row += f"{'N/A':>20}"
        print(row)

    print("=" * 100)


def create_instruction_table(data):
    """Create instruction count comparison."""
    print("\n" + "=" * 100)
    print("INSTRUCTION COUNT COMPARISON")
    print("=" * 100)

    # Header
    header = f"{'Operation':<20}"
    for variant_name in VARIANTS.values():
        header += f"{variant_name:>20}"
    print(header)
    print("-" * 100)

    # Data rows
    for op in OPERATIONS:
        row = f"{op:<20}"
        for variant in VARIANTS.keys():
            if op in data[variant] and data[variant][op]["simInsts"]:
                insts = int(data[variant][op]["simInsts"])
                row += f"{insts:>20,}"
            else:
                row += f"{'N/A':>20}"
        print(row)

    print("=" * 100)


def create_speedup_table(data):
    """Create speedup comparison (relative to CPU SIMD)."""
    print("\n" + "=" * 100)
    print("SPEEDUP vs CPU SIMD (higher is better)")
    print("=" * 100)

    # Header
    header = f"{'Operation':<20}"
    for variant_name in list(VARIANTS.values())[1:]:  # Skip CPU SIMD
        header += f"{variant_name:>20}"
    print(header)
    print("-" * 100)

    # Data rows
    for op in OPERATIONS:
        row = f"{op:<20}"
        baseline = None

        # Get CPU SIMD baseline
        if op in data["cpu_simd"] and data["cpu_simd"][op]["simTicks"]:
            baseline = float(data["cpu_simd"][op]["simTicks"])

        for variant in list(VARIANTS.keys())[1:]:  # Skip CPU SIMD
            if baseline and op in data[variant] and data[variant][op]["simTicks"]:
                ticks = float(data[variant][op]["simTicks"])
                speedup = baseline / ticks
                row += f"{speedup:>20.2f}x"
            else:
                row += f"{'N/A':>20}"
        print(row)

    print("=" * 100)


def plot_performance_comparison(data, output_file="performance_comparison.png"):
    """Create bar chart comparing performance with logarithmic scale."""
    fig, ax = plt.subplots(figsize=(16, 10))

    x = np.arange(len(OPERATIONS))
    width = 0.3

    # Color scheme from TU Dresden - Blues and Greys only
    colors = {
        "cpu_simd": "#2F57B2",  # Blau1 (RGB 47, 87, 178)
        "cim": "#566371",  # Grau80 (RGB 86, 99, 113)
        "gpu_test": "#00008C",  # Brilliantblau (RGB 0, 0, 140)
    }

    for i, (variant, variant_name) in enumerate(VARIANTS.items()):
        ticks = []
        for op in OPERATIONS:
            if op in data[variant] and data[variant][op]["simTicks"]:
                ticks.append(
                    float(data[variant][op]["simTicks"]) / 1e6
                )  # Convert to millions
            else:
                ticks.append(1)  # Use 1 instead of 0 for log scale

        ax.bar(x + i * width, ticks, width, label=variant_name, color=colors[variant])

    ax.set_xlabel("Operation", fontsize=12, fontweight="bold")
    ax.set_ylabel(
        "Simulated Time (Million Ticks) - LOG SCALE", fontsize=12, fontweight="bold"
    )
    ax.set_title(
        "Performance Comparison: CPU SIMD vs CIM vs GPU\n(Logarithmic Scale)",
        fontsize=14,
        fontweight="bold",
    )
    ax.set_xticks(x + width)
    ax.set_xticklabels(OPERATIONS, rotation=45, ha="right")
    ax.legend(loc="upper left", fontsize=10)
    ax.set_yscale("log")

    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches="tight")
    print(f"\n✓ Performance comparison chart saved to: {output_file}")
    plt.close()


def plot_speedup_comparison(data, output_file="speedup_comparison.png"):
    """Create speedup comparison chart with logarithmic scale."""
    fig, ax = plt.subplots(figsize=(16, 10))

    x = np.arange(len(OPERATIONS))
    width = 0.4

    # Color scheme from TU Dresden - Blues and Greys (excluding CPU SIMD as it's the baseline)
    colors = {
        "cim": "#566371",  # Grau80 (RGB 86, 99, 113)
        "gpu_test": "#00008C",  # Brilliantblau (RGB 0, 0, 140)
    }

    for i, (variant, variant_name) in enumerate(
        list(VARIANTS.items())[1:]
    ):  # Skip CPU SIMD (first variant)
        speedups = []
        for op in OPERATIONS:
            baseline = data["cpu_simd"].get(op, {}).get("simTicks")
            current = data[variant].get(op, {}).get("simTicks")

            if baseline and current:
                speedup = float(baseline) / float(current)
                speedups.append(speedup)
            else:
                speedups.append(1)  # Use 1 instead of 0 for log scale

        ax.bar(
            x + i * width, speedups, width, label=variant_name, color=colors[variant]
        )

    # Add reference line at 1.0x (no speedup)
    ax.axhline(
        y=1.0,
        color="#2F57B2",  # Use Blau1 for baseline reference
        linestyle="--",
        linewidth=2,
        alpha=0.5,
        label="Baseline (1.0x - CPU SIMD)",
    )

    ax.set_xlabel("Operation", fontsize=12, fontweight="bold")
    ax.set_ylabel("Speedup vs CPU SIMD - LOG SCALE", fontsize=12, fontweight="bold")
    ax.set_title(
        "Speedup Comparison (Relative to CPU SIMD)\n(Logarithmic Scale)",
        fontsize=14,
        fontweight="bold",
    )
    ax.set_xticks(x + width)
    ax.set_xticklabels(OPERATIONS, rotation=45, ha="right")
    ax.legend(loc="upper left", fontsize=10)
    ax.set_yscale("log")

    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches="tight")
    print(f"✓ Speedup comparison chart saved to: {output_file}")
    plt.close()


def plot_instruction_comparison(data, output_file="instruction_comparison.png"):
    """Create instruction count comparison chart with logarithmic scale."""
    fig, ax = plt.subplots(figsize=(16, 10))

    x = np.arange(len(OPERATIONS))
    width = 0.3

    # Color scheme from TU Dresden - Blues and Greys only
    colors = {
        "cpu_simd": "#2F57B2",  # Blau1 (RGB 47, 87, 178)
        "cim": "#566371",  # Grau80 (RGB 86, 99, 113)
        "gpu_test": "#00008C",  # Brilliantblau (RGB 0, 0, 140)
    }

    for i, (variant, variant_name) in enumerate(VARIANTS.items()):
        insts = []
        for op in OPERATIONS:
            if op in data[variant] and data[variant][op]["simInsts"]:
                insts.append(
                    float(data[variant][op]["simInsts"]) / 1000
                )  # Convert to thousands
            else:
                insts.append(0.001)  # Use small value instead of 0 for log scale

        ax.bar(x + i * width, insts, width, label=variant_name, color=colors[variant])

    ax.set_xlabel("Operation", fontsize=12, fontweight="bold")
    ax.set_ylabel(
        "Instructions (Thousands) - LOG SCALE", fontsize=12, fontweight="bold"
    )
    ax.set_title(
        "Instruction Count Comparison\n(Logarithmic Scale)",
        fontsize=14,
        fontweight="bold",
    )
    ax.set_xticks(x + width)
    ax.set_xticklabels(OPERATIONS, rotation=45, ha="right")
    ax.legend(loc="upper left", fontsize=10)
    ax.set_yscale("log")

    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches="tight")
    print(f"✓ Instruction comparison chart saved to: {output_file}")
    plt.close()


def save_detailed_json(data, output_file="detailed_statistics.json"):
    """Save all extracted data to JSON for further analysis."""
    # Convert to serializable format
    json_data = {}
    for variant, ops in data.items():
        json_data[variant] = {}
        for op, stats in ops.items():
            json_data[variant][op] = {
                k: str(v) if v is not None else None for k, v in stats.items()
            }

    with open(output_file, "w") as f:
        json.dump(json_data, f, indent=2)

    print(f"\n✓ Detailed statistics saved to: {output_file}")


def create_summary_report(data, output_file="COMPARISON_SUMMARY.md"):
    """Create a comprehensive markdown summary report."""
    with open(output_file, "w") as f:
        f.write("# Performance Comparison Summary\n\n")
        f.write("## Benchmark Configuration\n\n")
        f.write("- **CPU SIMD:** SSE4.1 optimized execution\n")
        f.write("- **CIM:** Processing-in-Memory (DRAM-based)\n")
        f.write("- **GPU:** GPU parallel execution (4 compute units)\n\n")

        f.write("## Performance Summary\n\n")
        f.write("### Simulated Time (Ticks)\n\n")
        f.write("| Operation | CPU SIMD | CIM | GPU |\n")
        f.write("|-----------|----------|-----|-----|\n")

        for op in OPERATIONS:
            f.write(f"| {op:<17} |")
            for variant in VARIANTS.keys():
                if op in data[variant] and data[variant][op]["simTicks"]:
                    ticks = int(data[variant][op]["simTicks"])
                    f.write(f" {ticks:>10,} |")
                else:
                    f.write(" N/A |")
            f.write("\n")

        f.write("\n### Speedup vs CPU SIMD\n\n")
        f.write("| Operation | CIM | GPU |\n")
        f.write("|-----------|-----|-----|\n")

        for op in OPERATIONS:
            f.write(f"| {op:<17} |")
            baseline = data["cpu_simd"].get(op, {}).get("simTicks")

            for variant in list(VARIANTS.keys())[1:]:  # Skip CPU SIMD
                if baseline and op in data[variant] and data[variant][op]["simTicks"]:
                    current = float(data[variant][op]["simTicks"])
                    speedup = float(baseline) / current
                    f.write(f" {speedup:>8.2f}x |")
                else:
                    f.write(" N/A |")
            f.write("\n")

        f.write("\n## Key Observations\n\n")

        # Calculate averages
        avg_speedups = {variant: [] for variant in list(VARIANTS.keys())[1:]}
        for op in OPERATIONS:
            baseline = data["cpu_simd"].get(op, {}).get("simTicks")
            if baseline:
                for variant in list(VARIANTS.keys())[1:]:
                    if op in data[variant] and data[variant][op]["simTicks"]:
                        current = float(data[variant][op]["simTicks"])
                        speedup = float(baseline) / current
                        avg_speedups[variant].append(speedup)

        f.write("### Average Speedup\n\n")
        for variant, variant_name in list(VARIANTS.items())[1:]:
            if avg_speedups[variant]:
                avg = np.mean(avg_speedups[variant])
                f.write(f"- **{variant_name}:** {avg:.2f}x faster than CPU SIMD\n")

        f.write("\n## Generated Files\n\n")
        f.write("- `performance_comparison.png` - Performance comparison chart\n")
        f.write("- `speedup_comparison.png` - Speedup comparison chart\n")
        f.write("- `instruction_comparison.png` - Instruction count comparison\n")
        f.write("- `detailed_statistics.json` - Complete data in JSON format\n")
        f.write("- `COMPARISON_SUMMARY.md` - This file\n")

    print(f"✓ Summary report saved to: {output_file}")


def main():
    print("=" * 80)
    print("BENCHMARK STATISTICS EXTRACTION AND COMPARISON")
    print("=" * 80)
    print()

    # Collect all data
    print("Step 1: Collecting statistics from all variants...")
    data = collect_all_data()

    # Create comparison tables
    print("\nStep 2: Creating comparison tables...")
    create_comparison_table(data)
    create_instruction_table(data)
    create_speedup_table(data)

    # Generate charts
    print("\nStep 3: Generating performance charts...")
    plot_performance_comparison(data)
    plot_speedup_comparison(data)
    plot_instruction_comparison(data)

    # Save detailed data
    print("\nStep 4: Saving detailed statistics...")
    save_detailed_json(data)

    # Create summary report
    print("\nStep 5: Creating summary report...")
    create_summary_report(data)

    print("\n" + "=" * 80)
    print("ANALYSIS COMPLETE!")
    print("=" * 80)
    print("\nGenerated files:")
    print("  - performance_comparison.png")
    print("  - speedup_comparison.png")
    print("  - instruction_comparison.png")
    print("  - detailed_statistics.json")
    print("  - COMPARISON_SUMMARY.md")
    print()


if __name__ == "__main__":
    main()
