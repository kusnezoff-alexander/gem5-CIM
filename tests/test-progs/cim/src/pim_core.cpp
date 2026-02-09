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
	void *virt_addr = nullptr; // TODO
	// Number of rows that are free in this mat
	FreeMatBlock* free_blocks_head;

	// newly created mats are considered to be fully free
	MatMeta(void *virt_addr):
		// TODO: store all `NR_MATS` here !!
		virt_addr(virt_addr)
	{
		FreeMatBlock* block = new FreeMatBlock{ virt_addr, NR_ROWS_IN_MAT, nullptr, nullptr};
		free_blocks_head = block;
		for (size_t i=0; i<NR_MATS-1; ++i) {
			auto next_block = new FreeMatBlock{ virt_addr, NR_ROWS_IN_MAT, nullptr, block};
			block -> next_free_mat_block = next_block;
			block = next_block;
		}
	};
};

/** For freeing allocated rows. */
struct AllocationHeader {
    size_t num_rows;      // Number of rows allocated (including header)
    MatMeta* mat;         // Which mat this belongs to
};

// track available mats
std::vector<MatMeta> mats;
using Mat = uint64_t;
using Row = uint64_t;
using DRAMLocation = std::pair<Mat, Row>;
std::unordered_map<void*, DRAMLocation> mappedAddresses;
using MatLabel = size_t;
std::unordered_map<MatLabel, MatMeta*> mat_label_to_mat;

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
	const size_t num_rows = (size + BYTES_PER_MAT_ROW - 1 ) / BYTES_PER_MAT_ROW;
	auto block = mat->free_blocks_head;
	while(block!=nullptr) {
		if (block->nr_free_rows_in_block >= num_rows) {
			// always choose the first `num_rows` inside this contiguous block
			auto allocated_addr = block->virt_addr;
			block->virt_addr = ((char*) block->virt_addr) + num_rows * BYTES_PER_MAT_ROW; // update addr to next free row

			block->nr_free_rows_in_block -= num_rows;
			if (block->nr_free_rows_in_block==0) {
				// delete block from list of free blocks
				if (block->prev_free_mat_block)
					block->prev_free_mat_block->next_free_mat_block = block->next_free_mat_block;

				if (block->next_free_mat_block)
					block->next_free_mat_block->prev_free_mat_block = block->prev_free_mat_block;
			}

			mat_label_to_mat[mat_label] = mat;
			return allocated_addr;
		}
		block = block->next_free_mat_block;

		if (!block) {
			cerr << "No free mats left" << endl;
			return nullptr;
		}
	}
	return nullptr;
}

/// Performs allocation of operands in the region reserved for PIM
void* pim_malloc(const size_t size, const size_t mat_label) {

	// sanity check
	if (size > MAT_SIZE_BYTES) {
		// Operand needs to fit into a single mat. Else ask the user to split `size` into multiple requests
		return nullptr;
	}

    auto it = mat_label_to_mat.find(mat_label);
	if (mat_label_to_mat.size() >= NR_MATS && it == mat_label_to_mat.end()) {
		cerr << "ERROR: out of mats (have " << mat_label_to_mat.size()
			<< " labels, max is " << NR_MATS << ")" << endl;
		abort();
	}

   	// 1. Check if `mat_label` refers to an already allocated mat
	// - if the mat referred to by `mat_label` doesn't have enough space available anymore: OOM
    if (it != mat_label_to_mat.end()) {
        // Mat with this label already exists
        auto mat = it->second;
        auto vaddr = find_free_space_in_mat(mat, size, mat_label);
        if (!vaddr) {
			cout << "Mat: " << mat << ", Mat Label: " << mat_label << ", Size: " << size << endl;
            perror("PIM OOM");
            return nullptr;
        }
        return vaddr;
    }

	// Else check if there is some space in the already allocated PIM mats
	for(auto& mat: mats) {
		auto vaddr = find_free_space_in_mat(&mat, size, mat_label);
		if (vaddr)
			return vaddr;
	}


	// If there is not enough space available: Allocate new huge page
	void* next_hugepage_start = ((char*) PIM_BASE_ADDR) + (pim_pages_allocated * HUGE_PAGE_SIZE);
    auto ret = mmapPim(next_hugepage_start, size, mat_label);
	if (ret == MAP_FAILED || !next_hugepage_start) {
        std::fprintf(stderr, "pim_malloc: failed to allocate %zu bytes\n", size);
		return nullptr;
	}
	pim_pages_allocated++;

	for(size_t i=0; i<MATS_PER_HUGE_PAGE; ++i){
		auto virt_addr = (char*) next_hugepage_start + MAT_SIZE_BYTES * i;
		mats.push_back(MatMeta(virt_addr));
	}

	// now the lastly added mat is guaranteed to be completely free
	return find_free_space_in_mat(&mats.back(), size, mat_label);
}

/// Frees the memory previously allocated with `pim_malloc()`
void pim_free(void* ptr) {
    if (!ptr) {
        return;  // Freeing nullptr is a no-op (like standard free)
    }

    // 1. Find which mat this address belongs to
    MatMeta* target_mat = nullptr;
    size_t row_offset = 0;

    for (auto& mat : mats) {
        void* mat_start = mat.virt_addr;
        void* mat_end = (char*)mat_start + MAT_SIZE_BYTES;

        if (ptr >= mat_start && ptr < mat_end) {
            target_mat = &mat;
            // Calculate which row this address corresponds to
            row_offset = ((char*)ptr - (char*)mat_start) / BYTES_PER_MAT_ROW;
            break;
        }
    }

    if (!target_mat) {
        cerr << "ERROR: pim_free called with invalid pointer " << ptr << endl;
        return;
    }

    // 2. TODO: We need to know how many rows to free

    cerr << "ERROR: pim_free not fully implemented - need allocation size tracking" << endl;
}

}
