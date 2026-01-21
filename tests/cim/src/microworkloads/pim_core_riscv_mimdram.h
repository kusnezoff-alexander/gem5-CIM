#pragma once
#include <stddef.h>
#include <stdint.h>

// namespace pim_core {

/** Allocates a new huge page if current huge page pool is not enough to fulfill request */
void* mmapPim(void* addr, size_t length, size_t mat_label);
void* pim_malloc(size_t size, size_t mat_label);

static inline void rowand(void* dst, const void* src1, const void* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWAND opcode [0000010 <rs2> <rs1> 000 <rd> 11101 11]
    asm volatile(
    ".insn r 0x77, 0x1, 0x2, %0, %1, %2"
    :
    :"r"(dst), "r"(src1), "r"(src2)
    :"memory"
    );
}

static inline void rowor(void* dst, const void* src1, const void* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWOR opcode [0000011 <rs2> <rs1> 000 <rd> 11101 11]
    asm volatile(
    ".insn r 0x77, 0x1, 0x3, %0, %1, %2"
    :
    :"r"(dst), "r"(src1), "r"(src2)
    :"memory"
    );
}

static inline void rownot(void* dst, const void* src1, const void* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWNOT opcode [0000100 <rs2> <rs1> 000 <rd> 11101 11]
    asm volatile(
    ".insn r 0x77, 0x1, 0x4, %0, %1, %2"
    :
    :"r"(dst), "r"(src1), "r"(src2)
    :"memory"
    );
}


static inline void rowxor(void* dst, const void* src1, const void* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWXOR opcode [0000101 <rs2> <rs1> 001 <rd> 11101 11]
    asm volatile(
    ".insn r 0x77, 0x1, 0x5, %0, %1, %2"
    :
    :"r"(dst), "r"(src1), "r"(src2)
    :"memory"
    );
}
static inline void rowmaj3(void* dst, const void* src1, const void* src2) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWMAJ opcode [0000110 <rs2> <rs1> 001 <rd> 11101 11]
    asm volatile(
    ".insn r 0x77, 0x1, 0x6, %0, %1, %2"
    :
    :"r"(dst), "r"(src1), "r"(src2)
    :"memory"
    );
}

static inline void rowclone(void* dst, const void* src) {
    // dst, src1, src2 are just placeholders for registers or memory operands
    // This emits the raw ROWMAJ opcode [1111111 <rs2> <rs1> 010 <rd> 11101 11]
    asm volatile(
    ".insn r 0x77, 0x2, 0x7f, %0, %1, %2"
    :
    :"r"(dst), "r"(src), "r"(src)
    :"memory"
    );
}

// }
