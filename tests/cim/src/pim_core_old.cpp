#include "pim_core.h"
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

#define __NR_mmapPim 500

// in practice all these parameters would be read from the device tree exposed by the modified OS
// (for which one would need to create a custom OS image with `mmapPim` syscall implemented
static size_t pim_base_addr = 0x10000000;
static const size_t HUGE_PAGE_SIZE = 2 * 1024 * 1024; 	// 2 MiB
static size_t pim_pages_allocated = 0;

static const size_t NR_COLS_IN_MAT = 1024;
static const size_t NR_ROWS_IN_MAT = 2048;
static const size_t MAT_SIZE = NR_COLS_IN_MAT * NR_ROWS_IN_MAT / 8; // mat size in bytes
static const size_t MATS_PER_HUGE_PAGE = HUGE_PAGE_SIZE / MAT_SIZE;

namespace pim_core {


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
    }
    return ret;
}

/// Performs allocation of operands in the region reserved for PIM
void* pim_malloc(size_t size, size_t mat_label) {
	void* ptr = (void*) (pim_base_addr + pim_pages_allocated * HUGE_PAGE_SIZE);
    mmapPim(ptr, size, mat_label);
    if (!ptr) {
        std::fprintf(stderr, "pim_malloc: failed to allocate %zu bytes\n", size);
    }
	pim_pages_allocated++;
    return ptr;
}

/// Frees the memory previously allocated with `pim_malloc()`
void* pim_free(size_t size, size_t mat_label) {
}

}
