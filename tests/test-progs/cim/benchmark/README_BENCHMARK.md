# CIM Benchmark Suite

## Overview

This benchmark suite compares three computing paradigms for array operations and machine learning workloads:
- **CPU**: Traditional CPU execution with compiler auto-vectorization (-O3)
- **GPU**: GPU parallel execution using HIP
- **PIM**: Processing-In-Memory using DRAM row operations

## Updated gem5 Configuration

The simulation now uses a more realistic configuration based on MIMDRAM Paper (Table 2):

```python
# System Configuration
- CPU: 4 GHz (was 1 GHz)
- L1 I/D Cache: 32KB, 8-way, 64B line (added - was direct-connected)
- L2 Cache: 256KB, 4-way, 64B line (added - was direct-connected)
- Memory: DDR4-2400, 8GB, single channel (was DDR3-1600, 512MB, no channels)
- Memory Controller: FR-FCFS scheduling, 8KB row size
```

**Key Changes:**
1. ✓ Added realistic cache hierarchy (L1/L2)
2. ✓ Upgraded to DDR4-2400
3. ✓ Increased memory to 8GB
4. ✓ Increased CPU clock to 4GHz

## Benchmark Workloads

### 1. Primitive Row Operations (12 operations)

Operations tested on arrays of 3000 elements:

| Operation | Description |
|-----------|-------------|
| `rowand` | Bitwise AND |
| `rowadd` | Element-wise addition |
| `rowsub` | Element-wise subtraction |
| `rowmult` | Element-wise multiplication |
| `rowmin` | Element-wise minimum |
| `rowmax` | Element-wise maximum |
| `rowequal` | Element-wise equality comparison |
| `rowgreater` | Element-wise greater-than |
| `rowgreater_equal` | Element-wise greater-or-equal |
| `rowif_else` | Conditional selection |
| `rowabs` | Absolute value |
| `rowbitcount` | Population count |

### 2. K-Nearest Neighbors (KNN)

Real-world machine learning workload on Iris dataset:
- **Training set**: 120 samples, 4 features each
- **Test set**: 30 samples
- **Task**: 3-NN classification
- **Distance metric**: Euclidean distance (L2)

**Implementations:**
- `combined_knn_cpu` - Pure CPU with nested loops
- `combined_knn_gpu` - GPU kernels for distance computation
- `combined_knn` - PIM-accelerated distance computation

## Running Benchmarks

### Build All Binaries

```bash
cd tests/test-progs/cim/src
make                    # Build PIM primitives + PIM KNN
make knn_all           # Build all 3 KNN variants (CPU, GPU, PIM)
```

### Run Full Benchmark Suite

```bash
cd tests/test-progs/cim/benchmark
./run_benchmarks.sh
```

This will:
1. Run all 12 primitive operations × 3 variants = 36 simulations
2. Run KNN workload × 3 variants = 3 simulations
3. Save results to `results/` directory

**Output structure:**
```
results/
├── cpu_rowand/         # gem5 output directories
│   ├── stats.txt
│   ├── config.ini
│   └── gem5_debug.log
├── cpu_rowand.txt      # Filtered output
├── gpu_rowand/
├── pim_rowand/
├── knn_cpu/
├── knn_gpu/
└── knn_pim/
```

## Expected Performance

Based on previous benchmarks with the old config (1GHz, no caches):

| Variant | Avg Speedup | Best Use Case |
|---------|-------------|---------------|
| **PIM** | **330,000x** | Simple operations on large arrays |
| **GPU** | **312x** | Parallel workloads, complex operations |
| **CPU** | 1.0x (baseline) | General-purpose, small data |

**Note**: New config (4GHz, with caches) will show different absolute numbers but similar relative trends.

## Analysis

After running benchmarks:

```bash
cd tests/test-progs/cim/benchmark

# Extract basic statistics
python3 extract_stats.py

# Calculate throughput and energy efficiency
python3 calculate_throughput.py
```

This generates:
- **Basic stats**: Simulated ticks, instruction counts, cache statistics, speedup comparisons
- **Throughput analysis**: 
  - `throughput_analysis.json` - Detailed metrics for all workloads
  - `THROUGHPUT_ANALYSIS.md` - Summary tables with GOps/s and GOps/W

### Metrics Calculated

**Throughput (GOps/s)**:
- Primitive ops: `N_ELEMENTS (3000) / execution_time`
- KNN: `~432,000 FLOPs / execution_time`

**Energy Efficiency (GOps/W)**:
- Power model: DRAM power (from gem5) + CPU power (50W estimate @ 4GHz)
- Efficiency: `total_operations / total_power`

**Note**: gem5's power modeling is limited. CPU power is a conservative estimate. DRAM power is accurately modeled from DDR4 specifications.

## File Structure

```
benchmark/
├── run_benchmarks.sh         # Main benchmark script
├── pim_test_cpu_serial       # CPU primitive operations binary
├── pim_test_gpu              # GPU primitive operations binary
├── pim_test_cim              # PIM primitive operations binary
├── extract_stats.py          # Statistics extraction
└── results/                  # Benchmark outputs

../bin/
├── combined_knn              # PIM KNN
├── combined_knn_cpu          # CPU KNN
└── combined_knn_gpu          # GPU KNN

../src/
├── combined_knn.cpp          # PIM KNN source
├── combined_knn_cpu.cpp      # CPU KNN source
├── combined_knn_gpu.cpp      # GPU KNN source
└── Makefile
```

## Notes

- **gem5 simulation time**: Each primitive op takes ~1-5 minutes; KNN takes ~10-30 minutes
- **VEGA_X86 build**: Required for GPU benchmarks
- **Data path**: KNN expects `tests/test-progs/cim/data/iris/train.csv` and `test.csv`
- **Filtering**: Script uses `grep -Ev '(^Command|WARNING|^.*warn:)'` to reduce log noise

## Citation

Configuration based on:
- MIMDRAM Paper, Table 2 (system specs)
- gem5 learning examples (cache hierarchy)
