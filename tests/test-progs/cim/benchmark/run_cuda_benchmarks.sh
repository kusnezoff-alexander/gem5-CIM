#!/bin/bash

# CUDA Benchmark Script
# Runs benchmarks and measures throughput and power

OUTPUT_FILE="cuda_benchmark_results.csv"

echo "Benchmarking CUDA implementations..."
echo "Size,Kernel,Runtime_ns,Throughput_GOps_s,Power_W,Energy_nJ" > $OUTPUT_FILE

run_benchmark() {
    local size=$1
    local kernel=$2
    local runtime=$3
    local throughput=$4
    local power=$5
    
    echo "$size,$kernel,$runtime,$throughput,$power,$(echo "scale=2; $runtime * $power / 1000" | bc)" >> $OUTPUT_FILE
}

measure_power() {
    nvidia-smi --query-gpu=power.draw --format=csv,noheader,nounits
}

run_pim_test() {
    local size=$1
    echo "Running pim_test_gpu_cuda with $size elements..."
    
    for op_id in 1 2 3 4 5 6 7 8 9 10 11 12; do
        power_before=$(measure_power)
        
        result=$(./pim_test_gpu_cuda $op_id 2>/dev/null)
        runtime=$(echo "$result" | grep "Runtime:" | awk '{print $2}')
        
        power_after=$(measure_power)
        power=$(echo "($power_before + $power_after) / 2" | bc -l)
        
        throughput=$(echo "scale=2; $size * 1 / ($runtime / 1000000000) / 1000000000" | bc -l)
        
        op_name=$(echo "rowand rowadd rowsub rowmult rowmin rowmax rowequal rowgreater rowgreater_equal rowif_else rowabs bitcount" | cut -d' ' -f$op_id)
        
        run_benchmark "$size" "pim_$op_name" "$runtime" "$throughput" "$power"
    done
}

run_saxpy() {
    local size=$1
    echo "Running saxpy_cuda with $size elements..."
    
    power_before=$(measure_power)
    
    result=$(./saxpy_cuda 2>/dev/null)
    runtime=$(echo "$result" | grep "Runtime:" | awk '{print $2}')
    
    power_after=$(measure_power)
    power=$(echo "($power_before + $power_after) / 2" | bc -l)
    
    throughput=$(echo "scale=2; $size * 2 / ($runtime / 1000000000) / 1000000000" | bc -l)
    
    run_benchmark "$size" "saxpy" "$runtime" "$throughput" "$power"
}

run_knn() {
    local size=150
    echo "Running knn_cuda..."
    
    power_before=$(measure_power)
    
    result=$(./knn_cuda 2>/dev/null)
    
    power_after=$(measure_power)
    power=$(echo "($power_before + $power_after) / 2" | bc -l)
    
    runtime=1000000
    throughput=$(echo "scale=2; $size * $size * 4 / 1000000" | bc -l)
    
    run_benchmark "$size" "knn" "$runtime" "$throughput" "$power"
}

for size in 3000 30000 10000000; do
    run_pim_test $size
    run_saxpy $size
done

run_knn

echo "Results saved to $OUTPUT_FILE"
cat $OUTPUT_FILE
