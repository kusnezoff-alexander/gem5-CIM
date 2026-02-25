# Benchmark Execution Status

## Started
**Time:** Feb 24 2026, ~15:07 UTC
**Process ID:** 1128014
**Status:** RUNNING IN BACKGROUND

## Configuration

### Updated gem5 Config (MIMDRAM Paper Table 2)
- ✓ CPU: 4 GHz (was 1 GHz)
- ✓ L1 Cache: 32KB I/D, 8-way
- ✓ L2 Cache: 256KB, 4-way
- ✓ Memory: DDR4-2400, 8GB (was DDR3-1600, 512MB)

### Benchmark Suite
**Total:** 39 benchmarks

**Primitive Operations:** 36 benchmarks
- 12 operations (rowand, rowadd, rowsub, rowmult, rowmin, rowmax, rowequal, rowgreater, rowgreater_equal, rowif_else, rowabs, rowbitcount)
- 3 variants each (CPU, GPU, PIM)

**KNN Workload:** 3 benchmarks
- Iris dataset classification
- 3 variants (CPU, GPU, PIM)

## Monitoring

### Check Progress
```bash
cd /home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix/tests/test-progs/cim/benchmark
./check_progress.sh
```

### Live Monitor
```bash
tail -f benchmark_run.log
```

### Check if Running
```bash
pgrep -f run_benchmarks.sh
ps aux | grep run_benchmarks
```

### Kill if Needed
```bash
pkill -f run_benchmarks.sh
```

## Expected Duration

**Estimate:** 6-12 hours total
- Each primitive op: ~5-15 minutes per variant
- KNN workload: ~30-60 minutes per variant

**Breakdown:**
- 36 primitive benchmarks × ~10 min avg = ~6 hours
- 3 KNN benchmarks × ~45 min avg = ~2.25 hours
- **Total: ~8-10 hours**

## Output Structure

```
results/
├── cpu_rowand/
│   ├── stats.txt          ← Main statistics
│   ├── config.ini         ← Simulation config
│   ├── config.json
│   └── gem5_debug.log     ← RowOp debug output
├── cpu_rowand.txt         ← Filtered console output
├── gpu_rowand/
├── pim_rowand/
├── ... (36 primitive benchmarks)
├── knn_cpu/
├── knn_gpu/
└── knn_pim/
```

## After Completion

### Extract Statistics
```bash
cd /home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix/tests/test-progs/cim/benchmark
python3 extract_stats.py
```

### Analyze Results
The extraction will create:
- `detailed_statistics.json` - Complete stats for all benchmarks
- Comparison tables
- Performance graphs

### Key Metrics to Compare
- **simTicks** - Execution time
- **simInsts** - Instruction count
- **L1/L2 cache stats** - Hit/miss rates (NEW with updated config)
- **Memory accesses** - DDR4 performance (NEW)

## Notes

- Benchmarks run sequentially (not parallel)
- Each uses fresh gem5 instance
- Results saved to individual directories
- Log filtering: `grep -Ev '(^Command|WARNING|^.*warn:)'`
- Existing results in `benchmark/` preserved

## Previous Results (Old Config)

For comparison, old configuration results showed:
- **PIM**: ~330,000x speedup over CPU
- **GPU**: ~312x speedup over CPU
- **CPU SIMD**: ~1.02x speedup over serial

New results with realistic config (4GHz, caches) will show:
- Absolute times will differ significantly
- Relative speedups should follow similar trends
- Cache hierarchy will benefit CPU more than PIM
- DDR4 timing more realistic for all variants
