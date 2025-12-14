section .data
    hex_prefix      db "0x"
    hex_prefix_len  equ $ - hex_prefix

    ; 16 hex chars + newline
    hexbuf          db "0000000000000000", 10
    hexbuf_len      equ $ - hexbuf


section .text
global _start

_start:
	; Alloc 1st huge page
    mov     rax, 500         ; syscall number
    ; mov     rdi, 0x0F7000000 ; Start vaddr
    mov     rdi, 0x10000000 ; Start vaddr
    mov 	rsi, 456         ; Region size
    mov 	rdx, 789         ; Mat label
    syscall

	; Alloc 2nd huge page
    mov     rax, 500
    ; mov     rdi, 0x0F8000000 ; Start vaddr (=next huge page)
    mov     rdi, 0x11000000; Start vaddr (=next huge page)
    mov 	rsi, 456
    mov 	rdx, 789
    syscall

	; Write some value into huge page 1
	; mov     rdi, qword 0x0F8000000   ; absolute address
	mov     rdi, qword 0x10000000   ; absolute address
	mov     byte [rdi], 0x41
    ; Read written value from before and print
    movzx rax, byte [rdi]
    call print_rax_hex       ; should print 0x42

	; allocate normal page
    mov rdi, 0x9789000       ; addr (page aligned)
	call mmap_alloc          ; returns addr in rax
	call print_rax_hex       ; print returned pointer

	; make sure memory allocated for huge page can't be mapped
	; (syscall should return -EINVAL)
    ; mov rdi, qword 0x0F8000000; addr (page aligned)
    mov rdi, qword   0x10000000; addr (page aligned)
	call mmap_alloc          ; returns addr in rax
	call print_rax_hex       ; print returned pointer

    ; Exit
    mov     rax, 60
    xor     rdi, rdi
    syscall


; --------------------------------------------------------
; mmap_alloc(addr)
; Allocates one 4KB page at the given address.
; rdi = address
; return rax = mapped address
; --------------------------------------------------------
mmap_alloc:
    mov     rax, 9          ; mmap
    mov     rsi, 0x1000     ; length
    mov     rdx, 3          ; PROT_READ|PROT_WRITE
    mov     r10, 0x32       ; MAP_PRIVATE|ANON|FIXED
    mov     r8, -1          ; fd
    mov     r9, 0           ; offset
    syscall
    ret


; --------------------------------------------------------
; print_rax_hex
; Prints RAX in hex as: 0xXXXXXXXXXXXXXXX\n
; Preserves rbx, rcx, rdx, rsi, rdi, r11
; --------------------------------------------------------
print_rax_hex:
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r11

    mov rbx, rax            ; copy value to convert

    ; Convert → hex string
    mov rcx, 16
    mov rdx, hexbuf + 15    ; fill backwards

.hex_loop:
    mov rax, rbx
    and rax, 0xF
    cmp al, 10
    jl .digit
    add al, 'A' - 10
    jmp .store
.digit:
    add al, '0'
.store:
    mov [rdx], al
    shr rbx, 4
    dec rdx
    loop .hex_loop

    ; Write "0x"
    mov rax, 1
    mov rdi, 1
    mov rsi, hex_prefix
    mov rdx, hex_prefix_len
    syscall

    ; Write hex digits + newline
    mov rax, 1
    mov rdi, 1
    mov rsi, hexbuf
    mov rdx, hexbuf_len
    syscall

    ; Restore
    pop r11
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    ret
