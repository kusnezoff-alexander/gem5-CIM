#!/bin/bash

BENCHMARK_DIR="/home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix/tests/test-progs/cim/benchmark"
GEM5_DIR="/home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix"
SRC_DIR="$GEM5_DIR/tests/test-progs/cim/src"
BUILD_BIN_DIR="$BENCHMARK_DIR/build/bin"
OUTPUT_DIR="$BENCHMARK_DIR/results"
CIM_CONFIG="$GEM5_DIR/configs/cim/cim.py"

N_ELEMS=${N_ELEMS:-3000}
N_RUNS=${N_RUNS:-10}

echo "=============================================="
echo "Configuration:"
echo "  N_ELEMS: $N_ELEMS"
echo "  N_RUNS:  $N_RUNS"
echo "=============================================="
echo ""

mkdir -p "$OUTPUT_DIR"

OP_NAMES=("rowand" "rowadd" "rowsub" "rowmult" "rowmin" "rowmax" "rowequal" "rowgreater" "rowgreater_equal" "rowif_else" "rowabs" "rowbitcount")

echo "=============================================="
echo "Building benchmarks..."
echo "=============================================="
echo ""

cd "$BENCHMARK_DIR"
make clean > /dev/null 2>&1

echo "Building pim_test_cim (CIM primitives)..."
make pim_test_cim

echo "Building pim_test_cpu (CPU SIMD primitives)..."
make pim_test_cpu

echo "Building pim_test_gpu ..."
make pim_test_gpu

echo "Building pim_axpy (SAXPY PIM)..."
make pim_axpy N_ELEMS=$N_ELEMS N_RUNS=$N_RUNS

echo "Building pim_axpy_cpu (SAXPY CPU)..."
make pim_axpy_cpu N_ELEMS=$N_ELEMS N_RUNS=$N_RUNS

echo "Building pim_axpy_gpu (SAXPY GPU)..."
make pim_axpy_gpu N_ELEMS=$N_ELEMS N_RUNS=$N_RUNS

echo "Building combined_knn (KNN)..."
make combined_knn N_ELEMS=$N_ELEMS N_RUNS=$N_RUNS

echo "Building combined_knn_cpu (KNN CPU)..."
make combined_knn_cpu N_ELEMS=$N_ELEMS N_RUNS=$N_RUNS

echo "Building combined_knn_gpu (KNN GPU)..."
make combined_knn_gpu N_ELEMS=$N_ELEMS N_RUNS=$N_RUNS

echo ""
echo "=============================================="
echo "Running primitive benchmarks in gem5..."
echo "=============================================="
echo ""

for op_id in {1..12}; do
    op_name="${OP_NAMES[$op_id-1]}"

    for variant in "cpu" "gpu" "pim"; do
        echo "--- $variant: $op_name (op_id=$op_id) ---"

        case "$variant" in
            "cpu")
                BIN="$BUILD_BIN_DIR/pim_test_cpu"
                ;;
            "gpu")
                BIN="$BUILD_BIN_DIR/pim_test_gpu"
                ;;
            "pim")
                BIN="$BUILD_BIN_DIR/pim_test_cim"
                ;;
        esac

        OUTPUT_SUBDIR="${OUTPUT_DIR}/${variant}_${op_name}"
        mkdir -p "$OUTPUT_SUBDIR"

        cd "$GEM5_DIR"
        ./build/X86/gem5.debug \
            --debug-flags=RowOp \
            --debug-start=0 \
            --debug-file="$OUTPUT_SUBDIR/gem5_debug.log" \
            --outdir="$OUTPUT_SUBDIR" \
            "$CIM_CONFIG" \
            --cmd "$BIN $op_id" \
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
echo "Running SAXPY benchmarks..."
echo "=============================================="
echo ""

for variant in "cpu" "gpu" "pim"; do
    echo "--- SAXPY: $variant ---"

    case "$variant" in
        "cpu")
            BIN="$BUILD_BIN_DIR/pim_axpy_cpu"
            ;;
        "gpu")
            BIN="$BUILD_BIN_DIR/pim_axpy_gpu"
            ;;
        "pim")
            BIN="$BUILD_BIN_DIR/pim_axpy"
            ;;
    esac

    OUTPUT_SUBDIR="$OUTPUT_DIR/saxpy_${variant}"
    mkdir -p "$OUTPUT_SUBDIR"

    cd "$GEM5_DIR"
    ./build/X86/gem5.debug \
        --debug-flags=RowOp \
        --debug-start=0 \
        --debug-file="$OUTPUT_SUBDIR/gem5_debug.log" \
        --outdir="$OUTPUT_SUBDIR" \
        "$CIM_CONFIG" \
        --cmd="$BIN" \
        2>&1 | grep -Ev '(^Command|WARNING|^.*warn:)' \
        > "$OUTPUT_DIR/saxpy_${variant}.txt"

    if [ $? -eq 0 ]; then
        echo "    Done: $OUTPUT_SUBDIR"
    else
        echo "    FAILED!"
    fi
done

echo ""
echo "=============================================="
echo "Running KNN benchmarks..."
echo "=============================================="
echo ""

for variant in "cpu" "gpu" "pim"; do
    echo "--- KNN: $variant ---"

    case "$variant" in
        "cpu")
            BIN="$BUILD_BIN_DIR/combined_knn_cpu"
            ;;
        "gpu")
            BIN="$BUILD_BIN_DIR/combined_knn_gpu"
            ;;
        "pim")
            BIN="$BUILD_BIN_DIR/combined_knn"
            ;;
    esac

    OUTPUT_SUBDIR="$OUTPUT_DIR/knn_${variant}"
    mkdir -p "$OUTPUT_SUBDIR"

    cd "$GEM5_DIR"
    ./build/X86/gem5.debug \
        --debug-flags=RowOp \
        --debug-start=0 \
        --debug-file="$OUTPUT_SUBDIR/gem5_debug.log" \
        --outdir="$OUTPUT_SUBDIR" \
        "$CIM_CONFIG" \
        --cmd="$BIN" \
        2>&1 | grep -Ev '(^Command|WARNING|^.*warn:)' \
        > "$OUTPUT_DIR/knn_${variant}.txt"

    if [ $? -eq 0 ]; then
        echo "    Done: $OUTPUT_SUBDIR"
    else
        echo "    FAILED!"
    fi
done

echo ""
echo "=============================================="
echo "All benchmarks completed!"
echo "Results in: $OUTPUT_DIR"
echo "=============================================="
echo ""
echo "Now run: python3 extract_stats.py"
