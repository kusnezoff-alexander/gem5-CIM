all: libpim.a pim_full_program

test: libpim.a pim_test_primitives

# Build object files for the library
pim_core.o: pim_core.cpp
	g++ -Wall -Wextra -O2 -c pim_core.cpp -o pim_core.o

# Create the static library
libpim.a: pim_core.o
	ar rcs libpim.a pim_core.o

# Build the program
pim_full_program.o: pim_full_program.cpp pim_core.h
	# Include pim_core.h to ensure template rowand is visible
	g++ -Wall -Wextra -O2 -c pim_full_program.cpp -o pim_full_program.o

pim_full_program: pim_full_program.o libpim.a
	g++ -Wall -Wextra -O2 pim_full_program.o -L. -lpim -o pim_full_program

pim_test_primitives: pim_test_primitives.cpp libpim.a
	g++ -std=c++20 -Wall -Wextra -O2 -c pim_test_primitives.cpp -o pim_test_primitives.o
	g++ -Wall -Wextra -O2 pim_test_primitives.o -L. -lpim -o pim_test_primitives

clean:
	rm -f *.o libpim.a pim_full_program pim_test_primitives
