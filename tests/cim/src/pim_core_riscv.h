#pragma once
#include <stddef.h>
#include <stdint.h>

namespace pim_core {

/** Allocates a new huge page if current huge page pool is not enough to fulfill request */
void* mmapPim(void* addr, size_t length, size_t mat_label);
void* pim_malloc(size_t size, size_t mat_label);

template<typename T>
static inline void rowand(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWAND opcode [0000010 <rs2> <rs1> 001 <rd> 11110 11]
    asm volatile(
    ".insn r 0x5b, 0x1, 0x2, %0, %1, %2"
    :
    :"r"(dst), "r"(src1), "r"(src2)
    :"memory"
    );
}

template<typename T>
static inline void rowor(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWOR opcode [0000011 <rs2> <rs1> 001 <rd> 11110 11]
    asm volatile(
    ".insn r 0x5b, 0x1, 0x3, %0, %1, %2"
    :
    :"r"(dst), "r"(src1), "r"(src2)
    :"memory"
    );
}

template<typename T>
static inline void rownot(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWNOT opcode [0000100 <rs2> <rs1> 001 <rd> 11110 11]
    asm volatile(
    ".insn r 0x5b, 0x1, 0x4, %0, %1, %2"
    :
    :"r"(dst), "r"(src1), "r"(src2)
    :"memory"
    );
}


template<typename T>
static inline void rowxor(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWXOR opcode [0000101 <rs2> <rs1> 001 <rd> 11110 11]
    asm volatile(
    ".insn r 0x5b, 0x1, 0x5, %0, %1, %2"
    :
    :"r"(dst), "r"(src1), "r"(src2)
    :"memory"
    );
}


template<typename T>
static inline void rownand(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWNAND opcode [0000110 <rs2> <rs1> 001 <rd> 11110 11]
    asm volatile(
    ".insn r 0x5b, 0x1, 0x6, %0, %1, %2"
    :
    :"r"(dst), "r"(src1), "r"(src2)
    :"memory"
    );
}


template<typename T>
static inline void rownor(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWNOR opcode [0000111 <rs2> <rs1> 001 <rd> 11110 11]
    asm volatile(
    ".insn r 0x5b, 0x1, 0x7, %0, %1, %2"
    :
    :"r"(dst), "r"(src1), "r"(src2)
    :"memory"
    );
}


template<typename T>
static inline void rowmaj3(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWMAJ opcode [0000110 <rs2> <rs1> 001 <rd> 10110 11]
    asm volatile(
    ".insn r 0x5b, 0x0, 0x0, %0, %1, %2"
    :
    :"r"(dst), "r"(src1), "r"(src2)
    :"memory"
    );
}


template<typename T>
static inline void rowclone(T* dst, const T* src) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWMAJ opcode [1111111 <rs2> <rs1> 010 <rd> 11110 11]
    asm volatile(
    ".insn r 0x5b, 0x0, 0x0, %0, %1, %2"
    :
    :"r"(dst), "r"(src), "r"(src)
    :"memory"
    );
}

}
