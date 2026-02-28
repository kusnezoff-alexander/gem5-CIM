// GPU (HIP) implementation of row operations benchmark
// Designed for gem5 VEGA_X86 APU simulation

#include <hip/hip_runtime.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <chrono>
#include <gem5/m5ops.h>

using namespace std;
using namespace std::chrono;

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

// Kernel wrapper function declarations (defined in gpu_kernels.hip)
extern "C" {
    void gpu_rowand(dtype* dst, const dtype* src1, const dtype* src2, size_t size);
    void gpu_rowadd(dtype* dst, const dtype* src1, const dtype* src2, size_t size);
    void gpu_rowsub(dtype* dst, const dtype* src1, const dtype* src2, size_t size);
    void gpu_rowmult(dtype* dst, const dtype* src1, const dtype* src2, size_t size);
    void gpu_rowmin(dtype* dst, const dtype* src1, const dtype* src2, size_t size);
    void gpu_rowmax(dtype* dst, const dtype* src1, const dtype* src2, size_t size);
    void gpu_rowequal(dtype* dst, const dtype* src1, const dtype* src2, size_t size);
    void gpu_rowgreater(dtype* dst, const dtype* src1, const dtype* src2, size_t size);
    void gpu_rowgreater_equal(dtype* dst, const dtype* src1, const dtype* src2, size_t size);
    void gpu_rowif_else(dtype* dst, const dtype* src1, const dtype* src2, const dtype* mask, size_t size);
    void gpu_rowabs(dtype* dst, const dtype* src, size_t size);
    void gpu_rowbitcount(dtype* dst, const dtype* src, size_t size);
}

const char* op_names[] = {
    "rowand", "rowadd", "rowsub", "rowmult",
    "rowmin", "rowmax", "rowequal", "rowgreater", "rowgreater_equal",
    "rowif_else", "rowabs", "bitcount"
};

// Initialize data on host
void init_data(dtype* array1, dtype* array2, dtype* array1_initial_val, dtype* array2_initial_val) {
    auto sign = -1;
    for(size_t i=0; i<N_ELEMS; ++i) {
        array1[i] = sign*i;
        array2[i] = 0x111;
        array1_initial_val[i] = array1[i];
        array2_initial_val[i] = array2[i];
        sign *= -1;
    }
}

