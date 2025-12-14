#include "pim_core.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace pim_core {

#define __NR_mmapPim 500

// in practice all these parameters would be read from the device tree exposed by the modified OS
// (for which one would need to create a custom OS image with `mmapPim` syscall implemented
static void *PIM_BASE_ADDR = (void*) 0x10000000;
static const size_t HUGE_PAGE_SIZE = 2 * 1024 * 1024; 	// 2 MiB
static size_t pim_pages_allocated = 0;

static const size_t NR_COLS_IN_MAT = 1024;
static const size_t BYTES_PER_MAT_ROW = NR_COLS_IN_MAT / 8;
static const size_t NR_ROWS_IN_MAT = 2048;
static const size_t MAT_SIZE_BYTES = NR_COLS_IN_MAT * NR_ROWS_IN_MAT / 8; // mat size in bytes
static const size_t MATS_PER_HUGE_PAGE = HUGE_PAGE_SIZE / MAT_SIZE_BYTES;

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
	FreeMatBlock free_blocks_head;

	// newly created mats are considered to be fully free
	MatMeta(void *virt_addr):
		virt_addr(virt_addr), free_blocks_head( FreeMatBlock{ virt_addr, NR_ROWS_IN_MAT, nullptr, nullptr})
	{};
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

    if (ret == MAP_FAILED) {
        perror("mmapPim syscall failed");
    } else {
		// store newly available mats and remember the virtual address they are mapped to (`ret`=start address of newly allocated huge page)
		for(int i=0; i<MATS_PER_HUGE_PAGE; ++i) {
			auto vaddr_of_mat = (char*) ret + MAT_SIZE_BYTES*i;
			mats.push_back(MatMeta(vaddr_of_mat));
		}
	}
    return ret;
}

void *find_free_space_in_mat(MatMeta* mat, const size_t size, const size_t mat_label)
{
	const size_t num_rows = ((size + BYTES_PER_MAT_ROW -1 ) / BYTES_PER_MAT_ROW) * BYTES_PER_MAT_ROW;
	auto block = &(mat->free_blocks_head);
	while(block!=nullptr) {
		size_t free_size_bytes = (block->nr_free_rows_in_block * NR_COLS_IN_MAT) / 8;
		if (free_size_bytes >= size) {
			// always choose the first `num_rows` inside this contiguous block
			block->nr_free_rows_in_block -= num_rows;
			block->virt_addr = ((char*) block->virt_addr) + num_rows * BYTES_PER_MAT_ROW;
			if (block->nr_free_rows_in_block==0) {
				// delete block from list of free blocks
				if (block->prev_free_mat_block)
					block->prev_free_mat_block->next_free_mat_block = block->next_free_mat_block;

				if (block->next_free_mat_block)
					block->next_free_mat_block->prev_free_mat_block = block->prev_free_mat_block;
			}

			mat_label_to_mat[mat_label] = mat;
			return block->virt_addr;
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

	// 1. TODO NEXT: Check if `mat_label` refers to an already allocated mat
	// - if the mat referred to by `mat_label` doesn't have enough space available anymore: OOM ?
	if (auto mat = mat_label_to_mat[mat_label]) {
		auto vaddr = find_free_space_in_mat(mat, size, mat_label);
		if (!vaddr)
			perror("PIM OOM");
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

	for(int i=0; i<MATS_PER_HUGE_PAGE; ++i){
		auto virt_addr = (char*) next_hugepage_start + MAT_SIZE_BYTES * i;
		mats.push_back(MatMeta(virt_addr));
	}

	// now the lastly added mat is guaranteed to be completely free
	return find_free_space_in_mat(&mats.back(), size, mat_label);
}

/// Frees the memory previously allocated with `pim_malloc()`
void pim_free(void* ptr) {
	// 1. Determine mat to which this physical address belongs
	// and update free blocks accordingly
}

}
