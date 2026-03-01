#include "pim_core.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

using namespace std;

namespace pim_core {

/** Tracks <u>contiguous</u> free space inside a subarray */
struct FreeSubarrayBlock {
    void * virt_addr;
    // free blocks are tracked at row granularity
    size_t nr_free_rows_in_block;
    FreeSubarrayBlock* next_free_subarray_block;
    FreeSubarrayBlock* prev_free_subarray_block;
};

/** Stores info about free rows in a subarray */
class SubarrayMeta {
  public:
    // Virtual address that maps to this DRAM subarray
    void *virt_addr = nullptr;
    // Number of rows that are free in this subarray
    FreeSubarrayBlock* free_blocks_head;

    // newly created subarrays are considered to be fully free
    SubarrayMeta(void *virt_addr):
        virt_addr(virt_addr)
    {
        // Create ONE block representing all free rows in this subarray
        free_blocks_head = new FreeSubarrayBlock{ virt_addr, NR_ROWS_IN_SUBARRAY, nullptr, nullptr};
    };
};

/** For freeing allocated rows - stored CPU-side */
struct AllocationHeader {
    size_t num_rows;  // Number of rows allocated
    SubarrayMeta* subarray;     // Which subarray this belongs to
    void* alloc_start; // Start of allocation
};

// track available subarrays
std::vector<SubarrayMeta> subarrays;
using SubarrayLabel = size_t;
std::unordered_map<SubarrayLabel, SubarrayMeta*> subarray_label_to_subarray;

// CPU-side allocation tracking - no PIM memory wasted
static std::unordered_map<void*, AllocationHeader> alloc_headers;

void* mmapPim(void* addr,
              size_t length,
              size_t subarray_label)
{
    void* ret = (void*) syscall(__NR_mmapPim,
                                addr,
                                length,
                                subarray_label);

    if (ret == MAP_FAILED)
        perror("mmapPim syscall failed");
    return ret;
}

void *find_free_space_in_subarray(SubarrayMeta* subarray, const size_t size, const size_t subarray_label)
{
    const size_t num_rows = (size + BYTES_PER_SUBARRAY_ROW - 1) / BYTES_PER_SUBARRAY_ROW;
    auto block = subarray->free_blocks_head;

    while (block != nullptr) {
        if (block->nr_free_rows_in_block >= num_rows) {
            // Allocate from the START of this free block
            auto allocated_addr = block->virt_addr;

            // Shrink the block by moving its start forward
            block->virt_addr = ((char*) block->virt_addr) + num_rows * BYTES_PER_SUBARRAY_ROW;
            block->nr_free_rows_in_block -= num_rows;

            if (block->nr_free_rows_in_block == 0) {
                // Block is fully used - remove from list
                if (block == subarray->free_blocks_head) {
                    // Removing head
                    subarray->free_blocks_head = block->next_free_subarray_block;
                    if (block->next_free_subarray_block) {
                        block->next_free_subarray_block->prev_free_subarray_block = nullptr;
                    }
                } else {
                    // Removing middle/end node
                    if (block->prev_free_subarray_block) {
                        block->prev_free_subarray_block->next_free_subarray_block = block->next_free_subarray_block;
                    }
                    if (block->next_free_subarray_block) {
                        block->next_free_subarray_block->prev_free_subarray_block = block->prev_free_subarray_block;
                    }
                }

                delete block;
            }

            subarray_label_to_subarray[subarray_label] = subarray;
            return allocated_addr;
        }
        block = block->next_free_subarray_block;
    }

    return nullptr;
}

/// Performs allocation of operands in the region reserved for PIM
void* pim_malloc(const size_t size, const size_t subarray_label) {
    // sanity check
    if (size > SUBARRAY_SIZE_BYTES) {
        return nullptr;
    }

    auto it = subarray_label_to_subarray.find(subarray_label);
    if (subarray_label_to_subarray.size() >= NR_SUBARRAYS && it == subarray_label_to_subarray.end()) {
        cerr << "ERROR: out of subarrays (have " << subarray_label_to_subarray.size()
            << " labels, max is " << NR_SUBARRAYS << ")" << endl;
        abort();
    }

    // Helper lambda: allocate from a specific subarray and register header
    auto allocate_from_subarray = [&](SubarrayMeta* subarray) -> void* {
        auto vaddr = find_free_space_in_subarray(subarray, size, subarray_label);
        if (!vaddr) return nullptr;

        const size_t num_rows = (size + BYTES_PER_SUBARRAY_ROW - 1) / BYTES_PER_SUBARRAY_ROW;
        alloc_headers[vaddr] = AllocationHeader{ num_rows, subarray, vaddr };
        return vaddr;
    };

    // 1. Check if subarray_label refers to an already allocated subarray
    if (it != subarray_label_to_subarray.end()) {
        auto* ptr = allocate_from_subarray(it->second);
        if (!ptr) {
            cerr << "PIM OOM on subarray " << subarray_label << endl;
            return nullptr;
        }
        return ptr;
    }

    // 2. Check if there is some space in already allocated PIM subarrays
    for (auto& subarray : subarrays) {
        auto* ptr = allocate_from_subarray(&subarray);
        if (ptr) return ptr;
    }

    // 3. Allocate new huge page
    void* next_hugepage_start = ((char*) PIM_BASE_ADDR) + (pim_pages_allocated * HUGE_PAGE_SIZE);
    auto ret = mmapPim(next_hugepage_start, size, subarray_label);
    if (ret == MAP_FAILED || !next_hugepage_start) {
        std::fprintf(stderr, "pim_malloc: failed to allocate %zu bytes\n", size);
        return nullptr;
    }
    pim_pages_allocated++;

    for (size_t i = 0; i < SUBARRAYS_PER_HUGE_PAGE; ++i) {
        auto virt_addr = (char*)next_hugepage_start + SUBARRAY_SIZE_BYTES * i;
        subarrays.push_back(SubarrayMeta(virt_addr));
    }

    return allocate_from_subarray(&subarrays.back());
}

/// Frees the memory previously allocated with `pim_malloc()`
void pim_free(void* ptr) {
    if (!ptr) return;

    // Look up allocation header
    auto it = alloc_headers.find(ptr);
    if (it == alloc_headers.end()) {
        cerr << "ERROR: pim_free called with unknown pointer " << ptr << endl;
        return;
    }

    const AllocationHeader& header = it->second;
    SubarrayMeta* subarray      = header.subarray;
    size_t   num_rows = header.num_rows;

    // Insert a new free block back into the subarray's free list
    auto* new_block = new FreeSubarrayBlock{
        ptr,                        // virt_addr: freed region starts here
        num_rows,                   // nr_free_rows_in_block
        subarray->free_blocks_head, // next
        nullptr                     // prev
    };

    if (subarray->free_blocks_head)
        subarray->free_blocks_head->prev_free_subarray_block = new_block;

    subarray->free_blocks_head = new_block;

    // Remove from CPU-side tracking
    alloc_headers.erase(it);

    // Coalesce adjacent free blocks to prevent fragmentation
    auto* block = subarray->free_blocks_head;
    while (block && block->next_free_subarray_block) {
        auto* next = block->next_free_subarray_block;
        void* block_end = (char*)block->virt_addr + block->nr_free_rows_in_block * BYTES_PER_SUBARRAY_ROW;

        if (block_end == next->virt_addr) {
            // Blocks are adjacent - merge them
            block->nr_free_rows_in_block += next->nr_free_rows_in_block;
            block->next_free_subarray_block    = next->next_free_subarray_block;

            if (next->next_free_subarray_block)
                next->next_free_subarray_block->prev_free_subarray_block = block;

            delete next;
            // Don't advance - check if newly merged block is adjacent to next one
        } else {
            block = block->next_free_subarray_block;
        }
    }
}

} // namespace pim_core
