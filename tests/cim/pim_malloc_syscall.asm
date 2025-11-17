section .text
global _start

_start:
    mov     rax, 500         ; syscall number
    mov     rdi, 123         ; arg1 (optional, here 0)
    mov 	rsi, 456         ; arg2 (optional, here 0)
    mov 	rdx, 789         ; arg3 (optional, here 0)
    syscall                  ; invoke syscall

	; do it twice
    mov     rax, 500         ; syscall number
    mov     rdi, 0x20007B    ; Start vaddr (=next huge page)
    mov 	rsi, 456         ; Region size
    mov 	rdx, 789         ; Mat label
    syscall                  ; invoke syscall

    ; Exit normally (sys_exit)
    mov     rax, 60          ; syscall: exit
    xor     rdi, rdi         ; exit code 0
    syscall
