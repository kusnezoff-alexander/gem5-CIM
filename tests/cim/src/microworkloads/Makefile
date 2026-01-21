all: target/x86/libpim.a target/x86/pim_full_program

# Cross compile to rsicv
31_and_reduction-plus.o: 31_and_reduction-plus.c pim_core_riscv.h mimdram.h
	riscv64-linux-gnu-gcc -Wall -Wextra -g -O0 -c 31_and_reduction-plus.c -o 31_and_reduction-plus.o

31_and_reduction-plus: 31_and_reduction-plus.o
	riscv64-linux-gnu-gcc -Wall -Wextra -g -O0 31_and_reduction-plus.o -static -o 31_and_reduction-plus

clean:
	rm 31_and_reduction-plus 31_and_reduction-plus.o
