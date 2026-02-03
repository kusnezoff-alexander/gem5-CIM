all: target/x86/libpim.a target/x86/pim_full_program

# Build object files for the library
target/x86/pim_core.o: pim_core.cpp
	g++ -Wall -Wextra -O2 -c pim_core.cpp -o target/x86/pim_core.o

target/riscv/pim_core.o: pim_core.cpp
	riscv64-linux-gnu-g++ -Wall -Wextra -g -O0 -c pim_core.cpp -o target/riscv/pim_core.o

# Create the static library
target/x86/libpim.a: target/x86/pim_core.o
	ar rcs target/x86/libpim.a target/x86/pim_core.o

# Create the static library
target/riscv/libpim.a: target/riscv/pim_core.o
	ar rcs target/riscv/libpim.a target/riscv/pim_core.o

# Build the program
target/x86/pim_full_program.o: pim_full_program.cpp pim_core_x86.h
	# Include pim_core.h to ensure template rowand is visible
	g++ -Wall -Wextra -O2 -c pim_full_program.cpp -o target/x86/pim_full_program.o

target/x86/pim_full_program: target/x86/pim_full_program.o target/x86/libpim.a
	g++ -Wall -Wextra -O2 target/x86/pim_full_program.o -Ltarget/x86 -lpim -o target/x86/pim_full_program

# Cross compile to rsicv
target/riscv/pim_full_program.o: pim_full_program.cpp pim_core_riscv.h
	riscv64-linux-gnu-g++ -Wall -Wextra -g -O0 -c pim_full_program.cpp -o target/riscv/pim_full_program.o

target/riscv/pim_full_program: target/riscv/pim_full_program.o target/riscv/libpim.a
	riscv64-linux-gnu-g++ -Wall -Wextra -g -O0 target/riscv/pim_full_program.o -static -Ltarget/riscv -lpim -o target/riscv/pim_full_program

target/riscv/pim_and_reduce.o: microworkloads/pim_and_reduce.cpp pim_core_riscv.h
	riscv64-linux-gnu-g++ -Wall -Wextra -g -O0 -c microworkloads/pim_and_reduce.cpp -o target/riscv/pim_and_reduce.o

target/riscv/pim_and_reduce: target/riscv/pim_and_reduce.o target/riscv/libpim.a
	riscv64-linux-gnu-g++ -Wall -Wextra -g -O0 \
	target/riscv/pim_and_reduce.o \
	-static \
	-Ltarget/riscv \
	-lpim \
	-L../../../util/m5/build/riscv/out \
	-lm5 \
	-o target/riscv/pim_and_reduce

	# -I../../../include \

target/riscv/baseline_and_reduce.o: microworkloads/baseline_and_reduce.cpp pim_core_riscv.h
	riscv64-linux-gnu-g++ -Wall -Wextra -g -O0 -c microworkloads/baseline_and_reduce.cpp -o target/riscv/baseline_and_reduce.o

target/riscv/baseline_and_reduce: target/riscv/baseline_and_reduce.o target/riscv/libpim.a
	riscv64-linux-gnu-g++ -Wall -Wextra -g -O0 \
	target/riscv/baseline_and_reduce.o \
	-static \
	-Ltarget/riscv \
	-lpim \
	-L../../../util/m5/build/riscv/out \
	-lm5 \
	-o target/riscv/baseline_and_reduce

target/riscv/pim_primitive_and_reduce.o: microworkloads/pim_primitive_and_reduce.cpp pim_core_riscv.h
	riscv64-linux-gnu-g++ -Wall -Wextra -g -O0 -c microworkloads/pim_primitive_and_reduce.cpp -o target/riscv/pim_primitive_and_reduce.o

target/riscv/pim_primitive_and_reduce: target/riscv/pim_primitive_and_reduce.o target/riscv/libpim.a
	riscv64-linux-gnu-g++ -Wall -Wextra -g -O0 \
	target/riscv/pim_primitive_and_reduce.o \
	-static \
	-Ltarget/riscv \
	-lpim \
	-L../../../util/m5/build/riscv/out \
	-lm5 \
	-o target/riscv/pim_primitive_and_reduce

microworkloads: target/riscv/baseline_and_reduce target/riscv/pim_and_reduce target/riscv/pim_primitive_and_reduce

clean:
	rm -f target/riscv/* target/x86/*
