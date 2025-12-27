all: libpim/libpim.a pim_full_program

# Build object files for the library
 libpim/pim_core.o: libpim/pim_core.cpp
	g++ -Wall -Wextra -O2 -c libpim/pim_core.cpp -o libpim/pim_core.o

# Create the static library
libpim/libpim.a: libpim/pim_core.o
	ar rcs libpim/libpim.a libpim/pim_core.o

# Build the program
pim_full_program.o: pim_full_program.cpp libpim/pim_core.h
	# Include pim_core.h to ensure template rowand is visible
	g++ -Wall -Wextra -O2 -c pim_full_program.cpp -o pim_full_program.o

pim_full_program: pim_full_program.o libpim/libpim.a
	g++ -Wall -Wextra -O2 pim_full_program.o -Llibpim -lpim -o pim_full_program

clean:
	rm -f *.o libpim/*.a libpim/*.o pim_full_program
