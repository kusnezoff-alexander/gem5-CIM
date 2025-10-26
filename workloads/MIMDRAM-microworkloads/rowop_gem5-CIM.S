
#define TWO_BYTE_OP(name, number)         \
        .globl name;                      \
        .func name;                       \
name:                                     \
        .byte 0x66, 0x0F, 0x38;     \
        .word number;                     \
        ret;                              \
        .endfunc;

TWO_BYTE_OP(rowop_and, 0x42)
TWO_BYTE_OP(rowop_or,  0x43)
TWO_BYTE_OP(rowop_xor, 0x44)
TWO_BYTE_OP(rowop_not, 0x45)
TWO_BYTE_OP(rowop_ap,  0x46)
TWO_BYTE_OP(rowop_aap, 0x47)
