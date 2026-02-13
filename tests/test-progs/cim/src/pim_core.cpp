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

/** Tracks <u>contiguous</u> free space inside a mat */
struct FreeMatBlock {
    void * virt_addr;
    // free blocks are tracked at row granularity
    size_t nr_free_rows_in_block;
    FreeMatBlock* next_free_mat_block;
    FreeMatBlock* prev_free_mat_block;
};

/** Stores info about free rows in a mat */
class MatMeta {
  public:
    // Virtual address that maps to this DRAM mat
    void *virt_addr = nullptr;
    // Number of rows that are free in this mat
    FreeMatBlock* free_blocks_head;

    // newly created mats are considered to be fully free
    MatMeta(void *virt_addr):
        virt_addr(virt_addr)
    {
        // Create ONE block representing all free rows in this mat
        free_blocks_head = new FreeMatBlock{ virt_addr, NR_ROWS_IN_MAT, nullptr, nullptr};
    };
};

/** For freeing allocated rows - stored CPU-side */
struct AllocationHeader {
    size_t num_rows;  // Number of rows allocated
    MatMeta* mat;     // Which mat this belongs to
    void* alloc_start; // Start of allocation
};

// track available mats
std::vector<MatMeta> mats;
using MatLabel = size_t;
std::unordered_map<MatLabel, MatMeta*> mat_label_to_mat;

// CPU-side allocation tracking - no PIM memory wasted
static std::unordered_map<void*, AllocationHeader> alloc_headers;

void* mmapPim(void* addr,
              size_t length,
              size_t mat_label)
{
    void* ret = (void*) syscall(__NR_mmapPim,
                                addr,
                                length,
                                mat_label);

    if (ret == MAP_FAILED)
        perror("mmapPim syscall failed");
    return ret;
}

void *find_free_space_in_mat(MatMeta* mat, const size_t size, const size_t mat_label)
{
    const size_t num_rows = (size + BYTES_PER_MAT_ROW - 1) / BYTES_PER_MAT_ROW;
    auto block = mat->free_blocks_head;

    while (block != nullptr) {
        if (block->nr_free_rows_in_block >= num_rows) {
            // Allocate from the START of this free block
            auto allocated_addr = block->virt_addr;

            // Shrink the block by moving its start forward
            block->virt_addr = ((char*) block->virt_addr) + num_rows * BYTES_PER_MAT_ROW;
            block->nr_free_rows_in_block -= num_rows;

            if (block->nr_free_rows_in_block == 0) {
                // Block is fully used - remove from list
                if (block == mat->free_blocks_head) {
                    // Removing head
                    mat->free_blocks_head = block->next_free_mat_block;
                    if (block->next_free_mat_block) {
                        block->next_free_mat_block->prev_free_mat_block = nullptr;
                    }
                } else {
                    // Removing middle/end node
                    if (block->prev_free_mat_block) {
                        block->prev_free_mat_block->next_free_mat_block = block->next_free_mat_block;
                    }
                    if (block->next_free_mat_block) {
                        block->next_free_mat_block->prev_free_mat_block = block->prev_free_mat_block;
                    }
                }

                delete block;
            }

            mat_label_to_mat[mat_label] = mat;
            return allocated_addr;
        }
        block = block->next_free_mat_block;
    }

    return nullptr;
}

/// Performs allocation of operands in the region reserved for PIM
void* pim_malloc(const size_t size, const size_t mat_label) {
    // sanity check
    if (size > MAT_SIZE_BYTES) {
        return nullptr;
    }

    auto it = mat_label_to_mat.find(mat_label);
    if (mat_label_to_mat.size() >= NR_MATS && it == mat_label_to_mat.end()) {
        cerr << "ERROR: out of mats (have " << mat_label_to_mat.size()
            << " labels, max is " << NR_MATS << ")" << endl;
        abort();
    }

    // Helper lambda: allocate from a specific mat and register header
    auto allocate_from_mat = [&](MatMeta* mat) -> void* {
        auto vaddr = find_free_space_in_mat(mat, size, mat_label);
        if (!vaddr) return nullptr;

        const size_t num_rows = (size + BYTES_PER_MAT_ROW - 1) / BYTES_PER_MAT_ROW;
        alloc_headers[vaddr] = AllocationHeader{ num_rows, mat, vaddr };
        return vaddr;
    };

    // 1. Check if mat_label refers to an already allocated mat
    if (it != mat_label_to_mat.end()) {
        auto* ptr = allocate_from_mat(it->second);
        if (!ptr) {
            cerr << "PIM OOM on mat " << mat_label << endl;
            return nullptr;
        }
        return ptr;
    }

    // 2. Check if there is some space in already allocated PIM mats
    for (auto& mat : mats) {
        auto* ptr = allocate_from_mat(&mat);
        if (ptr) return ptr;
    }

    // 3. Allocate new huge page
    void* next_hugepage_start = ((char*) PIM_BASE_ADDR) + (pim_pages_allocated * HUGE_PAGE_SIZE);
    auto ret = mmapPim(next_hugepage_start, size, mat_label);
    if (ret == MAP_FAILED || !next_hugepage_start) {
        std::fprintf(stderr, "pim_malloc: failed to allocate %zu bytes\n", size);
        return nullptr;
    }
    pim_pages_allocated++;

    for (size_t i = 0; i < MATS_PER_HUGE_PAGE; ++i) {
        auto virt_addr = (char*)next_hugepage_start + MAT_SIZE_BYTES * i;
        mats.push_back(MatMeta(virt_addr));
    }

    return allocate_from_mat(&mats.back());
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
    MatMeta* mat      = header.mat;
    size_t   num_rows = header.num_rows;

    // Insert a new free block back into the mat's free list
    auto* new_block = new FreeMatBlock{
        ptr,                        // virt_addr: freed region starts here
        num_rows,                   // nr_free_rows_in_block
        mat->free_blocks_head,      // next
        nullptr                     // prev
    };

    if (mat->free_blocks_head)
        mat->free_blocks_head->prev_free_mat_block = new_block;

    mat->free_blocks_head = new_block;

    // Remove from CPU-side tracking
    alloc_headers.erase(it);

    // Coalesce adjacent free blocks to prevent fragmentation
    auto* block = mat->free_blocks_head;
    while (block && block->next_free_mat_block) {
        auto* next = block->next_free_mat_block;
        void* block_end = (char*)block->virt_addr + block->nr_free_rows_in_block * BYTES_PER_MAT_ROW;

        if (block_end == next->virt_addr) {
            // Blocks are adjacent - merge them
            block->nr_free_rows_in_block += next->nr_free_rows_in_block;
            block->next_free_mat_block    = next->next_free_mat_block;

            if (next->next_free_mat_block)
                next->next_free_mat_block->prev_free_mat_block = block;

            delete next;
            // Don't advance - check if newly merged block is adjacent to next one
        } else {
            block = block->next_free_mat_block;
        }
    }
}

} // namespace pim_core
