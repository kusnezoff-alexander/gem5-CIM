#include "pim_core.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <sys/types.h>
#include <type_traits>
#include <functional>
#include <iostream>
#include <string>
#include <gem5/m5ops.h>

using namespace pim_core;
using namespace std;

#ifndef N_ELEMS
#define N_ELEMS 3000
#endif

#ifndef BITWIDTH
#define BITWIDTH 16
#endif

#if BITWIDTH == 8
using dtype = int8_t;
#elif BITWIDTH == 16
using dtype = int16_t;
#elif BITWIDTH == 32
using dtype = int32_t;
#else
using dtype = int16_t;
#endif

size_t next_mat = 0;

template<typename T>
typename enable_if<is_integral<T>::value, T*>::type
pim_alloc_safe(size_t size_bytes, size_t& next_mat) {
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

template<typename T>
typename enable_if<is_integral<T>::value>::type
init_data(T* array1, T* array2, T* array1_initial_val, T* array2_initial_val)
{
    auto sign = -1;
    for(size_t i=0; i<N_ELEMS; ++i) {
        array1[i] = sign*i;
        array2[i] = 0x111;
        array1_initial_val[i] = array1[i];
        array2_initial_val[i] = array2[i];
        sign *= -1;
    }
}

template<typename T, typename Op>
typename enable_if<is_integral<T>::value, bool>::type
check_result(T* res, T* array1_initial_val, T* array2_initial_val, Op op)
{
    bool is_correct = true;
    for(size_t i=0; i<N_ELEMS; ++i) {
        auto res_should = op(array1_initial_val[i], array2_initial_val[i]);
        if(res[i] != res_should) {
            std::printf("WRONG: array1[%zu]=%d but should be %d\n", i, res[i], res_should);
            is_correct = false;
        }
    }
    return is_correct;
}

template<typename T, typename Op>
typename enable_if<is_integral<T>::value, bool>::type
check_result(T* res, T* array1_initial_val, T* array2_initial_val, T* mask_initial_val, Op op)
{
    bool is_correct = true;
    for(size_t i=0; i<N_ELEMS; ++i) {
        auto res_should = op(mask_initial_val[i], array1_initial_val[i], array2_initial_val[i]);
        if(res[i] != res_should) {
            std::printf("WRONG: array1[%zu]=%d but should be %d\n", i, res[i], res_should);
            is_correct = false;
        }
    }
    return is_correct;
}

const char* op_names[] = {
    "rowand", "rowadd", "rowsub", "rowmult", // "rowdiv",
    "rowmin", "rowmax", "rowequal", "rowgreater", "rowgreater_equal",
    "rowif_else", "rowabs", "bitcount"
};

int main(int argc, char* argv[])
{
    bool run_checks = false;
    int op_id = 1;

    if (argc >= 2 && string(argv[1]) == "--check") {
        run_checks = true;
        op_id = 1;
    } else if (argc >= 2) {
        op_id = atoi(argv[1]);
    }

    if (argc >= 3 && string(argv[2]) == "--check") {
        run_checks = true;
    }

    if (argc > 3) {
        cerr << "Usage: " << argv[0] << " [op_id] [--check]" << endl;
        cerr << "op_id: 1=rowand, 2=rowadd, 3=rowsub, 4=rowmult, 5=rowdiv, ";
        cerr << "6=rowmin, 7=rowmax, 8=rowequal, 9=rowgreater, 10=rowgreater_equal, ";
        cerr << "11=rowif_else, 12=rowabs, 13=bitcount" << endl;
        cerr << "--check: run correctness verification" << endl;
        return 1;
    }

    if (op_id < 1 || op_id > 13) {
        cerr << "Invalid op_id: " << op_id << endl;
        return 1;
    }

    cout << "CIM: Running " << op_names[op_id-1] << " (op_id=" << op_id << ", n_elems=" << N_ELEMS << ")" << endl;

    auto array1_initial_val = static_cast<dtype*>(malloc(N_ELEMS*sizeof(dtype)));
    auto array2_initial_val = static_cast<dtype*>(malloc(N_ELEMS*sizeof(dtype)));
    auto array1 = pim_alloc_safe<dtype>(N_ELEMS*sizeof(dtype), next_mat);
    auto array2 = pim_alloc_safe<dtype>(N_ELEMS*sizeof(dtype), next_mat);

    std::printf("Ran pim_alloc_safe and got ptr array1=%p, array2=%p\n", array1, array2);

    init_data(array1, array2, array1_initial_val, array2_initial_val);

    // m5_reset_stats(0, 0);

    bool passed = false;

    switch(op_id) {
        case 1: {
            m5_reset_stats(0, 0);
            m5_work_begin(1, 0);
            rowand(array1, array2, array1, N_ELEMS, sizeof(dtype) * 8);
            m5_work_end(1, 0);
			m5_dump_stats(0, 0);
            if (run_checks) { passed = check_result(array1, array1_initial_val, array2_initial_val, bit_and<dtype>{}); }
            break;
        }
        case 2: {
            m5_reset_stats(0, 0);
            m5_work_begin(2, 0);
            rowadd(array1, array2, array1, N_ELEMS, sizeof(dtype) * 8);
            m5_work_end(2, 0);
			m5_dump_stats(0, 0);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, plus<dtype>{});
            break;
        }
        case 3: {
            m5_reset_stats(0, 0);
            m5_work_begin(3, 0);
            rowsub(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
            m5_work_end(3, 0);
			m5_dump_stats(0, 0);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, minus<dtype>{});
            break;
        }
        case 4: {
            m5_reset_stats(0, 0);
            m5_work_begin(4, 0);
            rowmult(array1, array2, array1, N_ELEMS, sizeof(dtype) * 8);
            m5_work_end(4, 0);
			m5_dump_stats(0, 0);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, multiplies<dtype>{});
            break;
        }
        case 5: {
            m5_reset_stats(0, 0);
            m5_work_begin(5, 0);
            rowdiv(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
            m5_work_end(5, 0);
			m5_dump_stats(0, 0);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, divides<dtype>{});
            break;
        }
        case 6: {
            auto min_op = [](dtype a, dtype b) { return a < b ? a : b; };
            m5_reset_stats(0, 0);
            m5_work_begin(6, 0);
            rowmin(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
            m5_work_end(6, 0);
			m5_dump_stats(0, 0);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, min_op);
            break;
        }
        case 7: {
            auto max_op = [](dtype a, dtype b) { return a > b ? a : b; };
            m5_reset_stats(0, 0);
            m5_work_begin(7, 0);
            rowmax(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
            m5_work_end(7, 0);
			m5_dump_stats(0, 0);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, max_op);
            break;
        }
        case 8: {
            auto row_equal = [](dtype a, dtype b) -> dtype {
                return (a == b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
            };
            m5_reset_stats(0, 0);
            m5_work_begin(8, 0);
            rowequal(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
            m5_work_end(8, 0);
			m5_dump_stats(0, 0);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, row_equal);
            break;
        }
        case 9: {
            auto row_greater = [](dtype a, dtype b) -> dtype {
                return (a > b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
            };
            m5_reset_stats(0, 0);
            m5_work_begin(9, 0);
            rowgreater(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
            m5_work_end(9, 0);
			m5_dump_stats(0, 0);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, row_greater);
            break;
        }
        case 10: {
            auto row_greater_equal = [](dtype a, dtype b) -> dtype {
                return (a >= b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
            };
            m5_reset_stats(0, 0);
            m5_work_begin(10, 0);
            rowgreater_equal(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
            m5_work_end(10, 0);
			m5_dump_stats(0, 0);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, row_greater_equal);
            break;
        }
        case 11: {
            auto mask_initial_val = new dtype[N_ELEMS];
            for (size_t i = 0; i < N_ELEMS; ++i) {
                mask_initial_val[i] = array1_initial_val[i];
            }
            auto row_ifelse = [](dtype mask_val, dtype a, dtype b) -> dtype {
                return (mask_val != 0) ? a : b;
            };
            m5_reset_stats(0, 0);
            m5_work_begin(11, 0);
            rowif_else(array1, array1, array2, array1_initial_val, N_ELEMS, sizeof(dtype) * 8);
            m5_work_end(11, 0);
			m5_dump_stats(0, 0);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, mask_initial_val, row_ifelse);
            delete[] mask_initial_val;
            break;
        }
        case 12: {
            auto row_abs = [](dtype a, dtype) -> dtype {
                return (a < 0) ? static_cast<dtype>(-a) : a;
            };
            m5_reset_stats(0, 0);
            m5_work_begin(12, 0);
            rowabs(array1, array1, N_ELEMS, sizeof(dtype) * 8);
            m5_work_end(12, 0);
			m5_dump_stats(0, 0);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, row_abs);
            break;
        }
        case 13: {
            auto row_bitcount = [](dtype a, dtype) -> dtype {
                unsigned int count = 0;
                unsigned int val = static_cast<unsigned int>(a);
                while (val) {
                    count += val & 1;
                    val >>= 1;
                }
                return static_cast<dtype>(count);
            };
            m5_reset_stats(0, 0);
            m5_work_begin(13, 0);
            rowbitcount(array1, array1, array2, N_ELEMS, sizeof(dtype) * 8);
            m5_work_end(13, 0);
			m5_dump_stats(0, 0);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, row_bitcount);
            break;
        }
    }

    if (run_checks) cout << "Result: " << (passed ? "PASSED" : "FAILED") << endl;
    return passed ? 0 : 1;
}
