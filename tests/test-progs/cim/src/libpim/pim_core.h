#pragma once
#include <cstddef>
#include <cstdint>

namespace pim_core {

/** Allocates a new huge page if current huge page pool is not enough to fulfill request */
void* mmapPim(void* addr, size_t length, size_t mat_label);
void* pim_malloc(size_t size, size_t mat_label);

template<typename T>
static inline void rowand(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWAND opcode (0x66 0x0F 0x38 0x42)
    asm volatile(
        ".byte 0x66, 0x0F, 0x38, 0x42\n"
        :
        : "D"(dst), "S"(src1), "d"(src2)  // Example: dst->RDI, src1->RSI, src2->RDX
        : "memory"
    );
}

template<typename T>
static inline void rowor(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWAND opcode (0x66 0x0F 0x38 0x42)
    asm volatile(
        ".byte 0x66, 0x0F, 0x38, 0x43\n"
        :
        : "D"(dst), "S"(src1), "d"(src2)  // Example: dst->RDI, src1->RSI, src2->RDX
        : "memory"
    );
}

template<typename T>
static inline void rownot(T* dst, const T* src1) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWAND opcode (0x66 0x0F 0x38 0x42)
    asm volatile(
        ".byte 0x66, 0x0F, 0x38, 0x44\n"
        :
        : "D"(dst), "S"(src1) // Example: dst->RDI, src1->RSI
        : "memory"
    );
}

template<typename T>
static inline void rowxor(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWAND opcode (0x66 0x0F 0x38 0x42)
    asm volatile(
        ".byte 0x66, 0x0F, 0x38, 0x45\n"
        :
        : "D"(dst), "S"(src1), "d"(src2)  // Example: dst->RDI, src1->RSI, src2->RDX
        : "memory"
    );
}
template<typename T>
static inline void rowmaj3(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWAND opcode (0x66 0x0F 0x38 0x42)
    asm volatile(
        ".byte 0x66, 0x0F, 0x38, 0x46\n"
        :
        : "D"(dst), "S"(src1), "d"(src2)  // Example: dst->RDI, src1->RSI, src2->RDX
        : "memory"
    );
}

template<typename T>
static inline void rowclone(T* dst, const T* src1) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWAND opcode (0x66 0x0F 0x38 0x42)
    asm volatile(
        ".byte 0x66, 0x0F, 0x38, 0x47\n"
        :
        : "D"(dst), "S"(src1) // Example: dst->RDI, src1->RSI, src2->RDX
        : "memory"
    );
}

template<typename T>
static inline void rowtrsp_init(T* dst, const size_t size, const size_t elem_size) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWAND opcode (0x66 0x0F 0x38 0x42)
    asm volatile(
        ".byte 0x66, 0x0F, 0x38, 0x48\n"
        :
        : "D"(dst), "S"(size), "d"(elem_size)  // Example: dst->RDI, src1->RSI, src2->RDX
        : "memory"
    );
}

template<typename T>
static inline void rowsub(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWAND opcode (0x66 0x0F 0x38 0x42)
    asm volatile(
        ".byte 0x66, 0x0F, 0x38, 0x49\n"
        :
        : "D"(dst), "S"(src1), "d"(src2) // Example: dst->RDI, src1->RSI, src2->RDX
        : "memory"
    );
}


template<typename T>
static inline void rowmin(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWAND opcode (0x66 0x0F 0x38 0x42)
    asm volatile(
        ".byte 0x66, 0x0F, 0x38, 0x4a\n"
        :
        : "D"(dst), "S"(src1), "d"(src2) // Example: dst->RDI, src1->RSI, src2->RDX
        : "memory"
    );
}

template<typename T>
static inline void rowmax(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWAND opcode (0x66 0x0F 0x38 0x42)
    asm volatile(
        ".byte 0x66, 0x0F, 0x38, 0x4b\n"
        :
        : "D"(dst), "S"(src1), "d"(src2) // Example: dst->RDI, src1->RSI, src2->RDX
        : "memory"
    );
}

template<typename T>
static inline void rowright_shift(T* dst, const T* src1) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWAND opcode (0x66 0x0F 0x38 0x42)
    asm volatile(
        ".byte 0x66, 0x0F, 0x38, 0x4c\n"
        :
        : "D"(dst), "S"(src1) // Example: dst->RDI, src1->RSI, src2->RDX
        : "memory"
    );
}

template<typename T>
static inline void rowleft_shift(T* dst, const T* src1) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWAND opcode (0x66 0x0F 0x38 0x42)
    asm volatile(
        ".byte 0x66, 0x0F, 0x38, 0x4d\n"
        :
        : "D"(dst), "S"(src1) // Example: dst->RDI, src1->RSI, src2->RDX
        : "memory"
    );
}

}
