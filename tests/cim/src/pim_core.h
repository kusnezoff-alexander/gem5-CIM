#pragma once
#include <cstddef>
#include <cstdint>

namespace pim_core {

/** Allocates a new huge page if current huge page pool is not enough to fulfill request */
void* mmapPim(void* addr, size_t length, size_t mat_label);
/**
 * @param size in BYTES !!
 */
void* pim_malloc(size_t size, size_t mat_label);
void pim_free(void* ptr);

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

template<typename T>
static inline void rowif_else(T* dst, const T* src1, const T* src2, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src1;
	register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x4f\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rdx), "+r"(rcx), "+r"(r8)
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
static inline void rowabs(T* dst, const T* src1, const T* src2, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src1;
	register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x5b\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rdx), "+r"(rcx), "+r"(r8)
		:
		: "memory"
	);
}

template<typename T>
static inline void rowtrsp_init(T* dst, const T* src1, const T* src2, const size_t size, const size_t n) {
	register uint64_t rdi asm("rdi") = (uint64_t)dst;
	register uint64_t rsi asm("rsi") = (uint64_t)src1;
	register uint64_t rdx asm("rdx") = (uint64_t)src2;
	register uint64_t rcx asm("rcx") = size;
	register uint64_t r8 asm("r8") = n;

	asm volatile(
		".byte 0x66, 0x0F, 0x38, 0x5c\n"
		: "+r"(rdi), "+r"(rsi), "+r"(rdx), "+r"(rcx), "+r"(r8)
		:
		: "memory"
	);
}

}
