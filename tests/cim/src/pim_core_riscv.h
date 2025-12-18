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
    // This emits the raw ROWAND opcode [0000000 <rs2> <rs1> 010 <rd> 11101 11]
    asm volatile(
    ".insn r 0x77, 2, 0x0, %0, %1, %2"
    :
    :"r"(dst), "r"(src1), "r"(src2)
    );
}

template<typename T>
static inline void rowor(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWOR opcode [0000000 <rs2> <rs1> 011 <rd> 11101 11]
    asm volatile(
    ".insn r 0x77, 3, 0, %0, %1, %2"
    :
    :"r"(dst), "r"(src1), "r"(src2)
    );
}

template<typename T>
static inline void rownot(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWNOT opcode [0000000 <rs2> <rs1> 100 <rd> 11101 11]
    asm volatile(
    ".insn r 0x77, 4, 0, %0, %1, %2"
    :
    :"r"(dst), "r"(src1), "r"(src2)
    );
}


template<typename T>
static inline void rowxor(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWXOR opcode [0000000 <rs2> <rs1> 101 <rd> 11101 11]
    asm volatile(
    ".insn r 0x77, 5, 0, %0, %1, %2"
    :
    :"r"(dst), "r"(src1), "r"(src2)
    );
}
template<typename T>
static inline void rowmaj3(T* dst, const T* src1, const T* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWMAJ opcode [0000000 <rs2> <rs1> 110 <rd> 11101 11]
    asm volatile(
    ".insn r 0x77, 6, 0, %0, %1, %2"
    :
    :"r"(dst), "r"(src1), "r"(src2)
    );
}
template<typename T>
static inline void rowclone(T* dst, const T* src) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWMAJ opcode [0000000 <rs2> <rs1> 111 <rd> 11101 11]
    asm volatile(
    ".insn r 0x77, 7, 0, %0, %1, %2"
    :
    :"r"(dst), "r"(src), "r"(src)
    );
}

}
