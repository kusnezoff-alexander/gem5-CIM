#ifdef __x86_64__
	#include "../pim_core_x86.h"
#else
	#include "../pim_core_riscv.h"
#endif
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include "dims.h"
#include "../../../../include/gem5/m5ops.h"

using namespace pim_core;

#define VERIFY 1
using dtype = uint32_t;

int main(int argc, char* argv[])
{
	srand(123456);


	dtype* list_arrays[N_ARRAYS];
	for(uint32_t i=0; i<N_ARRAYS; ++i) {
		list_arrays[i] = static_cast<dtype*>(pim_malloc(sizeof(dtype)*N_ELEMS, 0));
		// std::printf("Ran pim_malloc and got ptr=%p\n", list_arrays[i]);
	}

	// 1. Write data
	for(uint32_t i=0; i<N_ARRAYS; ++i) {
		for(uint32_t j=0; j<N_ELEMS; ++j) {
			list_arrays[i][j] = rand();
		}
	}

	m5_reset_stats(0, 0);
	m5_work_begin(0,0);
	for(uint32_t i=1; i<N_ARRAYS; ++i) {
		rowand(list_arrays[0], list_arrays[0], list_arrays[i]);
	}
	m5_work_end(0,0);
	m5_dump_stats(0,0);
	return 0;
}
