#include "pim_core.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <sys/types.h>
#include <concepts>
#include <functional>
#include <iostream>

using namespace pim_core;
using namespace std;

const size_t N_ELEMS = 300'000;
const size_t N_ROWOPS = 7;
size_t next_mat = 0;

void init_data(uint16_t*& array1, uint16_t*& array2, uint16_t*& array1_initial_val, uint16_t*& array2_initial_val)
{
	// 1. Write data
	for(size_t i=0; i<N_ELEMS; ++i) {
		array1[i] = i;
		array2[i] = 0x111;

		array1_initial_val[i] = array1[i];
		array2_initial_val[i] = array2[i];
	}
}

void init_data_fuzzy(uint16_t*& array1, uint16_t*& array2,
               uint16_t*& array1_initial_val, uint16_t*& array2_initial_val, bool is_div=false)
{
    // Random number generator
    random_device rd;   // Seed
    mt19937 gen(rd());  // Mersenne Twister engine
    uniform_int_distribution<uint16_t> dist(0, 0xFFFF); // full range of uint16_t

    for(size_t i = 0; i < N_ELEMS; ++i) {
        array1[i] = dist(gen);
        array2[i] = dist(gen);

		if (is_div && array2[i]==0)
			array2[i] = 1; // avoid div by 0

        array1_initial_val[i] = array1[i];
        array2_initial_val[i] = array2[i];
    }
}

/**
 * @returns Whether check was correct
 */
template<std::integral T, typename Op>
bool check_result(T* res, T* array1_initial_val, T* array2_initial_val, Op op)
{
	bool is_correct = true;
	// 2. Read result data back in (and check that it is true)
	for(size_t i=0; i<N_ELEMS; ++i) {
		// make sure both arrays have now the correct results stored inside

		auto res_should = op(array1_initial_val[i], array2_initial_val[i]);
		if(res[i] != res_should) {
			std::printf("WRONG: array1[%zu]=%d but should be %d (values: %d / %d)\n", i, res[i], res_should,
					array1_initial_val[i], array2_initial_val[i]);
		}
	}
	return is_correct;
}

using dtype = uint16_t;
bool test_every_rowop()
{
	size_t nr_correct = 0;
	auto array1_initial_val = static_cast<uint16_t*>(malloc(N_ELEMS*sizeof(dtype)));
	auto array2_initial_val = static_cast<uint16_t*>(malloc(N_ELEMS*sizeof(dtype)));
	auto array1 = static_cast<uint16_t*>(pim_malloc(N_ELEMS*sizeof(dtype), 0));
	if (!array1) {
		printf("NOTE: Not enough space left in current mat \n");
		next_mat++;
		return false;
	}

	auto array2 = static_cast<uint16_t*>(pim_malloc(N_ELEMS*sizeof(dtype), 0));
	if (!array2) {
		printf("NOTE: Not enough space left in current mat \n");
		pim_free(array1); // TODO !!
		next_mat++;
		return false;
	}
	std::printf("Ran pim_malloc and got ptr array1=%p, array2=%p\n", array1, array2);

	cout << "AND..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	rowand(array1, array2, array1, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, bit_and<uint16_t>{});

	cout << "ADD..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	rowadd(array1, array2, array1, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, plus<uint16_t>{});

	cout << "SUB..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	rowsub(array1, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, minus<uint16_t>{});

	cout << "MUL..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	rowmult(array1, array2, array1, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, multiplies<uint16_t>{});

	cout << "DIV..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	rowdiv(array1, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, divides<uint16_t>{});

	cout << "MIN..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	auto min_op = [](auto a, auto b) { return a < b ? a : b; };
	rowmin(array1, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, min_op);

	cout << "MAX..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	auto max_op = [](auto a, auto b) { return a > b ? a : b; };
	rowmax(array1, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, max_op);

	cout << "Passed " << nr_correct << "/" << N_ROWOPS << endl;
	return true;
}


size_t fuzzy_testing()
{
	size_t nr_correct = 0;
	auto array1_initial_val = static_cast<uint16_t*>(malloc(N_ELEMS*sizeof(dtype)));
	auto array2_initial_val = static_cast<uint16_t*>(malloc(N_ELEMS*sizeof(dtype)));
	auto array1 = static_cast<uint16_t*>(pim_malloc(N_ELEMS*sizeof(dtype), 0));
	if (!array1) {
		printf("NOTE: Not enough space left in current mat \n");
		next_mat++;
		return 0;
	}

	auto array2 = static_cast<uint16_t*>(pim_malloc(N_ELEMS*sizeof(dtype), 0));
	if (!array2) {
		printf("NOTE: Not enough space left in current mat \n");
		pim_free(array1); // TODO !!
		next_mat++;
		return 0;
	}
	std::printf("Ran pim_malloc and got ptr array1=%p, array2=%p\n", array1, array2);

	cout << "AND..." << endl;
	init_data_fuzzy(array1, array2, array1_initial_val, array2_initial_val);
	rowand(array1, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, bit_and<uint16_t>{});

	cout << "ADD..." << endl;
	init_data_fuzzy(array1, array2, array1_initial_val, array2_initial_val);
	rowadd(array1, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, plus<uint16_t>{});

	cout << "SUB..." << endl;
	init_data_fuzzy(array1, array2, array1_initial_val, array2_initial_val);
	rowsub(array1, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, minus<uint16_t>{});

	cout << "MUL..." << endl;
	init_data_fuzzy(array1, array2, array1_initial_val, array2_initial_val);
	rowmult(array1, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, multiplies<uint16_t>{});

	cout << "DIV..." << endl;
	init_data_fuzzy(array1, array2, array1_initial_val, array2_initial_val);
	rowdiv(array1, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, divides<uint16_t>{});

	cout << "MIN..." << endl;
	init_data_fuzzy(array1, array2, array1_initial_val, array2_initial_val);
	auto min_op = [](auto a, auto b) { return a < b ? a : b; };
	rowmin(array1, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, min_op);

	cout << "MAX..." << endl;
	init_data_fuzzy(array1, array2, array1_initial_val, array2_initial_val);
	auto max_op = [](auto a, auto b) { return a > b ? a : b; };
	rowmax(array1, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, max_op);

	return nr_correct;
}

int main()
{
	cout << "Running pim_test_pimmalloc.cpp" << endl;
	while(next_mat < NR_MATS && !test_every_rowop()) ;

	// also try with random data
	int nr_fuzzy_tests = 5;
	size_t nr_correct = 0;
	for (int i=0; i<nr_fuzzy_tests && next_mat < NR_MATS; ++i) {
		auto c =  fuzzy_testing();
		if (c==0) {
			next_mat++;
			i--; // try again
		}
		nr_correct += c;
	}
	cout << "Passed " << nr_correct << "/" << nr_fuzzy_tests*N_ROWOPS << endl;

	return 0;
}
