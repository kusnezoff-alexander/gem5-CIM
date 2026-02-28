#include <hip/hip_runtime.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>

#ifndef N_ELEMS
#define N_ELEMS 3000
#endif

#ifndef N_RUNS
#define N_RUNS 10
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

#define HIP_CHECK(cmd) \
    do { \
        hipError_t error = cmd; \
        if (error != hipSuccess) { \
            fprintf(stderr, "HIP error: '%s' (%d) at %s:%d\n", \
                    hipGetErrorString(error), error, __FILE__, __LINE__); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

__global__ void axpy_kernel(dtype* y, const dtype* x, dtype alpha, size_t n)
{
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;

    int32_t temp = static_cast<int32_t>(alpha) * static_cast<int32_t>(x[idx]);
    y[idx] = static_cast<dtype>(temp + static_cast<int32_t>(y[idx]));
}

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
        auto expected = static_cast<dtype>(alpha) * x_initial[i] + y_initial[i];
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

    auto x = static_cast<dtype*>(malloc(N_ELEMS * sizeof(dtype)));
    auto y = static_cast<dtype*>(malloc(N_ELEMS * sizeof(dtype)));

    dtype* d_x;
    dtype* d_y;

    HIP_CHECK(hipMalloc(&d_x, N_ELEMS * sizeof(dtype)));
    HIP_CHECK(hipMalloc(&d_y, N_ELEMS * sizeof(dtype)));

    printf("GPU memory allocated: d_x=%p, d_y=%p\n", (void*)d_x, (void*)d_y);

    init_data(x, y, x_initial, y_initial);

    HIP_CHECK(hipMemcpy(d_x, x, N_ELEMS * sizeof(dtype), hipMemcpyHostToDevice));
    HIP_CHECK(hipMemcpy(d_y, y, N_ELEMS * sizeof(dtype), hipMemcpyHostToDevice));

    printf("Data initialized. Running AXPY: y = %d * x + y\n", alpha);

    int blockSize = 256;
    int numBlocks = (N_ELEMS + blockSize - 1) / blockSize;

    for (int r = 0; r < N_RUNS; ++r) {
        hipLaunchKernelGGL(axpy_kernel, dim3(numBlocks), dim3(blockSize), 0, 0,
                           d_y, d_x, alpha, N_ELEMS);
    }

    HIP_CHECK(hipDeviceSynchronize());

    HIP_CHECK(hipMemcpy(y, d_y, N_ELEMS * sizeof(dtype), hipMemcpyDeviceToHost));

    bool correct = check_axpy_result(y, y_initial, x_initial, alpha);

    if (correct) {
        printf("AXPY test PASSED!\n");
    } else {
        printf("AXPY test FAILED!\n");
    }

    HIP_CHECK(hipFree(d_x));
    HIP_CHECK(hipFree(d_y));

    free(x_initial);
    free(y_initial);
    free(x);
    free(y);

    return correct;
}

int main()
{
    printf("Starting GPU AXPY test with %d elements, %d runs\n", N_ELEMS, N_RUNS);

    bool result = test_axpy();

    if (result) {
        printf("\nAll AXPY tests PASSED!\n");
        return 0;
    } else {
        printf("\nAXPY tests FAILED\n");
        return 1;
    }
}
