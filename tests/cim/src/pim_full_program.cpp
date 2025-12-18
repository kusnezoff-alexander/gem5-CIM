#ifdef __x86_64__
	#include "pim_core_x86.h"
#else
	#include "pim_core_riscv.h"
#endif
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>

using namespace pim_core;

#define VERIFY 1
const size_t N_ELEMS = 3;

void test_every_rowop()
{

#ifdef VERIFY
	auto array1_initial_val = static_cast<uint16_t*>(malloc(N_ELEMS));
	auto array2_initial_val = static_cast<uint16_t*>(malloc(N_ELEMS));
#endif
	auto array1 = static_cast<uint16_t*>(pim_malloc(N_ELEMS, 0));
	auto array2 = static_cast<uint16_t*>(pim_malloc(N_ELEMS, 0));
	std::printf("Ran pim_malloc and got ptr array1=%p, array2=%p\n", array1, array2);

	// 1. Write data
	for(int i=0; i<N_ELEMS; ++i) {
		// array1[i] = 1;
		array1[i] = i;
		array2[i] = ~i;

#ifdef VERIFY
		array1_initial_val[i] = i;
		array2_initial_val[i] = ~i;
#endif
	}

	// TODO: perform ROWAND !
	// rowand(array1, array2, array1);
	// rowand(array1, array2, array1);
	// rowand(array1, array2, array1);

	// 2. Read result data back in (and check that it is true)
	for(int i=0; i<N_ELEMS; ++i) {
		// make sure both arrays have now the correct results stored inside
#ifdef VERIFY
		auto res_should = array1_initial_val[i] & array2_initial_val[i];
		if(array1[i] != res_should)
			std::printf("WRONG: array1[%d]=%d but should be %d\n", i, array1[i], res_should);
		else
			std::printf("Correct result for index=%d\n", i);
#endif
	}

	// rowor(array1, array2, array1);
	// rowor(array1, array2, array1);
	// rowor(array1, array2, array1);

}

using dtype = uint16_t;

int main(int argc, char* argv[])
{

	auto array1 = static_cast<dtype*>(pim_malloc(sizeof(dtype)*N_ELEMS, 0));
	auto array2 = static_cast<dtype*>(pim_malloc(sizeof(dtype)*N_ELEMS, 0));
	// 1. Write data
	for(uint16_t i=0; i<N_ELEMS; ++i) {
		// array1[i] = 1;
		array1[i] = i;
		array2[i] = ~i;
	}

	std::printf("Ran pim_malloc and got ptr array1=%p, array2=%p\n", array1, array2);
	rowand(array1, array2, array1);

	for(uint16_t i=0; i<N_ELEMS; ++i) {
		std::printf("%d, %d\n", array1[i], array2[i]);
	}
	return 0;
}
