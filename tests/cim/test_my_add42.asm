; Compilation:
; nasm -f elf64 test_my_add42.asm -o test_my_add42.o
; ld test_my_add42.o -o test_my_add42
BITS 64
GLOBAL _start

SECTION .text
_start:

    ; Call mmap_alloc to map memory at 0x123000
    call mmap_alloc

    ; After mmap, the mapped address is in RAX
    ; We'll use RAX + 0x456 = 0x123456
    mov rdi, rax
    mov qword [rdi + 0x456], 42

 	; mov rax, 10          ; Set RAX = 10
	mov rdi, 0x123456		; dst
	mov rsi, 0x123457		; src1
	mov rdx, 0x123458		; src2

	; mov qword [rdi], 42		; put sth at this addr (maybe PageFault "unmapped address" is caused by this?)
	; mov byte [0x1000], 42	; example mem-access (for debugging `writeMem()`)
	; TODO: change this to a CIM-instruction code
    ; db 0x0F, 0xAA        ; Our custom instruction: my_add42
    ; db 0x0F, 0xAB, 0xD8         ; BT RAX, RCX
	; db 0x0F, 0xA4, 0xD8, 0x01   ; SHLD EAX, EBX, 1
	;db 0x0F, 0xA6, 0x00           ; ROWAND instruction
	db 0x0F, 0xA6 		          ; ROWAND instruction (without 0x00: no syscall Error??)

    ; Exit syscall
    mov rax, 60          ; syscall number for exit
    xor rdi, rdi 		 ; exit code 0
    syscall

; "Function" to allocate memory at a fixed address
mmap_alloc:
    ; mmap(addr=0x123000, length=0x1000, prot=RW,
    ;      flags=MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, fd=-1, offset=0)

    mov rax, 9              ; syscall number for mmap
    mov rdi, 0x123000       ; addr (must be page-aligned)
    mov rsi, 0x1000         ; length = 1 page
    mov rdx, 3              ; PROT_READ | PROT_WRITE
    mov r10, 0x32           ; MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED
    mov r8, -1              ; fd = -1
    mov r9, 0               ; offset = 0
    syscall
    ret                     ; return with mapped address in RAX
