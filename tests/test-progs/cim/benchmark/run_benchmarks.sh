#!/bin/bash

BENCHMARK_DIR="/home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix/tests/test-progs/cim/benchmark"
GEM5_DIR="/home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix"
OUTPUT_DIR="$BENCHMARK_DIR/results"
CIM_CONFIG="$GEM5_DIR/configs/cim/cim.py"

mkdir -p "$OUTPUT_DIR"

OP_NAMES=("rowand" "rowadd" "rowsub" "rowmult" "rowmin" "rowmax" "rowequal" "rowgreater" "rowgreater_equal" "rowif_else" "rowabs" "rowbitcount") # "rowdiv"

echo "=============================================="
echo "Running benchmarks in gem5..."
echo "=============================================="
echo ""

for op_id in {1..13}; do
    op_name="${OP_NAMES[$op_id-1]}"

    echo "--- Operation: $op_name (op_id=$op_id) ---"

    for variant in "cpu_serial" "cpu_simd" "cim"; do
        echo "  Running $variant..."

        case "$variant" in
            "cpu_serial")
                BIN="$BENCHMARK_DIR/pim_test_cpu_serial"
                ;;
            "cpu_simd")
                BIN="$BENCHMARK_DIR/pim_test_cpu_simd"
                ;;
            "cim")
                BIN="$BENCHMARK_DIR/pim_test_cim"
                ;;
        esac

        OUTPUT_SUBDIR="$OUTPUT_DIR/${variant}_${op_name}"
        mkdir -p "$OUTPUT_SUBDIR"

        cd "$GEM5_DIR"
        ./build/X86/gem5.debug \
            --debug-flags=RowOp \
            --debug-start=0 \
            --debug-file="$OUTPUT_SUBDIR/gem5_debug.log" \
            --outdir="$OUTPUT_SUBDIR" \
            "$CIM_CONFIG" \
            --cmd="$BIN" \
            --options="$op_id" \
            2>&1 | grep -Ev '(^Command|WARNING|^.*warn:)' \
            > "$OUTPUT_DIR/${variant}_${op_name}.txt"

        if [ $? -eq 0 ]; then
            echo "    Done: $OUTPUT_SUBDIR"
        else
            echo "    FAILED!"
        fi
    done
    echo ""
done

echo "=============================================="
echo "All benchmarks completed!"
echo "Results in: $OUTPUT_DIR"
echo "=============================================="
echo ""
echo "Now run: python3 extract_stats.py"
