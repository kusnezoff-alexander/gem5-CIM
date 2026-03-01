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
size_t next_mat = 0;

using dtype = int16_t;

template<std::integral T>
T* pim_alloc_safe(size_t size_bytes, size_t& next_mat) {
    T* ptr = nullptr;

    do {
        ptr = static_cast<T*>(pim_malloc(size_bytes, next_mat));
        if (ptr != nullptr) break;
        next_mat++;
    } while (next_mat < NR_SUBARRAYS);

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
        auto expected = static_cast<T>(alpha) * x_initial[i] + y_initial[i];
        if(y[i] != expected) {
            std::printf("WRONG: y[%zu]=%d but should be %d (alpha=%d, x=%d, y_initial=%d)\n",
                    i, y[i], expected, alpha, x_initial[i], y_initial[i]);
            is_correct = false;
        }
    }
    return is_correct;
}

bool test_axpy()
{
    dtype alpha = 3;

    auto x_initial = static_cast<dtype*>(malloc(N_ELEMS * sizeof(dtype)));
    auto y_initial = static_cast<dtype*>(malloc(N_ELEMS * sizeof(dtype)));

    auto x = pim_alloc_safe<dtype>(N_ELEMS * sizeof(dtype), next_mat);
    auto y = pim_alloc_safe<dtype>(N_ELEMS * sizeof(dtype), next_mat);
    auto temp = pim_alloc_safe<dtype>(N_ELEMS * sizeof(dtype), next_mat);
    auto scalar_alpha = pim_alloc_safe<dtype>(N_ELEMS * sizeof(dtype), next_mat);

    printf("PIM memory allocated: x=%p, y=%p, temp=%p, scalar_alpha=%p\n",
           x, y, temp, scalar_alpha);

    rowtrsp_init(x, N_ELEMS, sizeof(dtype));
    rowtrsp_init(y, N_ELEMS, sizeof(dtype));
    rowtrsp_init(temp, N_ELEMS, sizeof(dtype));
    rowtrsp_init(scalar_alpha, N_ELEMS, sizeof(dtype));

    init_data(x, y, x_initial, y_initial);

    for(size_t i = 0; i < N_ELEMS; ++i) {
        scalar_alpha[i] = alpha;
    }

    printf("Data initialized. Running AXPY: y = %d * x + y\n", alpha);

    m5_reset_stats(0, 0);

    m5_work_begin(1, 0);
    rowmult(temp, x, scalar_alpha, N_ELEMS, sizeof(dtype) * 8);
    m5_work_end(1, 0);

    m5_work_begin(2, 0);
    rowadd(y, y, temp, N_ELEMS, sizeof(dtype) * 8);
    m5_work_end(2, 0);

    m5_dump_reset_stats(0, 0);

    bool correct = check_axpy_result(y, y_initial, x_initial, alpha);

    if (correct) {
        printf("AXPY test PASSED!\n");
    } else {
        printf("AXPY test FAILED!\n");
    }

    free(x_initial);
    free(y_initial);

    return correct;
}

int main()
{
    printf("Starting AXPY test with %zu elements\n", N_ELEMS);

    bool result = false;
    while(next_mat < NR_SUBARRAYS && !result) {
        result = test_axpy();
        if (!result) {
            next_mat++;
        }
    }

    if (result) {
        printf("\nAll AXPY tests PASSED!\n");
        return 0;
    } else {
        printf("\nAXPY tests FAILED - not enough PIM space\n");
        return 1;
    }
}
