// SAXPY (Single-precision A*X Plus Y) CUDA benchmark
// Computes: Y = A * X + Y

#include <cuda_runtime.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <random>
#include <cmath>

using namespace std;
using namespace std::chrono;

const size_t N_ELEMS = 10000000;
const float A_CONST = 2.5f;

__global__ void saxpy_kernel(float* y, const float* x, float a, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        y[idx] = a * x[idx] + y[idx];
    }
}

__global__ void saxpy_kernel_double(double* y, const double* x, double a, size_t size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        y[idx] = a * x[idx] + y[idx];
    }
}

#define CUDA_CHECK(cmd) \
    do { \
        cudaError_t error = cmd; \
        if (error != cudaSuccess) { \
            fprintf(stderr, "CUDA error: '%s' (%d) at %s:%d\n", \
                    cudaGetErrorString(error), error, __FILE__, __LINE__); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

bool verify_saxpy(float* y, float* x, float a, size_t size) {
    bool passed = true;
    for (size_t i = 0; i < size; ++i) {
        float expected = a * x[i] + (float)(i % 100);
        if (fabsf(y[i] - expected) > 0.001f) {
            printf("Mismatch at index %zu: got %f, expected %f\n", i, y[i], expected);
            passed = false;
            if (i > 20) break;
        }
    }
    return passed;
}

int main(int argc, char* argv[])
{
    bool run_checks = false;
    bool use_double = false;
    
    for (int i = 1; i < argc; ++i) {
        if (string(argv[i]) == "--check") {
            run_checks = true;
        } else if (string(argv[i]) == "--double") {
            use_double = true;
        }
    }

    cout << "CUDA SAXPY Benchmark" << endl;
    cout << "Elements: " << N_ELEMS << endl;
    cout << "A constant: " << A_CONST << endl;
    cout << "Precision: " << (use_double ? "double" : "float") << endl;

    float *x_host, *y_host;
    float *d_x, *d_y;
    
    if (use_double) {
        x_host = (float*)malloc(N_ELEMS * sizeof(double));
        y_host = (float*)malloc(N_ELEMS * sizeof(double));
    } else {
        x_host = (float*)malloc(N_ELEMS * sizeof(float));
        y_host = (float*)malloc(N_ELEMS * sizeof(float));
    }

    if (!x_host || !y_host) {
        printf("ERROR: malloc failed\n");
        return 1;
    }

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 100.0f);

    for (size_t i = 0; i < N_ELEMS; ++i) {
        x_host[i] = dist(rng);
        y_host[i] = (float)(i % 100);
    }

    CUDA_CHECK(cudaMalloc(&d_x, N_ELEMS * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_y, N_ELEMS * sizeof(float)));

    CUDA_CHECK(cudaMemcpy(d_x, x_host, N_ELEMS * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_y, y_host, N_ELEMS * sizeof(float), cudaMemcpyHostToDevice));

    int threadsPerBlock = 256;
    int blocksPerGrid = (N_ELEMS + threadsPerBlock - 1) / threadsPerBlock;

    auto start = high_resolution_clock::now();

    if (use_double) {
        saxpy_kernel_double<<<blocksPerGrid, threadsPerBlock>>>(
            (double*)d_y, (const double*)d_x, (double)A_CONST, N_ELEMS);
    } else {
        saxpy_kernel<<<blocksPerGrid, threadsPerBlock>>>(
            d_y, d_x, A_CONST, N_ELEMS);
    }

    CUDA_CHECK(cudaDeviceSynchronize());

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(end - start);

    CUDA_CHECK(cudaMemcpy(y_host, d_y, N_ELEMS * sizeof(float), cudaMemcpyDeviceToHost));

    float throughput = (float)N_ELEMS / (duration.count() / 1e9f) / 1e9f;
    cout << "Runtime: " << duration.count() << " ns" << endl;
    cout << "Throughput: " << throughput << " Gelements/s" << endl;

    bool passed = true;
    if (run_checks) {
        passed = verify_saxpy(y_host, x_host, A_CONST, N_ELEMS);
        cout << "Result: " << (passed ? "PASSED" : "FAILED") << endl;
    }

    CUDA_CHECK(cudaFree(d_x));
    CUDA_CHECK(cudaFree(d_y));
    free(x_host);
    free(y_host);

    return passed ? 0 : 1;
}
