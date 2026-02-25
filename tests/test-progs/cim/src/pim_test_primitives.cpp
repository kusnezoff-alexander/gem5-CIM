#include "pim_core.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <sys/types.h>
#include <concepts>
#include <functional>
#include <iostream>
#include <gem5/m5ops.h>

using namespace pim_core;
using namespace std;

const size_t N_ELEMS = 3000;
const size_t N_ROWOPS = 12;
size_t next_mat = 0;

template<typename T>
T* pim_alloc_safe(size_t size_bytes, size_t& next_mat) {
    T* ptr = nullptr;

    do {
        ptr = static_cast<T*>(pim_malloc(size_bytes, next_mat));
        if (ptr != nullptr) break;
        next_mat++;
    } while (next_mat < NR_MATS);

    if (ptr == nullptr) {
        cerr << "ERROR: not enough PIM space for "
             << size_bytes << " bytes\n";
        exit(1);
    }

    return ptr;
}

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
 * @returns Whether check was correct (2-operand version)
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
			is_correct = false;
		}
	}
	return is_correct;
}

/**
 * @returns Whether check was correct (3-operand version for IF_ELSE)
 */
template<std::integral T, typename Op>
bool check_result(T* res, T* array1_initial_val, T* array2_initial_val, T* mask_initial_val, Op op)
{
	bool is_correct = true;
	for(size_t i=0; i<N_ELEMS; ++i) {
		auto res_should = op(mask_initial_val[i], array1_initial_val[i], array2_initial_val[i]);
		if(res[i] != res_should) {
			std::printf("WRONG: array1[%zu]=%d but should be %d (mask=%d, src1=%d, src2=%d)\n",
					i, res[i], res_should,
					mask_initial_val[i], array1_initial_val[i], array2_initial_val[i]);
			is_correct = false;
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
	rowtrsp_init(array1, N_ELEMS, sizeof(dtype));
	rowtrsp_init(array2, N_ELEMS, sizeof(dtype));

	m5_reset_stats(0, 0);

	init_data(array1, array2, array1_initial_val, array2_initial_val);

	m5_dump_reset_stats(0, 0);

	m5_work_begin(1, 0);
	rowand(array1, array2, array1, N_ELEMS, sizeof(dtype) * 8);
	m5_work_end(1, 0);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, bit_and<dtype>{});

	init_data(array1, array2, array1_initial_val, array2_initial_val);

	m5_dump_reset_stats(0, 0);

	m5_work_begin(2, 0);
	rowadd(array1, array2, array1, N_ELEMS, sizeof(dtype) * 8);
	m5_work_end(2, 0);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, plus<dtype>{});

	init_data(array1, array2, array1_initial_val, array2_initial_val);

	m5_dump_reset_stats(0, 0);

	m5_work_begin(3, 0);
	rowsub(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	m5_work_end(3, 0);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, minus<dtype>{});

	init_data(array1, array2, array1_initial_val, array2_initial_val);

	m5_dump_reset_stats(0, 0);

	m5_work_begin(4, 0);
	rowmult(array1, array2, array1, N_ELEMS, sizeof(dtype) * 8);
	m5_work_end(4, 0);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, multiplies<dtype>{});

	init_data(array1, array2, array1_initial_val, array2_initial_val);

	auto min_op = [](auto a, auto b) { return a < b ? a : b; };
	m5_dump_reset_stats(0, 0);

	m5_work_begin(5, 0);
	rowmin(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	m5_work_end(5, 0);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, min_op);

	init_data(array1, array2, array1_initial_val, array2_initial_val);

	auto max_op = [](auto a, auto b) { return a > b ? a : b; };
	m5_dump_reset_stats(0, 0);

	m5_work_begin(6, 0);
	rowmax(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	m5_work_end(6, 0);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, max_op);

	init_data(array1, array2, array1_initial_val, array2_initial_val);

	auto row_equal = [](dtype a, dtype b) -> dtype {
		return (a == b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
	};
	m5_dump_reset_stats(0, 0);

	m5_work_begin(7, 0);
	rowequal(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	m5_work_end(7, 0);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, row_equal);

	init_data(array1, array2, array1_initial_val, array2_initial_val);

	auto row_greater = [](dtype a, dtype b) -> dtype {
		return (a > b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
	};
	m5_dump_reset_stats(0, 0);

	m5_work_begin(8, 0);
	rowgreater(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	m5_work_end(8, 0);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, row_greater);

	init_data(array1, array2, array1_initial_val, array2_initial_val);

	auto row_greater_equal = [](dtype a, dtype b) -> dtype {
		return (a >= b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
	};
	m5_dump_reset_stats(0, 0);

	m5_work_begin(9, 0);
	rowgreater_equal(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	m5_work_end(9, 0);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, row_greater_equal);

	init_data(array1, array2, array1_initial_val, array2_initial_val);

	auto mask = pim_alloc_safe<dtype>(N_ELEMS * sizeof(dtype), next_mat);
	auto mask_initial_val = new dtype[N_ELEMS];
	for (size_t i = 0; i < N_ELEMS; ++i) {
		mask[i] = mask_initial_val[i] = array1_initial_val[i];
	}
	auto row_ifelse = [](dtype mask_val, dtype a, dtype b) -> dtype {
		return (mask_val != 0) ? a : b;
	};
	m5_dump_reset_stats(0, 0);

	m5_work_begin(10, 0);
	rowif_else(array1, array1, array2, mask, N_ELEMS, sizeof(dtype) * 8);
	m5_work_end(10, 0);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, mask_initial_val, row_ifelse);

	init_data(array1, array2, array1_initial_val, array2_initial_val);

	auto row_abs = [](dtype a, dtype _) -> dtype {
		return (a < 0) ? static_cast<dtype>(-a) : a;
	};
	m5_dump_reset_stats(0, 0);

	m5_work_begin(11, 0);
	rowabs(array1, array1, N_ELEMS, sizeof(dtype) * 8);
	m5_work_end(11, 0);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, row_abs);

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

	m5_reset_stats(0, 0);

	m5_dump_reset_stats(0, 0);

	m5_work_begin(1, 0);
	rowand(array_res, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	m5_work_end(1, 0);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, bit_and<dtype>{});

	m5_dump_reset_stats(0, 0);

	m5_work_begin(2, 0);
	rowadd(array_res, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	m5_work_end(2, 0);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, plus<dtype>{});

	m5_dump_reset_stats(0, 0);

	m5_work_begin(3, 0);
	rowsub(array_res, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	m5_work_end(3, 0);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, minus<dtype>{});

	m5_dump_reset_stats(0, 0);

	m5_work_begin(4, 0);
	rowmult(array_res, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	m5_work_end(4, 0);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, multiplies<dtype>{});

	auto min_op = [](auto a, auto b) { return a < b ? a : b; };
	m5_dump_reset_stats(0, 0);

	cout << "ROWMIN..." << endl;
	rowmin(array_res, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, min_op);

	auto max_op = [](auto a, auto b) { return a > b ? a : b; };
	m5_dump_reset_stats(0, 0);

	cout << "ROWMAX..." << endl;
	rowmax(array_res, array1, array2, N_ELEMS, sizeof(uint16_t) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, max_op);

	auto row_equal = [](dtype a, dtype b) -> dtype {
		return (a == b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
	};
	m5_dump_reset_stats(0, 0);

	cout << "ROWEQUAL..." << endl;
	rowequal(array_res, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, row_equal);

	auto row_greater = [](dtype a, dtype b) -> dtype {
		return (a > b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
	};
	m5_dump_reset_stats(0, 0);

	cout << "ROWGREATER..." << endl;
	rowgreater(array_res, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, row_greater);

	auto row_greater_equal = [](dtype a, dtype b) -> dtype {
		return (a >= b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
	};
	m5_dump_reset_stats(0, 0);

	cout << "ROWGREATEREQUAL..." << endl;
	rowgreater_equal(array_res, array1, array2, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, row_greater_equal);

	auto mask = pim_alloc_safe<dtype>(N_ELEMS * sizeof(dtype), next_mat);
	auto mask_initial_val = new dtype[N_ELEMS];
	for (size_t i = 0; i < N_ELEMS; ++i) {
		mask[i] = mask_initial_val[i] = array1_initial_val[i];
	}
	auto row_ifelse = [](dtype mask_val, dtype a, dtype b) -> dtype {
		return (mask_val != 0) ? a : b;
	};
	m5_dump_reset_stats(0, 0);

	cout << "ROWIFELSE..." << endl;
	rowif_else(array1, array1, array2, mask, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array1, array1_initial_val, array2_initial_val, mask_initial_val, row_ifelse);

	auto row_abs = [](dtype a, dtype _) -> dtype {
		return (a < 0) ? static_cast<dtype>(-a) : a;
	};
	m5_dump_reset_stats(0, 0);

	cout << "ROWABS..." << endl;
	rowabs(array_res, array1, N_ELEMS, sizeof(dtype) * 8);
	nr_correct += check_result(array_res, array1_initial_val, array2_initial_val, row_abs);

	return nr_correct;
}

int main()
{
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
