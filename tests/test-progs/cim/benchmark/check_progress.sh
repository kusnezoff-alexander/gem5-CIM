#!/bin/bash

BENCHMARK_DIR="/home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix/tests/test-progs/cim/benchmark"
LOG_FILE="$BENCHMARK_DIR/benchmark_run.log"
RESULTS_DIR="$BENCHMARK_DIR/results"

echo "=============================================="
echo "Benchmark Progress Monitor"
echo "=============================================="
echo ""

# Check if benchmarks are running
if pgrep -f "run_benchmarks.sh" > /dev/null; then
    echo "✓ Benchmarks are running"
    echo ""
else
    echo "✗ Benchmarks are NOT running"
    echo ""
fi

# Show current operation
echo "Current operation:"
tail -5 "$LOG_FILE" 2>/dev/null || echo "  (log not yet created)"
echo ""

# Count completed benchmarks
TOTAL_EXPECTED=39  # 12 ops × 3 variants + 3 KNN variants
COMPLETED_DIRS=$(find "$RESULTS_DIR" -type d -name "*_row*" -o -name "knn_*" | wc -l)
COMPLETED_STATS=$(find "$RESULTS_DIR" -type f -name "stats.txt" | wc -l)

echo "Progress:"
echo "  Completed result directories: $COMPLETED_DIRS / $TOTAL_EXPECTED"
echo "  Completed stats files: $COMPLETED_STATS / $TOTAL_EXPECTED"
echo ""

# Estimate remaining time (rough)
if [ $COMPLETED_STATS -gt 0 ]; then
    PERCENT=$((COMPLETED_STATS * 100 / TOTAL_EXPECTED))
    echo "  ~${PERCENT}% complete"
fi

echo ""
echo "=============================================="
echo "To monitor live: tail -f $LOG_FILE"
echo "=============================================="
