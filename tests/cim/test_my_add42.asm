; Compilation:
; nasm -f elf64 test_my_add42.asm -o test_my_add42.o
; ld test_my_add42.o -o test_my_add42
BITS 64
GLOBAL _start

SECTION .text
_start:
    mov rax, 10          ; Set RAX = 10
	mov rdi, 0x123456		; dst
	mov rsi, 0x123457		; src1
	mov rdx, 0x123458		; src2

	; mov byte [0x1000], 42	; example mem-access (for debugging `writeMem()`)
	; TODO: change this to a CIM-instruction code
    ; db 0x0F, 0xAA        ; Our custom instruction: my_add42
    ; db 0x0F, 0xAB, 0xD8         ; BT RAX, RCX
	; db 0x0F, 0xA4, 0xD8, 0x01   ; SHLD EAX, EBX, 1
	db 0x0F, 0xA6, 0x00           ; ROWAND instruction

    mov rbx, rax         ; Copy RAX to RBX (should be 52)

    ; Exit syscall
    mov rax, 60          ; syscall number for exit
    xor rdi, rdi
    syscall
