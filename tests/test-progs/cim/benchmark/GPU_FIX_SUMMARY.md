# GPU Benchmark Fixes for gem5 VEGA_X86

## Issues Fixed

### 1. ISA Initialization Error (FIXED ✓)
**Error:** `fatal: Number of ISAs (0) assigned to the CPU does not equal number of threads (1).`

**Root Cause:** Command Processor CPUs were not having their ISAs initialized.

**Fix:** Modified `configs/example/apu_se.py` line 833-835:
```python
for cp in cp_list:
    cp.createThreads()  # Added this line
    cp.workload = host_cpu.workload
```

The `createThreads()` method automatically creates the appropriate ISA (X86ISA) for each CPU thread.

**File:** `configs/example/apu_se.py`

---

### 2. Ruby Network Configuration Error (FIXED ✓)
**Error:** `fatal: system.ruby.sqc_cntrl1.mandatoryQueue without default or user set value`

**Root Cause:** Command Processor SQC (Shared Queue Cache) controllers were missing network message buffer initialization.

**Fix:** Modified `configs/ruby/GPU_VIPER.py` lines 901-911 (after line 900):
```python
# Connect the SQC controller to the ruby network
sqc_cntrl.requestFromSQC = MessageBuffer(ordered=True)
sqc_cntrl.requestFromSQC.out_port = network.in_port

sqc_cntrl.probeToSQC = MessageBuffer(ordered=True)
sqc_cntrl.probeToSQC.in_port = network.out_port

sqc_cntrl.responseToSQC = MessageBuffer(ordered=True)
sqc_cntrl.responseToSQC.in_port = network.out_port

sqc_cntrl.mandatoryQueue = MessageBuffer()
```

This mirrors the initialization done for regular SQC controllers at lines 802-811.

**File:** `configs/ruby/GPU_VIPER.py`

---

## Remaining Issues

### 3. VMA (Virtual Memory Area) Initialization Error (PENDING ⚠️)
**Error:** `gem5.debug: src/sim/vma.cc:106: void gem5::VMA::sanityCheck(): Assertion '_addrRange.start() != _addrRange.end()' failed.`

**Location:** Occurs during process initialization in `X86Process::argsInit()`

**Root Cause:** The GPU program is trying to map a huge page region with an invalid address range (start == end).

**Potential Causes:**
1. GPU workload memory requirements are not compatible with gem5's SE mode
2. Huge page pool configuration might be incorrect for GPU workloads
3. ROCm runtime initialization might be trying to allocate memory in a way that's not supported

**Possible Solutions:**
1. **Use a simpler GPU test program:** Create a minimal GPU test that doesn't require complex memory mappings
2. **Modify huge page pool settings:** Adjust `--mem-size`, `huge_page_pool_base`, etc.
3. **Use Full System (FS) mode:** GPU workloads might need FS mode instead of SE mode
4. **Debug the VMA mapping:** Add debugging to see what address ranges are being requested

**Related Code:**
- `src/sim/vma.cc:106` - VMA sanity check
- `src/arch/x86/process.cc` - X86Process::argsInit()
- `src/sim/mem_state.cc` - MemState::mapHugePageRegion()

---

## Files Modified

1. **configs/example/apu_se.py**
   - Line 833-835: Added `cp.createThreads()` for Command Processors

2. **configs/ruby/GPU_VIPER.py**
   - Lines 901-911: Added message buffer initialization for CP SQC controllers

3. **tests/test-progs/cim/benchmark/run_gpu_benchmarks.sh**
   - Created new GPU benchmark script (similar to run_benchmarks.sh)

---

## Testing Status

| Test | Status | Notes |
|------|--------|-------|
| gem5 builds successfully | ✓ | VEGA_X86 build complete |
| GPU binary compiles | ✓ | `pim_test_gpu` built with hipcc |
| gem5 starts with GPU config | ✓ | Passes initial checks |
| ISA initialization | ✓ | Fixed |
| Ruby network setup | ✓ | Fixed |
| Process initialization | ✗ | VMA assertion failure |
| GPU kernel execution | - | Not reached yet |

---

## Next Steps

### Short-term (to get GPU benchmarks running):

1. **Option A: Create a simpler GPU test**
   - Write a minimal HIP program that doesn't use ROCm runtime
   - Just allocate device memory and run a simple kernel
   - Avoid huge page allocations

2. **Option B: Debug the VMA issue**
   - Add debug prints to `MemState::mapHugePageRegion()`
   - Check what addresses are being requested
   - See if we can adjust the memory layout to avoid zero-length ranges

3. **Option C: Try different gem5 GPU examples**
   - Look at existing gem5 GPU test programs
   - See if they work with current configuration
   - Adapt our benchmark to match their structure

### Long-term (for production use):

1. **Full System Mode**
   - GPU workloads may work better in FS mode
   - Requires Linux kernel and ROCm drivers
   - Much slower but more accurate

2. **Update gem5**
   - Check if newer gem5 versions have better GPU support
   - May have fixes for SE mode GPU issues

3. **Contribute fixes upstream**
   - If we solve the VMA issue, submit patch to gem5
   - Help improve gem5's GPU support

---

## Command to Run GPU Benchmarks

Once the VMA issue is resolved:

```bash
cd /home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix/tests/test-progs/cim/benchmark
./run_gpu_benchmarks.sh
```

Or run individual tests:

```bash
cd /home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix

./build/VEGA_X86/gem5.debug \
    configs/example/apu_se.py \
    --num-compute-units=4 \
    --num-cp=1 \
    --cpu-type=X86TimingSimpleCPU \
    -n 1 \
    --mem-size=512MB \
    -c tests/test-progs/cim/benchmark/pim_test_gpu \
    --options="1 --check" \
    2>&1 | grep -v -E "^(COMMAND|WARN)"
```

---

## Summary

**What works:**
- gem5 VEGA_X86 build compiles successfully
- GPU program (pim_test_gpu) compiles with hipcc
- gem5 can load and start the GPU configuration
- CPU and Command Processor ISAs are properly initialized
- Ruby memory network is correctly configured

**What doesn't work yet:**
- Process initialization fails with VMA assertion
- GPU kernels cannot execute because process doesn't start

**Impact:**
- Cannot run GPU benchmarks in gem5 yet
- Need to resolve VMA issue or find alternative approach
- CPU/CIM benchmarks are unaffected and work fine
