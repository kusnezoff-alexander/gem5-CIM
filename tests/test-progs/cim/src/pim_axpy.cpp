#ifndef N_ELEMS
#define N_ELEMS 3000
#endif

#ifndef N_RUNS
#define N_RUNS 10
#endif

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

size_t next_subarray = 0;

#if BITWIDTH == 8
using dtype = int8_t;
#elif BITWIDTH == 16
using dtype = int16_t;
#elif BITWIDTH == 32
using dtype = int32_t;
#else
using dtype = int16_t;
#endif

template<std::integral T>
T* pim_alloc_safe(size_t size_bytes, size_t& next_subarray) {
    T* ptr = nullptr;

    do {
        ptr = static_cast<T*>(pim_malloc(size_bytes, next_subarray));
        if (ptr != nullptr) break;
        next_subarray++;
    } while (next_subarray < NR_SUBARRAYS);

    if (ptr == nullptr) {
        cerr << "ERROR: not enough PIM space for "
             << size_bytes << " bytes\n";
        exit(1);
    }

    return ptr;
}

template<std::integral T>
void init_data(T* x, T* y, T* x_initial, T* y_initial)
{
    random_device rd;
    mt19937 gen(rd());

    using dist_type = std::conditional_t<std::is_signed_v<T>,
                                         std::uniform_int_distribution<int>,
                                         std::uniform_int_distribution<T>>;

    dist_type dist(std::is_signed_v<T> ? -1000 : 0, 1000);

    for(size_t i = 0; i < N_ELEMS; ++i) {
        x[i] = dist(gen);
        y[i] = dist(gen);

        x_initial[i] = x[i];
        y_initial[i] = y[i];
    }
}

template<std::integral T>
bool check_axpy_result(T* y, T* y_initial, T* x_initial, dtype alpha)
{
    bool is_correct = true;
    for(size_t i = 0; i < N_ELEMS; ++i) {
        T expected = static_cast<T>(alpha) * x_initial[i] + y_initial[i];
        if(y[i] != expected) {
            std::printf("WRONG: y[%zu]=%d but should be %d (alpha=%d, x=%d, y_initial=%d)\n",
                    i, y[i], expected, alpha, x_initial[i], y_initial[i]);
            is_correct = false;
        }
    }
    return is_correct;
}

bool test_axpy(int run_id)
{
    dtype alpha = 3;

    auto x_initial = static_cast<dtype*>(malloc(N_ELEMS * sizeof(dtype)));
    auto y_initial = static_cast<dtype*>(malloc(N_ELEMS * sizeof(dtype)));

    auto x = pim_alloc_safe<dtype>(N_ELEMS * sizeof(dtype), next_subarray);
    auto y = pim_alloc_safe<dtype>(N_ELEMS * sizeof(dtype), next_subarray);
    auto scalar_alpha = pim_alloc_safe<dtype>(N_ELEMS * sizeof(dtype), next_subarray);

    printf("Run %d: PIM memory allocated: x=%p, y=%p, scalar_alpha=%p\n",
           run_id, x, y, scalar_alpha);

    rowtrsp_init(x, N_ELEMS, sizeof(dtype));
    rowtrsp_init(y, N_ELEMS, sizeof(dtype));
    rowtrsp_init(scalar_alpha, N_ELEMS, sizeof(dtype));

    init_data(x, y, x_initial, y_initial);

    for(size_t i = 0; i < N_ELEMS; ++i) {
        scalar_alpha[i] = alpha;
    }

    printf("Run %d: Data initialized. Running AXPY: y = %d * x + y\n", run_id, alpha);

    m5_reset_stats(0, 0);

    rowmult(x, x, scalar_alpha, N_ELEMS, sizeof(dtype) * 8);

    rowadd(y, y, x, N_ELEMS, sizeof(dtype) * 8);

    m5_dump_stats(0, 0);

    bool correct = check_axpy_result(y, y_initial, x_initial, alpha);

    if (correct) {
        printf("Run %d: AXPY test PASSED!\n", run_id);
    } else {
        printf("Run %d: AXPY test FAILED!\n", run_id);
    }

    free(x_initial);
    free(y_initial);

    return correct;
}

int main()
{
    printf("Starting AXPY test with %zu elements, %d runs\n", (size_t)N_ELEMS, N_RUNS);

    int passed_runs = 0;
    for (int run = 0; run < N_RUNS; run++) {
        printf("\n=== Run %d/%d ===\n", run + 1, N_RUNS);

        bool result = false;
        while(next_subarray < NR_SUBARRAYS && !result) {
            result = test_axpy(run + 1);
            if (!result) {
                next_subarray++;
            }
        }

        if (result) {
            passed_runs++;
            printf("\nAXPY test PASSED!\n");
        } else {
            printf("\nAXPY test FAILED - not enough PIM space\n");
        }
    }

    printf("\n=== Summary: %d/%d runs passed ===\n", passed_runs, N_RUNS);
    if (passed_runs == N_RUNS) {
        return 0;
    } else {
        return 1;
    }
}
