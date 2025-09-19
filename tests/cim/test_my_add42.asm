; Compilation:
; nasm -f elf64 test_my_add42.asm -o test_my_add42.o
; ld test_my_add42.o -o test_my_add42
BITS 64
GLOBAL _start

SECTION .text
_start:
    mov rax, 10          ; Set RAX = 10

	; TODO: change this to a CIM-instruction code
    db 0x0F, 0xAA        ; Our custom instruction: my_add42

    mov rbx, rax         ; Copy RAX to RBX (should be 52)

    ; Exit syscall
    mov rax, 60          ; syscall number for exit
    xor rdi, rdi
    syscall
