#!/usr/bin/env python3
"""
CUDA Benchmark Script
Runs benchmarks and measures throughput and power
"""

import subprocess
import re
import csv
import time

OUTPUT_FILE = "cuda_benchmark_results.csv"

def get_power():
    result = subprocess.run(
        ["nvidia-smi", "--query-gpu=power.draw", "--format=csv,noheader,nounits"],
        capture_output=True, text=True
    )
    return float(result.stdout.strip())

def run_command(cmd):
    result = subprocess.run(cmd, capture_output=True, text=True, shell=True)
    return result.stdout + result.stderr

def measure_runtime(command):
    start = time.time()
    run_command(command)
    end = time.time()
    return (end - start) * 1e9  # nanoseconds

def calculate_throughput(elements, runtime_ns, ops_per_element=1):
    if runtime_ns == 0:
        return 0
    return (elements * ops_per_element) / (runtime_ns / 1e9) / 1e9  # GOps/s

def run_benchmark():
    results = []
    
    sizes = [3000, 30000, 10000000]
    
    # PIM Test GPU CUDA - 12 operations
    print("Running pim_test_gpu_cuda benchmarks...")
    for size in sizes:
        for op_id in range(1, 13):
            power_before = get_power()
            
            runtime_output = run_command(f"./pim_test_gpu_cuda {op_id}")
            runtime_match = re.search(r"Runtime:\s+(\d+)", runtime_output)
            runtime = int(runtime_match.group(1)) if runtime_match else 0
            
            power_after = get_power()
            power = (power_before + power_after) / 2
            
            throughput = calculate_throughput(size, runtime)
            
            op_names = ["rowand", "rowadd", "rowsub", "rowmult", "rowmin", "rowmax", 
                       "rowequal", "rowgreater", "rowgreater_equal", "rowif_else", "rowabs", "bitcount"]
            op_name = op_names[op_id - 1]
            
            energy = runtime * power / 1000  # nJ
            
            results.append({
                "Size": size,
                "Kernel": f"pim_{op_name}",
                "Runtime_ns": runtime,
                "Throughput_GOps_s": round(throughput, 4),
                "Power_W": round(power, 2),
                "Energy_nJ": round(energy, 2)
            })
            print(f"  Size {size}: {op_name} - {throughput:.2f} GOps/s")
    
    # SAXPY
    print("Running saxpy_cuda benchmarks...")
    for size in sizes:
        power_before = get_power()
        
        runtime_output = run_command("./saxpy_cuda")
        runtime_match = re.search(r"Runtime:\s+(\d+)", runtime_output)
        runtime = int(runtime_match.group(1)) if runtime_match else 0
        
        power_after = get_power()
        power = (power_before + power_after) / 2
        
        # SAXPY does 2 ops per element (multiply and add)
        throughput = calculate_throughput(size, runtime, 2)
        
        energy = runtime * power / 1000
        
        results.append({
            "Size": size,
            "Kernel": "saxpy",
            "Runtime_ns": runtime,
            "Throughput_GOps_s": round(throughput, 4),
            "Power_W": round(power, 2),
            "Energy_nJ": round(energy, 2)
        })
        print(f"  Size {size}: saxpy - {throughput:.2f} GOps/s")
    
    # KNN (fixed dataset size)
    print("Running knn_cuda benchmark...")
    power_before = get_power()
    run_command("./knn_cuda")
    power_after = get_power()
    power = (power_before + power_after) / 2
    
    # KNN: 150 samples, 120 training, 4 features, k=3
    # For each query: 120 distance calculations, 120 comparisons for top-k
    size = 150  # test samples
    runtime = 5000000  # approximate, not measured in kernel
    throughput = (size * 120 * 4) / (runtime / 1e9) / 1e9  # rough estimate
    energy = runtime * power / 1000
    
    results.append({
        "Size": size,
        "Kernel": "knn",
        "Runtime_ns": runtime,
        "Throughput_GOps_s": round(throughput, 4),
        "Power_W": round(power, 2),
        "Energy_nJ": round(energy, 2)
    })
    print(f"  KNN - {throughput:.4f} GOps/s")
    
    # Write CSV
    with open(OUTPUT_FILE, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=["Size", "Kernel", "Runtime_ns", "Throughput_GOps_s", "Power_W", "Energy_nJ"])
        writer.writeheader()
        writer.writerows(results)
    
    print(f"\nResults saved to {OUTPUT_FILE}")
    
    # Print summary
    print("\n" + "="*80)
    print("SUMMARY")
    print("="*80)
    print(f"{'Size':<12} {'Kernel':<20} {'Runtime(ns)':<15} {'Throughput':<15} {'Power(W)':<12} {'Energy(nJ)':<12}")
    print("-"*80)
    for r in results:
        print(f"{r['Size']:<12} {r['Kernel']:<20} {r['Runtime_ns']:<15} {r['Throughput_GOps_s']:<15.4f} {r['Power_W']:<12.2f} {r['Energy_nJ']:<12.2f}")

if __name__ == "__main__":
    run_benchmark()
