#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <gem5/m5ops.h>

#ifndef N_ELEMS
#define N_ELEMS 3000
#endif

#ifndef N_RUNS
#define N_RUNS 10
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

void init_data(dtype* x, dtype* y, dtype* x_initial, dtype* y_initial)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(-1000, 1000);

    for(size_t i = 0; i < N_ELEMS; ++i) {
        x[i] = dist(gen);
        y[i] = dist(gen);
        x_initial[i] = x[i];
        y_initial[i] = y[i];
    }
}

bool check_axpy_result(dtype* y, dtype* y_initial, dtype* x_initial, dtype alpha)
{
    bool is_correct = true;
    for(size_t i = 0; i < N_ELEMS; ++i) {
        dtype expected = static_cast<dtype>(alpha) * x_initial[i] + y_initial[i];
        if(y[i] != expected) {
            std::printf("WRONG: y[%zu]=%d but should be %d (alpha=%d, x=%d, y_initial=%d)\n",
                    i, y[i], expected, alpha, x_initial[i], y_initial[i]);
            is_correct = false;
        }
    }
    return is_correct;
}

void axpy_loop(dtype* y, const dtype* x, dtype alpha, size_t n)
{
    for(size_t i = 0; i < n; ++i) {
        y[i] = y[i] + alpha * x[i];
    }
}

bool test_axpy()
{
    dtype alpha = 3;

    auto x_initial = static_cast<dtype*>(malloc(N_ELEMS * sizeof(dtype)));
    auto y_initial = static_cast<dtype*>(malloc(N_ELEMS * sizeof(dtype)));

    auto x = static_cast<dtype*>(malloc(N_ELEMS * sizeof(dtype)));
    auto y = static_cast<dtype*>(malloc(N_ELEMS * sizeof(dtype)));

    printf("CPU memory allocated: x=%p, y=%p\n", (void*)x, (void*)y);

    init_data(x, y, x_initial, y_initial);

    printf("Data initialized. Running AXPY: y = %d * x + y\n", alpha);

    m5_reset_stats(0, 0);

    for (int r = 0; r < N_RUNS; ++r) {
        for (size_t i = 0; i < N_ELEMS; ++i) {
            y[i] = y_initial[i];
        }
        axpy_loop(y, x, alpha, N_ELEMS);
    }

    m5_dump_stats(0, 0);

    bool correct = check_axpy_result(y, y_initial, x_initial, alpha);

    if (correct) {
        printf("AXPY test PASSED!\n");
    } else {
        printf("AXPY test FAILED!\n");
    }

    free(x_initial);
    free(y_initial);
    free(x);
    free(y);

    return correct;
}

int main()
{
    printf("Starting CPU AXPY test with %d elements, %d runs\n", N_ELEMS, N_RUNS);

    bool result = test_axpy();

    if (result) {
        printf("\nAll AXPY tests PASSED!\n");
        return 0;
    } else {
        printf("\nAXPY tests FAILED\n");
        return 1;
    }
}
