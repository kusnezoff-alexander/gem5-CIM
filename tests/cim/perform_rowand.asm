; Compilation:
; nasm -f elf64 test_my_add42.asm -o test_my_add42.o
; ld test_my_add42.o -o test_my_add42
BITS 64
GLOBAL _start

SECTION .data
start db "Starting ROWAND...", 0xA  ; string + newline
start_len equ $ - start 			; length of string
end db "Finished ROWAND...", 0xA  	; string + newline
end_len equ $ - end 				; length of string

SECTION .text
_start:
	call alloc_and_touch_mem

	; debug printing after ROWAND has been executed
	mov rdi, start 			; str pointer
    mov rsi, start_len      ; str length
    call print_string 		; call our function

 	; mov rax, 10          ; Set RAX = 10
	mov rdi, 0x9567890		; dst
	mov rsi, 0x9678901		; src1
	; mov rdx, 0x9789012		; src2
	mov rdx, 0x9678901		; src2
	db 0x66, 0x0F, 0x38, 0x42 ; ROWAND
	align 16

	; debug printing after ROWAND has been executed
	align 16
	mov rdi, end 			; str pointer
    mov rsi, end_len      	; str length
    call print_string 		; call our function

    ; Exit syscall
    mov rax, 60          ; syscall number for exit
    xor rdi, rdi 		 ; exit code 0
    syscall


; Simulated data present in pages (and thus in memory) to be computed upon
alloc_and_touch_mem:
    ; Call mmap_alloc to map memory at addresses to be used by CIM-Ops
	; and simulate an access to those pages
    mov rdi, 0x9567000       ; addr dest (must be page-aligned)
    call mmap_alloc
    mov rdi, rax
    mov qword [rdi], 42


	; After mmap, the mapped address is in RAX
	; simulated access to addresses (that are later used in CIM-computation)
	; since the data we operate on is assumed to already reside in the memory
	; (not causing any PageFaults)
	; REMEMBER: a single mem-operation only causes one `PageFault` (for a single page) normally
    mov rdi, 0x9678000       ; addr src1 (must be page-aligned)
    call mmap_alloc
    mov rdi, rax
    mov qword [rdi], 42

    mov rdi, 0x9789000       ; addr src2 (must be page-aligned)
    call mmap_alloc
    mov rdi, rax
    mov qword [rdi], 42

	ret

; mmap_alloc(addr):
; Allocates one 4KB page at the given address.
; Argument:
;   rdi = desired address (must be page-aligned)
; Return:
;   rax = mapped address (or -1 on error)
mmap_alloc:
    mov     rax, 9          ; syscall number for mmap
    mov     rsi, 0x1000     ; length = 1 page
    mov     rdx, 3          ; PROT_READ | PROT_WRITE
    mov     r10, 0x32       ; MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED
    mov     r8, -1          ; fd = -1
    mov     r9, 0           ; offset = 0
    syscall
    ret

; ----------------------------------------
; void print_string(char* str, uint64 len)
; Arguments:
;   rdi = address of string
;   rsi = length of string
; ----------------------------------------
print_string:
    mov rax, 1        ; syscall: write
    mov rdx, rsi      ; length
    mov rsi, rdi      ; string pointer
    mov rdi, 1        ; fd = stdout
    syscall
    ret