// Check results
template<typename Op>
bool check_result(dtype* res, dtype* array1_initial_val, dtype* array2_initial_val, Op op) {
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

// Check results for 3-operand operations
template<typename Op>
bool check_result(dtype* res, dtype* array1_initial_val, dtype* array2_initial_val,
                  dtype* mask_initial_val, Op op) {
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

// Helper macro for HIP error checking
#define HIP_CHECK(cmd) \
    do { \
        hipError_t error = cmd; \
        if (error != hipSuccess) { \
            fprintf(stderr, "HIP error: '%s' (%d) at %s:%d\n", \
                    hipGetErrorString(error), error, __FILE__, __LINE__); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

int main(int argc, char* argv[])
{
    bool run_checks = false;
    int op_id = 1;

    // Parse arguments
    if (argc == 2) {
        if (string(argv[1]) == "--check") {
            run_checks = true;
            op_id = 1;  // default to first operation
        } else {
            op_id = atoi(argv[1]);
        }
    } else if (argc == 3 && string(argv[2]) == "--check") {
        run_checks = true;
        op_id = atoi(argv[1]);
    } else if (argc != 1) {
        cerr << "Usage: " << argv[0] << " [op_id] [--check]" << endl;
        return 1;
    }

    if (op_id < 1 || op_id > 12) {
        cerr << "Invalid op_id: " << op_id << endl;
        return 1;
    }

    cout << "GPU (HIP): Running " << op_names[op_id-1] << " (op_id=" << op_id << ")" << endl;

    // Allocate host memory
    auto array1_initial_val = static_cast<dtype*>(malloc(N_ELEMS*sizeof(dtype)));
    auto array2_initial_val = static_cast<dtype*>(malloc(N_ELEMS*sizeof(dtype)));
    auto array1_host = static_cast<dtype*>(malloc(N_ELEMS*sizeof(dtype)));
    auto array2_host = static_cast<dtype*>(malloc(N_ELEMS*sizeof(dtype)));

    if (!array1_host || !array2_host || !array1_initial_val || !array2_initial_val) {
        printf("ERROR: malloc failed\n");
        return 1;
    }

    // Initialize data
    init_data(array1_host, array2_host, array1_initial_val, array2_initial_val);

    // Allocate device memory
    dtype *d_array1, *d_array2;
    HIP_CHECK(hipMalloc(&d_array1, N_ELEMS * sizeof(dtype)));
    HIP_CHECK(hipMalloc(&d_array2, N_ELEMS * sizeof(dtype)));

    // Copy data to device
    HIP_CHECK(hipMemcpy(d_array1, array1_host, N_ELEMS * sizeof(dtype), hipMemcpyHostToDevice));
    HIP_CHECK(hipMemcpy(d_array2, array2_host, N_ELEMS * sizeof(dtype), hipMemcpyHostToDevice));

    m5_reset_stats(0, 0);

    auto start = high_resolution_clock::now();

    bool passed = false;

    // Execute the selected operation
    switch(op_id) {
        case 1: { // rowand
            gpu_rowand(d_array1, d_array2, d_array1, N_ELEMS);
            HIP_CHECK(hipDeviceSynchronize());
            HIP_CHECK(hipMemcpy(array1_host, d_array1, N_ELEMS * sizeof(dtype), hipMemcpyDeviceToHost));
            if (run_checks) passed = check_result(array1_host, array1_initial_val, array2_initial_val, bit_and<dtype>{});
            break;
        }
        case 2: { // rowadd
            gpu_rowadd(d_array1, d_array2, d_array1, N_ELEMS);
            HIP_CHECK(hipDeviceSynchronize());
            HIP_CHECK(hipMemcpy(array1_host, d_array1, N_ELEMS * sizeof(dtype), hipMemcpyDeviceToHost));
            if (run_checks) passed = check_result(array1_host, array1_initial_val, array2_initial_val, plus<dtype>{});
            break;
        }
        case 3: { // rowsub
            gpu_rowsub(d_array1, d_array1, d_array2, N_ELEMS);
            HIP_CHECK(hipDeviceSynchronize());
            HIP_CHECK(hipMemcpy(array1_host, d_array1, N_ELEMS * sizeof(dtype), hipMemcpyDeviceToHost));
            if (run_checks) passed = check_result(array1_host, array1_initial_val, array2_initial_val, minus<dtype>{});
            break;
        }
        case 4: { // rowmult
            gpu_rowmult(d_array1, d_array2, d_array1, N_ELEMS);
            HIP_CHECK(hipDeviceSynchronize());
            HIP_CHECK(hipMemcpy(array1_host, d_array1, N_ELEMS * sizeof(dtype), hipMemcpyDeviceToHost));
            if (run_checks) passed = check_result(array1_host, array1_initial_val, array2_initial_val, multiplies<dtype>{});
            break;
        }
        case 5: { // rowmin
            auto min_op = [](auto a, auto b) { return a < b ? a : b; };
            gpu_rowmin(d_array1, d_array1, d_array2, N_ELEMS);
            HIP_CHECK(hipDeviceSynchronize());
            HIP_CHECK(hipMemcpy(array1_host, d_array1, N_ELEMS * sizeof(dtype), hipMemcpyDeviceToHost));
            if (run_checks) passed = check_result(array1_host, array1_initial_val, array2_initial_val, min_op);
            break;
        }
        case 6: { // rowmax
            auto max_op = [](auto a, auto b) { return a > b ? a : b; };
            gpu_rowmax(d_array1, d_array1, d_array2, N_ELEMS);
            HIP_CHECK(hipDeviceSynchronize());
            HIP_CHECK(hipMemcpy(array1_host, d_array1, N_ELEMS * sizeof(dtype), hipMemcpyDeviceToHost));
            if (run_checks) passed = check_result(array1_host, array1_initial_val, array2_initial_val, max_op);
            break;
        }
        case 7: { // rowequal
            auto row_equal = [](dtype a, dtype b) -> dtype {
                return (a == b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
            };
            gpu_rowequal(d_array1, d_array1, d_array2, N_ELEMS);
            HIP_CHECK(hipDeviceSynchronize());
            HIP_CHECK(hipMemcpy(array1_host, d_array1, N_ELEMS * sizeof(dtype), hipMemcpyDeviceToHost));
            if (run_checks) passed = check_result(array1_host, array1_initial_val, array2_initial_val, row_equal);
            break;
        }
        case 8: { // rowgreater
            auto row_greater = [](dtype a, dtype b) -> dtype {
                return (a > b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
            };
            gpu_rowgreater(d_array1, d_array1, d_array2, N_ELEMS);
            HIP_CHECK(hipDeviceSynchronize());
            HIP_CHECK(hipMemcpy(array1_host, d_array1, N_ELEMS * sizeof(dtype), hipMemcpyDeviceToHost));
            if (run_checks) passed = check_result(array1_host, array1_initial_val, array2_initial_val, row_greater);
            break;
        }
        case 9: { // rowgreater_equal
            auto row_greater_equal = [](dtype a, dtype b) -> dtype {
                return (a >= b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
            };
            gpu_rowgreater_equal(d_array1, d_array1, d_array2, N_ELEMS);
            HIP_CHECK(hipDeviceSynchronize());
            HIP_CHECK(hipMemcpy(array1_host, d_array1, N_ELEMS * sizeof(dtype), hipMemcpyDeviceToHost));
            if (run_checks) passed = check_result(array1_host, array1_initial_val, array2_initial_val, row_greater_equal);
            break;
        }
        case 10: { // rowif_else
            auto mask_initial_val = new dtype[N_ELEMS];
            for (size_t i = 0; i < N_ELEMS; ++i) {
                mask_initial_val[i] = array1_initial_val[i];
            }
            auto row_ifelse = [](dtype mask_val, dtype a, dtype b) -> dtype {
                return (mask_val != 0) ? a : b;
            };
            gpu_rowif_else(d_array1, d_array1, d_array2, d_array1, N_ELEMS);
            HIP_CHECK(hipDeviceSynchronize());
            HIP_CHECK(hipMemcpy(array1_host, d_array1, N_ELEMS * sizeof(dtype), hipMemcpyDeviceToHost));
            if (run_checks) passed = check_result(array1_host, array1_initial_val, array2_initial_val, mask_initial_val, row_ifelse);
            delete[] mask_initial_val;
            break;
        }
        case 11: { // rowabs
            auto row_abs = [](dtype a, dtype) -> dtype {
                return (a < 0) ? static_cast<dtype>(-a) : a;
            };
            gpu_rowabs(d_array1, d_array1, N_ELEMS);
            HIP_CHECK(hipDeviceSynchronize());
            HIP_CHECK(hipMemcpy(array1_host, d_array1, N_ELEMS * sizeof(dtype), hipMemcpyDeviceToHost));
            if (run_checks) passed = check_result(array1_host, array1_initial_val, array2_initial_val, row_abs);
            break;
        }
        case 12: { // rowbitcount
            auto row_bitcount = [](dtype a, dtype) -> dtype {
                unsigned int count = 0;
                unsigned int val = static_cast<unsigned int>(a);
                while (val) {
                    count += val & 1;
                    val >>= 1;
                }
                return static_cast<dtype>(count);
            };
            gpu_rowbitcount(d_array1, d_array1, N_ELEMS);
            HIP_CHECK(hipDeviceSynchronize());
            HIP_CHECK(hipMemcpy(array1_host, d_array1, N_ELEMS * sizeof(dtype), hipMemcpyDeviceToHost));
            if (run_checks) passed = check_result(array1_host, array1_initial_val, array2_initial_val, row_bitcount);
            break;
        }
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(end - start);

    cout << "Runtime: " << duration.count() << " ns" << endl;
    if (run_checks) cout << "Result: " << (passed ? "PASSED" : "FAILED") << endl;

    // Cleanup
    HIP_CHECK(hipFree(d_array1));
    HIP_CHECK(hipFree(d_array2));
    free(array1_host);
    free(array2_host);
    free(array1_initial_val);
    free(array2_initial_val);

    return passed ? 0 : 1;
}
