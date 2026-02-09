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

const size_t N_ELEMS = 3000;
const size_t N_ROWOPS = 12;
size_t next_mat = 0;

template<std::integral T>
void init_data(T*& array1, T*& array2, T*& array1_initial_val, T*& array2_initial_val)
{
	// 1. Write data
	auto sign = -1;
	for(size_t i=0; i<N_ELEMS; ++i) {
		array1[i] = sign*i;
		array2[i] = 0x111;

		array1_initial_val[i] = array1[i];
		array2_initial_val[i] = array2[i];

		sign *= -1;
	}
}

template<std::integral T>
void init_data_fuzzy(T*& array1, T*& array2,
                     T*& array1_initial_val, T*& array2_initial_val,
                     bool is_div = false)
{
    random_device rd;
    mt19937 gen(rd());

    using dist_type = std::conditional_t<std::is_signed_v<T>,
                                         std::uniform_int_distribution<int>,
                                         std::uniform_int_distribution<T>>;

    dist_type dist(std::is_signed_v<T> ? std::numeric_limits<T>::min() : 0,
                   std::numeric_limits<T>::max());

    for(size_t i = 0; i < N_ELEMS; ++i) {
        array1[i] = dist(gen);
        array2[i] = dist(gen);

        if (is_div && array2[i] == 0)
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

using dtype = int16_t;
bool test_every_rowop()
{
	size_t nr_correct = 0;
	auto array1_initial_val = static_cast<dtype*>(malloc(N_ELEMS*sizeof(dtype)));
	auto array2_initial_val = static_cast<dtype*>(malloc(N_ELEMS*sizeof(dtype)));
	auto array1 = static_cast<dtype*>(pim_malloc(N_ELEMS*sizeof(dtype), next_mat));
	if (!array1) {
		printf("NOTE: Not enough space left in current mat \n");
		next_mat++;
		return false;
	}

	auto array2 = static_cast<dtype*>(pim_malloc(N_ELEMS*sizeof(dtype), next_mat));
	if (!array2) {
		printf("NOTE: Not enough space left in current mat \n");
		pim_free(array1); // TODO !!
		next_mat++;
		return false;
	}
	std::printf("Ran pim_malloc and got ptr array1=%p, array2=%p\n", array1, array2);

	cout << "AND..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	rowand(array1, array2, array1, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, bit_and<dtype>{});

	cout << "ADD..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	rowadd(array1, array2, array1, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, plus<dtype>{});

	cout << "SUB..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	rowsub(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, minus<dtype>{});

	cout << "MUL..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	rowmult(array1, array2, array1, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, multiplies<dtype>{});

	cout << "DIV..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	rowdiv(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, divides<dtype>{});

	cout << "MIN..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	auto min_op = [](auto a, auto b) { return a < b ? a : b; };
	rowmin(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, min_op);

	cout << "MAX..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	auto max_op = [](auto a, auto b) { return a > b ? a : b; };
	rowmax(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, max_op);

	cout << "ROWEQUAL..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	auto row_equal = [](dtype a, dtype b) -> dtype {
		return (a == b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
	};
	rowequal(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, row_equal);

	cout << "ROWGREATER..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	auto row_greater = [](dtype a, dtype b) -> dtype {
		return (a > b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
	};
	rowgreater(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, row_greater);

	cout << "ROWGREATEREQUAL..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	auto row_greater_equal = [](dtype a, dtype b) -> dtype {
		return (a >= b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
	};
	rowgreater_equal(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, row_greater_equal);

	cout << "ROWIFELSE..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	auto row_ifelse = [](dtype a, dtype b) -> dtype {
		return (a != 0) ? b : static_cast<dtype>(0);
	};
	rowif_else(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, row_ifelse);

	cout << "ROWABS..." << endl;
	init_data(array1, array2, array1_initial_val, array2_initial_val);
	auto row_abs = [](dtype a, dtype _) -> dtype {
		return (a < 0) ? static_cast<dtype>(-a) : a;
	};
	rowabs(array1, array1, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, row_abs);

	// rowtrsp_init ?
	cout << "Passed " << nr_correct << "/" << N_ROWOPS << endl;
	return true;
}


size_t fuzzy_testing()
{
	size_t nr_correct = 0;
	auto array1_initial_val = static_cast<dtype*>(malloc(N_ELEMS*sizeof(dtype)));
	auto array2_initial_val = static_cast<dtype*>(malloc(N_ELEMS*sizeof(dtype)));
	auto array1 = static_cast<dtype*>(pim_malloc(N_ELEMS*sizeof(dtype), next_mat));
	if (!array1) {
		printf("NOTE: Not enough space left in current mat \n");
		next_mat++;
		return 0;
	}

	auto array2 = static_cast<dtype*>(pim_malloc(N_ELEMS*sizeof(dtype), next_mat));
	if (!array2) {
		printf("NOTE: Not enough space left in current mat \n");
		// give up PIM space to other applications
		pim_free(array1);
		next_mat++;
		return 0;
	}

	auto array_res = static_cast<dtype*>(pim_malloc(N_ELEMS*sizeof(dtype), next_mat));
	if (!array_res) {
		printf("NOTE: Not enough space left in current mat \n");
		pim_free(array1);
		pim_free(array2);
		next_mat++;
		return 0;
	}

	std::printf("Ran pim_malloc and got ptr array1=%p, array2=%p\n", array1, array2);
	init_data_fuzzy(array1, array2, array1_initial_val, array2_initial_val);

	cout << "AND..." << endl;
	rowand(array_res, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, bit_and<dtype>{});

	cout << "ADD..." << endl;
	rowadd(array_res, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, plus<dtype>{});

	cout << "SUB..." << endl;
	rowsub(array_res, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, minus<dtype>{});

	cout << "MUL..." << endl;
	rowmult(array_res, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, multiplies<dtype>{});

	cout << "DIV..." << endl;
	rowdiv(array_res, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, divides<dtype>{});

	cout << "MIN..." << endl;
	auto min_op = [](auto a, auto b) { return a < b ? a : b; };
	rowmin(array_res, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, min_op);

	cout << "MAX..." << endl;
	auto max_op = [](auto a, auto b) { return a > b ? a : b; };
	rowmax(array_res, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, max_op);

	cout << "ROWEQUAL..." << endl;
	auto row_equal = [](dtype a, dtype b) -> dtype {
		return (a == b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
	};
	rowequal(array_res, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, row_equal);

	cout << "ROWGREATER..." << endl;
	auto row_greater = [](dtype a, dtype b) -> dtype {
		return (a > b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
	};
	rowgreater(array_res, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, row_greater);

	cout << "ROWGREATEREQUAL..." << endl;
	auto row_greater_equal = [](dtype a, dtype b) -> dtype {
		return (a >= b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
	};
	rowgreater_equal(array_res, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, row_greater_equal);

	cout << "ROWIFELSE..." << endl;
	auto row_ifelse = [](dtype a, dtype b) -> dtype {
		return (a != 0) ? b : static_cast<dtype>(0);
	};
	rowif_else(array_res, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, row_ifelse);

	cout << "ROWABS..." << endl;
	auto row_abs = [](dtype a, dtype _) -> dtype {
		return (a < 0) ? static_cast<dtype>(-a) : a;
	};
	rowabs(array_res, array1, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, row_abs);

	return nr_correct;
}

int main()
{
	cout << "Running pim_test_primitives.cpp" << endl;
	while(next_mat < NR_MATS && !test_every_rowop()) ;

	// also try with random data
	int nr_fuzzy_tests = 1;
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
