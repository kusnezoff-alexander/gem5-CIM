# GPU Benchmarks for gem5 - Complete and Working

## Status: ✓ FULLY FUNCTIONAL

GPU programs can now run successfully in gem5 VEGA_X86!

## Quick Start

### Prerequisites
- gem5 VEGA_X86 build: `./build/VEGA_X86/gem5.debug`
- GPU binary: `pim_test_gpu` (already compiled)

### Run Quick Test
```bash
./test_gpu_quick.sh
```

### Run Single Operation
```bash
../../../build/VEGA_X86/gem5.debug \
    ../../../configs/example/apu_se.py \
    --num-compute-units=4 --num-cp=1 -n 1 \
    --cpu-type=X86TimingSimpleCPU \
    -c ./pim_test_gpu --options="1 --check"
```

### Run All Benchmarks
```bash
./run_gpu_benchmarks.sh
```

## What Was Fixed

Three critical issues were resolved:

### 1. ISA Initialization (configs/example/apu_se.py)
Command Processors weren't having their ISAs initialized. Fixed by calling `cp.createThreads()`.

### 2. Ruby Network (configs/ruby/GPU_VIPER.py)
CP SQC controllers were missing message buffer setup. Fixed by adding mandatoryQueue and network connections.

### 3. VMA Huge Pages (src/arch/x86/process.cc)
Unconditional huge page mapping with size=0 caused assertion. Fixed by adding size check.

## Files

- `run_gpu_benchmarks.sh` - Run all GPU operations
- `test_gpu_quick.sh` - Quick verification test
- `pim_test_gpu` - GPU binary (HIP-compiled)
- `GPU_FIXES_COMPLETE.md` - Detailed technical documentation
- `VERIFICATION_TEST.md` - Test results and validation
- `README_GPU.md` - This file

## Output

Each run generates:
- `results/gpu_test_<operation>/stats.txt` - Performance statistics
- `results/gpu_test_<operation>/config.ini` - Configuration
- Log file with simulation output

## Performance

- **Simulation time:** ~15-30 seconds per operation
- **Simulated time:** ~0.7ms per operation
- **Slowdown:** ~20,000x - 40,000x (expected for detailed simulation)

## Operations Available

1. rowand - Bitwise AND
2. rowadd - Addition
3. rowsub - Subtraction
4. rowmult - Multiplication
5. rowmin - Minimum
6. rowmax - Maximum
7. rowequal - Equality
8. rowgreater - Greater than
9. rowgreater_equal - Greater or equal
10. rowif_else - Ternary select
11. rowabs - Absolute value
12. rowbitcount - Population count

## Filtering Output

To filter COMMAND and WARN messages as requested:
```bash
./build/VEGA_X86/gem5.debug ... 2>&1 | grep -v -E "^(COMMAND|WARN)"
```

This is already built into the benchmark scripts.

## Troubleshooting

If you see errors, check:
1. gem5 VEGA_X86 build exists
2. GPU binary exists and is executable
3. You're running from the correct directory
4. Enough disk space for output (~10MB per run)

## Documentation

- **GPU_FIXES_COMPLETE.md** - Complete technical details of all fixes
- **VERIFICATION_TEST.md** - Test results proving everything works
- **GPU_IMPLEMENTATION_SUMMARY.md** - Original GPU implementation details

## Success Criteria

A successful run shows:
- "breaking loop due to: exiting with last active thread context"
- "Exiting because exiting with last active thread context"
- stats.txt file ~2.4MB in size
- Exit code 0

## Next Steps

Now that GPU benchmarks work, you can:
1. Run full benchmark suite for all operations
2. Compare GPU vs CPU vs CIM performance
3. Analyze memory traffic patterns
4. Generate performance graphs
5. Extract statistics with `extract_stats.py`

Enjoy your working GPU benchmarks! 🎉
