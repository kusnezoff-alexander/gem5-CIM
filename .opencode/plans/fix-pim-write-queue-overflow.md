# Fix PIM Write Queue Overflow

## Problem
PIM (RowOp) instructions trigger massive cache eviction storms that overflow the write queue buffer. A single PIM instruction can evict up to 384 cache blocks (128 iterations × 3 rows), but the write buffer only holds ~24 entries (8 + 16 reserve), causing `assert(!freeList.empty())` in `WriteQueue::allocate`.

## Root Cause
In `Cache::access()` (cache.cc:175-205), PIM packets evict all cache blocks overlapping with dest/src1/src2 rows. `doWritebacks()` then tries to allocate write buffer entries for ALL of them in a tight loop, overflowing the buffer.

## Solution: Capacity-Aware doWritebacks with Deferred Retry

### File Changes

### 1. `src/mem/cache/queue.hh` - Add freeCount() method (after isFull())

```cpp
    /**
     * Returns the number of free entries (including reserve).
     */
    int freeCount() const
    {
        return numEntries - allocated;
    }
```

### 2. `src/mem/cache/cache.hh` - Add deferred writeback infrastructure

Add to `Cache` class protected section (after `outstandingSnoop`):

```cpp
    /** Deferred writebacks that couldn't fit in the write buffer. */
    PacketList deferredWritebacks;

    /** Forward time for deferred writebacks. */
    Tick deferredWritebackForwardTime = 0;

    /** Process any deferred writebacks when write buffer space frees up. */
    void processDeferredWritebacks();
```

### 3. `src/mem/cache/cache.cc` - Modify doWritebacks() and add processDeferredWritebacks()

Replace `Cache::doWritebacks()` (lines 225-265) with:

```cpp
void
Cache::doWritebacks(PacketList& writebacks, Tick forward_time)
{
    while (!writebacks.empty()) {
        PacketPtr wbPkt = writebacks.front();

        // Call isCachedAbove for Writebacks, CleanEvicts and
        // WriteCleans to discover if the block is cached above.
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
                assert(wbPkt->cmd == MemCmd::WritebackDirty ||
                       wbPkt->cmd == MemCmd::WriteClean);
                wbPkt->setBlockCached();
            }
        }

        // Check if the write buffer has space before allocating.
        // Keep at least 1 entry free to avoid exhausting the reserve.
        if (writeBuffer.freeCount() <= 1) {
            // Defer remaining writebacks for later processing
            deferredWritebackForwardTime = forward_time;
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

Add new method after doWritebacks:

```cpp
void
Cache::processDeferredWritebacks()
{
    while (!deferredWritebacks.empty()) {
        if (writeBuffer.freeCount() <= 1) {
            DPRINTF(Cache, "Still %zu deferred writebacks, write buffer "
                    "full\n", deferredWritebacks.size());
            return;
        }

        PacketPtr wbPkt = deferredWritebacks.front();
        deferredWritebacks.pop_front();

        allocateWriteBuffer(wbPkt, deferredWritebackForwardTime);
    }

    DPRINTF(Cache, "All deferred writebacks processed\n");
}
```

### 4. `src/mem/cache/base.hh` - Hook retry into markInService

Modify `markInService(WriteQueueEntry *entry)` (lines 424-432) to call a virtual method when space frees:

First, add a virtual method declaration in BaseCache:
```cpp
    virtual void writeBufferFreed() {}
```

Then modify markInService:
```cpp
    void markInService(WriteQueueEntry *entry)
    {
        bool wasFull = writeBuffer.isFull();
        writeBuffer.markInService(entry);

        if (wasFull && !writeBuffer.isFull()) {
            clearBlocked(Blocked_NoWBBuffers);
        }

        // Notify subclass that write buffer space freed up
        writeBufferFreed();
    }
```

### 5. `src/mem/cache/cache.hh` - Override writeBufferFreed

Add to the protected section:
```cpp
    void writeBufferFreed() override;
```

### 6. `src/mem/cache/cache.cc` - Implement writeBufferFreed

```cpp
void
Cache::writeBufferFreed()
{
    if (!deferredWritebacks.empty()) {
        processDeferredWritebacks();
    }
}
```

## Build Command
```
scons build/X86/gem5.debug -j$(nproc)
```

## Test Command
```
build/X86/gem5.debug --debug-flags=RowOp --debug-start=0 --debug-file=/tmp/code/dprint3.log configs/cim/cim.py
```
