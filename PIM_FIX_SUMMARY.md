# PIM Cache Coherence Fix - Summary

## Problem Description
PIM (Processing-In-Memory) instructions in gem5 cause cache coherence issues:
- PIM RowOp instructions are marked UNCACHEABLE and bypass the cache hierarchy
- They trigger massive cache eviction storms (384 blocks = 3 rows × 128 blocks)
- Write buffer overflows causing crashes
- Even when not crashing, PIM reads stale data because writebacks complete AFTER the PIM operation executes

## Root Cause Analysis
1. **PIM packets bypass cache hierarchy**: Debug shows RowOp packets go CPU → L2Bus → MemBus → Memory Controller directly, NOT through L1/L2 caches
2. **Cache eviction happens but too late**: Even though L1 evicts overlapping blocks when PIM passes through, the writebacks arrive at DRAM AFTER the RowOp executes
3. **The timing issue**: 
   - Writebacks get deferred when write buffer is full
   - Deferred writebacks have later readyTime than PIM packet
   - Writebacks process AFTER PIM executes → PIM reads stale data

## Fix Approaches Attempted

### Approach 1: Defer writebacks + delay PIM (in gem5 cache code)
**Files modified:**
- `src/mem/cache/queue.hh` - Added `freeCount()` method
- `src/mem/cache/cache.hh` - Added deferred writeback infrastructure
- `src/mem/cache/cache.cc` - Modified `doWritebacks()` to defer with `curTick()`, added callback
- `src/mem/cache/base.hh` - Added `writeBufferFreed()` hook

**Status**: Didn't fully work - PIM packets bypass the cache entirely, so code wasn't triggered

### Approach 2: Flush cache before PIM (in C test code) - CURRENT
**Files modified:**
- `tests/test-progs/cim/src/pim_core.h` - Added `flush_cache_line()` and `flush_array_cache()` functions using `clflush` instruction
- `tests/test-progs/cim/src/pim_test_primitives.cpp` - Added `flush_array_cache()` calls before each PIM operation

**Status**: Crashes because clflush generates many writebacks that overflow the memory controller's write queue

## Current Status
- clflush approach crashes at memory controller write queue overflow
- Need to either:
  1. Increase memory controller write buffer size (parameter not found)
  2. Use a different flush approach (e.g., non-temporal stores)
  3. Revert to original working version and try different gem5-level fix

## Key Files to Look At

### For memory controller write queue:
- `src/mem/mem_ctrl.cc` - Memory controller implementation
- `src/mem/qos/mem_ctrl.cc` - QoS memory controller
- `src/mem/DRAMInterface.py` - DRAM parameters

### For cache-level fix:
- `src/mem/cache/cache.cc` - Cache access and writeback handling
- `src/mem/cache/base.cc` - Base cache implementation
- `src/mem/coherent_xbar.cc` - Crossbar where PIM packets bypass caches

### For C-level fix:
- `tests/test-progs/cim/src/pim_core.h` - PIM helper functions
- `tests/test-progs/cim/src/pim_test_primitives.cpp` - Test with PIM operations

## Debug Findings
- RowOp packets show in debug as: `WriteReq [0:37] UC` (UNCACHEABLE)
- They appear at L2Bus and MemBus but NOT at L1DCache
- This confirms PIM bypasses cache hierarchy entirely

## Last Working State
Commit `99c011bcad` - "Checkpoint: current state before PIM cache fix implementation"
