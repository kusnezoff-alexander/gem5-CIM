# Complete Fix Plan for PIM Write Queue Overflow

## Problem Summary
1. PIM instructions evict up to 384 cache blocks (3 rows × 128 blocks)
2. Cache write buffer only has ~32 entries → overflow crash
3. Deferred writebacks must reach DRAM BEFORE the PIM packet (correctness)
4. Must not trigger memory controller's pre-existing off-by-one assertion

## Root Causes
- **Crash**: doWritebacks() allocates all writebacks in one loop, exceeding write buffer capacity
- **Correctness**: Deferred writebacks get allocated AFTER the PIM packet → lower priority in queue
- **Mem controller assertion**: writeQueueFull() uses `>` but drain assertion uses `<`

## Solution: Capacity-Aware Deferred Writebacks with Earlier Timing

### Key Insight
Use `curTick()` (current time) as the readyTime for deferred writebacks. Since PIM has a future readyTime (e.g., 2 cycles in future), deferred writebacks with curTick() will be sorted BEFORE PIM in the readyList, ensuring correct ordering.

---

## Implementation Steps

### Step 1: Add freeCount() to Queue class
**File**: `src/mem/cache/queue.hh`

Add after `isFull()` method:
```cpp
int freeCount() const {
    return numEntries - allocated;
}
```

### Step 2: Add deferred writeback infrastructure to Cache
**File**: `src/mem/cache/cache.hh`

Add to protected section (after `outstandingSnoop`):
```cpp
PacketList deferredWritebacks;
Tick deferredWritebackForwardTime = 0;

void processDeferredWritebacks();
void writeBufferFreed() override;
```

### Step 3: Add virtual hook to BaseCache
**File**: `src/mem/cache/base.hh`

Add in `markInService(WriteQueueEntry *entry)` after `clearBlocked()`:
```cpp
writeBufferFreed();  // Notify subclass that write buffer space freed
```

Add virtual method declaration in BaseCache class:
```cpp
virtual void writeBufferFreed() {}
```

### Step 4: Modify doWritebacks() in Cache
**File**: `src/mem/cache/cache.cc`

Replace `doWritebacks()` with capacity-aware version that defers excess writebacks:
```cpp
void Cache::doWritebacks(PacketList& writebacks, Tick forward_time)
{
    while (!writebacks.empty()) {
        PacketPtr wbPkt = writebacks.front();

        if (isCachedAbove(wbPkt)) {
            if (wbPkt->cmd == MemCmd::CleanEvict) {
                delete wbPkt;
                writebacks.pop_front();
                continue;
            } else if (wbPkt->cmd == MemCmd::WritebackClean) {
                assert(writebackClean);
                delete wbPkt;
                writebacks.pop_front();
                continue;
            } else {
                wbPkt->setBlockCached();
            }
        }

        // Check write buffer capacity before allocating
        if (writeBuffer.freeCount() <= 1) {
            // Defer remaining writebacks - use curTick() so they're
            // ordered BEFORE the PIM packet (which has future readyTime)
            deferredWritebackForwardTime = curTick();
            while (!writebacks.empty()) {
                deferredWritebacks.push_back(writebacks.front());
                writebacks.pop_front();
            }
            DPRINTF(Cache, "Deferring %zu writebacks, write buffer full\n",
                    deferredWritebacks.size());
            break;
        }

        allocateWriteBuffer(wbPkt, forward_time);
        writebacks.pop_front();
    }
}
```

### Step 5: Add processDeferredWritebacks() implementation
**File**: `src/mem/cache/cache.cc`

Add after `doWritebacks()`:
```cpp
void Cache::processDeferredWritebacks()
{
    while (!deferredWritebacks.empty()) {
        if (writeBuffer.freeCount() <= 1) {
            DPRINTF(Cache, "Still %zu deferred writebacks\n",
                    deferredWritebacks.size());
            return;
        }

        PacketPtr wbPkt = deferredWritebacks.front();
        deferredWritebacks.pop_front();

        allocateWriteBuffer(wbPkt, deferredWritebackForwardTime);
    }

    DPRINTF(Cache, "All deferred writebacks processed\n");
}
```

### Step 6: Add writeBufferFreed() implementation
**File**: `src/mem/cache/cache.cc`

Add after `processDeferredWritebacks()`:
```cpp
void Cache::writeBufferFreed()
{
    if (!deferredWritebacks.empty()) {
        processDeferredWritebacks();
    }
}
```

---

## Why This Works

1. **No crash**: Capacity check prevents exceeding write buffer
2. **Correct ordering**: Deferred writebacks use curTick() which is NOW, while PIM uses future readyTime. Sorted readyList means writebacks go first.
3. **No mem controller issue**: Uses curTick() (immediate), not a large future time. Entries drain faster, never overfilling mem controller.

---

## Build & Test

```bash
scons build/X86/gem5.debug -j$(nproc)
build/X86/gem5.debug --debug-flags=RowOp --debug-start=0 --debug-file=/tmp/code/dprint3.log configs/cim/cim.py
```
