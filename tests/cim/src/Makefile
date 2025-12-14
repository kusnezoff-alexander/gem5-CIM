all: libpim.a pim_full_program

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

clean:
	rm -f *.o libpim.a pim_full_program
