#!/usr/bin/env python3
"""
Script to split gem5 stats.txt into separate files for each rowop.

Usage: python3 split_stats.py <stats.txt> [output_dir]

The script looks for work_begin work_id markers in the stats file and
splits the stats into separate files for each region.

Each rowop is assigned an ID:
  1: ROWAND
  2: ROWADD
  3: ROWSUB
  4: ROWMUL
  5: ROWDIV
  6: ROWMIN
  7: ROWMAX
  8: ROWEQUAL
  9: ROWGREATER
 10: ROWGREATEREQUAL
 11: ROWIFELSE
 12: ROWABS
"""

import sys
import os
import re
from pathlib import Path

ROWOP_NAMES = {
    1: "ROWAND",
    2: "ROWADD",
    3: "ROWSUB",
    4: "ROWMUL",
    5: "ROWDIV",
    6: "ROWMIN",
    7: "ROWMAX",
    8: "ROWEQUAL",
    9: "ROWGREATER",
    10: "ROWGREATEREQUAL",
    11: "ROWIFELSE",
    12: "ROWABS",
}


def parse_stats_file(stats_path):
    """Parse the stats file and return a list of (start_line, end_line, work_id) tuples."""

    with open(stats_path, "r") as f:
        lines = f.readlines()

    regions = []
    in_region = False
    region_start = 0
    current_work_id = None

    for i, line in enumerate(lines):
        # Look for work_begin markers
        match = re.search(r"work_begin\s+(\d+)", line)
        if match:
            current_work_id = int(match.group(1))
            region_start = i
            in_region = True
            continue

        # Look for work_end markers
        match = re.search(r"work_end\s+(\d+)", line)
        if match and in_region:
            work_id = int(match.group(1))
            regions.append((region_start, i, work_id))
            in_region = False
            current_work_id = None
            continue

    return lines, regions


def split_stats(stats_path, output_dir="."):
    """Split the stats file into separate files for each rowop."""

    lines, regions = parse_stats_file(stats_path)

    if not regions:
        print("No work regions found in stats file!")
        print("Make sure you're using m5_work_begin and m5_work_end in your program.")
        return

    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    # Extract the header (lines before first region that don't start with a stat)
    header_end = regions[0][0]
    header = lines[:header_end]

    # Find the footer (everything after the last region)
    last_region_end = regions[-1][1]
    footer = lines[last_region_end:]

    # Write each region's stats
    for start, end, work_id in regions:
        region_lines = lines[start : end + 1]

        if work_id in ROWOP_NAMES:
            filename = f"{ROWOP_NAMES[work_id]}.txt"
        else:
            filename = f"workid_{work_id}.txt"

        output_path = output_dir / filename

        with open(output_path, "w") as f:
            f.writelines(header)
            f.writelines(region_lines)
            f.writelines(footer)

        print(f"Written: {output_path}")

    # Also write a combined summary
    summary_path = output_dir / "summary.txt"
    with open(summary_path, "w") as f:
        f.write("Stats Split Summary\n")
        f.write("=" * 50 + "\n\n")
        for start, end, work_id in regions:
            name = ROWOP_NAMES.get(work_id, f"workid_{work_id}")
            f.write(f"  {name}: lines {start}-{end}\n")

    print(f"\nSummary written to: {summary_path}")


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    stats_path = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "split_stats"

    if not os.path.exists(stats_path):
        print(f"Error: {stats_path} not found!")
        sys.exit(1)

    split_stats(stats_path, output_dir)


if __name__ == "__main__":
    main()
