section .text
global _start

_start:
    mov     rax, 500         ; syscall number
    xor     rdi, rdi         ; arg1 (optional, here 0)
    xor     rsi, rsi         ; arg2 (optional, here 0)
    xor     rdx, rdx         ; arg3 (optional, here 0)
    syscall                  ; invoke syscall

    ; Exit normally (sys_exit)
    mov     rax, 60          ; syscall: exit
    xor     rdi, rdi         ; exit code 0
    syscall
