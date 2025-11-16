# === Reserve a memory region for CIM via a device driver in Linux
section .data
    path db "/dev/cim", 0       ; device path
    length equ 8*1024*1024      ; 8 MB

section .bss
    fd resq 1
    addr resq 1

section .text
    global _start

_start:
    ; --- open("/dev/cim", O_RDWR) ---
    mov rax, 2                  ; sys_open
    lea rdi, [rel path]         ; filename
    mov rsi, 2                  ; O_RDWR
    xor rdx, rdx                ; mode
    syscall
    test rax, rax
    js open_failed              ; if rax < 0, jump

    mov [fd], rax

    ; --- mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0) ---
    mov rax, 9                  ; sys_mmap
    xor rdi, rdi                ; addr = NULL
    mov rsi, length
    mov rdx, 0x3                ; PROT_READ|PROT_WRITE
    mov r10, 0x01               ; MAP_SHARED
    mov r8, [fd]
    xor r9, r9                  ; offset = 0
    syscall
    test rax, rax
    js mmap_failed
    mov [addr], rax

    ; --- exit(0) ---
    mov rax, 60
    xor rdi, rdi
    syscall

open_failed:
    ; write error code for open
    mov rax, 60
    mov rdi, 1                  ; exit code 1
    syscall

mmap_failed:
    ; write error code for mmap
    mov rax, 60
    mov rdi, 2                  ; exit code 2
    syscall
