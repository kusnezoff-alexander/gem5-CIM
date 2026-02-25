#pragma once
#include <cstddef>
#include <cstdint>

namespace pim_core {

#define __NR_mmapPim 500

// in practice all these parameters would be read from the device tree exposed by the modified OS
// (for which one would need to create a custom OS image with `mmapPim` syscall implemented
static const void *PIM_BASE_ADDR = (void*) 0x10000000;
static const size_t HUGE_PAGE_SIZE = 2 * 1024 * 1024; 	// 2 MiB
static size_t pim_pages_allocated = 0;

// see /gem5-CIM-fix/src/mem/DRAMInterface.py for config
static const size_t NR_COLS_IN_MAT = 1024;
static const size_t BYTES_PER_MAT_ROW = NR_COLS_IN_MAT / 8;
static const size_t NR_ROWS_IN_MAT = 2048;
static const size_t MAT_SIZE_BYTES = NR_COLS_IN_MAT * NR_ROWS_IN_MAT / 8; // mat size in bytes
static const size_t MATS_PER_HUGE_PAGE = HUGE_PAGE_SIZE / MAT_SIZE_BYTES;
static const size_t NR_HUGEPAGES = 20;
static const size_t NR_MATS = NR_HUGEPAGES * MATS_PER_HUGE_PAGE;

/** Allocates a new huge page if current huge page pool is not enough to fulfill request */
void* mmapPim(void* addr, size_t length, size_t mat_label);
/**
 * @param size in BYTES !!
 */
void* pim_malloc(size_t size, size_t mat_label);
void pim_free(void* ptr);

static inline void flush_cache_line(void* addr) {
    asm volatile("clflush %0" : : "m"(*(char*)addr) : "memory");
}

static const size_t CACHE_LINE_SIZE = 64;

template<typename T>
static inline void flush_array_cache(T* ptr, size_t n) {
    char* char_ptr = reinterpret_cast<char*>(ptr);
    size_t total_bytes = n * sizeof(T);

    for (size_t i = 0; i < total_bytes; i += CACHE_LINE_SIZE) {
        flush_cache_line(char_ptr + i);
    }

    asm volatile("mfence" : : : "memory");
}

/**
 * @oaram n in **BITS** (since we also allow for <1byte, eg 4bit operands are supported
 */
template<typename T>
static inline void rowand(T* dst, const T* src1, const T* src2, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src1;
	register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x42\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rdx), "+r"(rcx), "+r"(r8)
		:
		: "memory"
	);
}

template<typename T>
static inline void rowor(T* dst, const T* src1, const T* src2, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src1;
	register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x43\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rdx), "+r"(rcx), "+r"(r8)
		:
		: "memory"
	);
}

template<typename T>
static inline void rownot(T* dst, const T* src, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x44\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rcx), "+r"(r8)
		:
		: "memory"
	);
}

template<typename T>
static inline void rowxor(T* dst, const T* src1, const T* src2, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src1;
	register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x45\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rdx), "+r"(rcx), "+r"(r8)
		:
		: "memory"
	);
}

template<typename T>
static inline void rowadd(T* dst, const T* src1, const T* src2, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src1;
	register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x46\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rdx), "+r"(rcx), "+r"(r8)
		:
		: "memory"
	);
}

template<typename T>
static inline void rowsub(T* dst, const T* src1, const T* src2, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src1;
	register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x47\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rdx), "+r"(rcx), "+r"(r8)
		:
		: "memory"
	);
}

template<typename T>
static inline void rowmult(T* dst, const T* src1, const T* src2, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src1;
	register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x48\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rdx), "+r"(rcx), "+r"(r8)
		:
		: "memory"
	);
}

template<typename T>
static inline void rowdiv(T* dst, const T* src1, const T* src2, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src1;
	register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x49\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rdx), "+r"(rcx), "+r"(r8)
		:
		: "memory"
	);
}

template<typename T>
static inline void rowmin(T* dst, const T* src1, const T* src2, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src1;
	register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x4a\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rdx), "+r"(rcx), "+r"(r8)
		:
		: "memory"
	);
}

template<typename T>
static inline void rowmax(T* dst, const T* src1, const T* src2, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src1;
	register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x4b\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rdx), "+r"(rcx), "+r"(r8)
		:
		: "memory"
	);
}

template<typename T>
static inline void rowequal(T* dst, const T* src1, const T* src2, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src1;
	register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x4c\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rdx), "+r"(rcx), "+r"(r8)
		:
		: "memory"
	);
}

template<typename T>
static inline void rowgreater(T* dst, const T* src1, const T* src2, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src1;
	register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x4d\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rdx), "+r"(rcx), "+r"(r8)
		:
		: "memory"
	);
}

template<typename T>
static inline void rowgreater_equal(T* dst, const T* src1, const T* src2, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src1;
	register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x4e\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rdx), "+r"(rcx), "+r"(r8)
		:
		: "memory"
	);
}

// TODO: make `mask` use a single bit per element (instead of setting all 1s of T)
template<typename T>
static inline void rowif_else(T* dst, const T* src1, const T* src2, const T* mask, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src1;
	register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t r9 asm("r9") = (uint64_t)mask;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x4f\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rdx), "+r"(rcx), "+r"(r8), "+r"(r9)
		:
		: "memory"
	);
}

template<typename T>
static inline void rowbitcount(T* dst, const T* src1, const T* src2, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src1;
	register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x5a\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rdx), "+r"(rcx), "+r"(r8)
		:
		: "memory"
	);
}

template<typename T>
static inline void rowabs(T* dst, const T* src, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x5b\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rcx), "+r"(r8)
		:
		: "memory"
	);
}

template<typename T>
static inline void rowtrsp_init(T* arr, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)arr;
	// register uint64_t rsi asm("rsi") = (uint64_t)src1;
	// register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x5c\n"
		: "+r"(rdi), "+r"(rcx), "+r"(r8) // "+r"(rsi), "+r"(rdx),
		:
		: "memory"
	);
}

}
