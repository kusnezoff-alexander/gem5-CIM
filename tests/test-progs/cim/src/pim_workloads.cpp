/**
 *  PIM-only:
 *	- saxpy
 *	- ReLU
 *
 *	PIM & CPU mixed:
 *	- knn
 *	- ...
 */
#include "pim_core.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <type_traits>
using namespace pim_core;
using namespace std;

/* Common config */
constexpr size_t N_ELEMS = 3000;
size_t next_mat = 0;

/* Native INT4 helpers (since there is no `int4_t`) */
constexpr size_t INT4_BITS = 4;

inline int sign_extend4(int x) {
    x &= 0xF;
    return (x & 0x8) ? (x | ~0xF) : x;
}

inline int8_t pack_int4(int lo, int hi) {
    lo &= 0xF; hi &= 0xF;
    return static_cast<int8_t>((hi << 4) | lo);
}

inline int unpack_int4(int8_t byte, int lane) {
    return sign_extend4((byte >> (lane * 4)) & 0xF);
}

/* Generic (int8/int16/int32/int64) */
template<typename T>
void init_data(T* X, T* Y, T* X_init, T* Y_init) {
    int sign = -1;
    for (size_t i = 0; i < N_ELEMS; ++i) {
        X[i] = static_cast<T>(sign * (i % 64));
        Y[i] = static_cast<T>(0x11);
        X_init[i] = X[i];
        Y_init[i] = Y[i];
        sign *= -1;
    }
}

template<typename T>
bool saxpy_with_check(T* Y, const T* X, T a, const T* Y_init, const T* X_init) {
    auto tmp = static_cast<T*>(pim_malloc(N_ELEMS * sizeof(T), next_mat));
    auto a_vec = static_cast<T*>(pim_malloc(N_ELEMS * sizeof(T), next_mat));
    if (!tmp || !a_vec) return false;

    for (size_t i = 0; i < N_ELEMS; ++i) a_vec[i] = a;

    rowmult(tmp, X, a_vec, N_ELEMS, sizeof(T) * 8);
    rowadd(Y, Y, tmp, N_ELEMS, sizeof(T) * 8);

    bool ok = true;
    for (size_t i = 0; i < N_ELEMS; ++i) {
        using wide_t = std::conditional_t<(sizeof(T) < 8), int64_t, __int128_t>;
        wide_t expected = wide_t(a) * wide_t(X_init[i]) + wide_t(Y_init[i]);
        expected = static_cast<T>(expected);
        if (Y[i] != static_cast<T>(expected)) {
            printf("SAXPY ERROR [%zu]: got=%ld exp=%ld\n", i, (long)Y[i], (long)expected);
            ok = false;
            break;
        }
    }
    // pim_free(tmp);
    // pim_free(a_vec);
    return ok;
}

template<typename T>
bool relu_with_check(T* Y, const T* X, const T* X_init) {
    auto zero = static_cast<T*>(pim_malloc(N_ELEMS * sizeof(T), next_mat));
    if (!zero) return false;
    for (size_t i = 0; i < N_ELEMS; ++i) zero[i] = static_cast<T>(0);

    rowmax(Y, X, zero, N_ELEMS, sizeof(T) * 8);

    bool ok = true;
    for (size_t i = 0; i < N_ELEMS; ++i) {
        T expected = (X_init[i] > 0) ? X_init[i] : static_cast<T>(0);
        if (Y[i] != expected) {
            printf("ReLU ERROR [%zu]: got=%ld exp=%ld\n", i, (long)Y[i], (long)expected);
            ok = false;
            break;
        }
    }
    // pim_free(zero);
    return ok;
}

template<typename T>
int run_generic_tests() {
    auto X_init = static_cast<T*>(malloc(N_ELEMS * sizeof(T)));
    auto Y_init = static_cast<T*>(malloc(N_ELEMS * sizeof(T)));
    auto X = static_cast<T*>(pim_malloc(N_ELEMS * sizeof(T), next_mat));
    auto Y = static_cast<T*>(pim_malloc(N_ELEMS * sizeof(T), next_mat));
    auto Y_relu = static_cast<T*>(pim_malloc(N_ELEMS * sizeof(T), next_mat));
    if (!X || !Y || !Y_relu) return 0;

    init_data(X, Y, X_init, Y_init);
    T a = static_cast<T>(3);

    bool ok_saxpy = saxpy_with_check(Y, X, a, Y_init, X_init);
    bool ok_relu = relu_with_check(Y_relu, X, X_init);

    printf("SAXPY: %s\n", ok_saxpy ? "PASSED" : "FAILED");
    printf("ReLU:  %s\n", ok_relu ? "PASSED" : "FAILED");

    // pim_free(X);
    // pim_free(Y);
    // pim_free(Y_relu);
    free(X_init);
    free(Y_init);

    int passed = 0;
    if (ok_saxpy) passed++;
    if (ok_relu) passed++;
    return passed;
}

/** Native INT4 */
int run_int4_tests() {
    constexpr size_t N_BYTES = (N_ELEMS + 1) / 2;
    auto X_init = static_cast<int8_t*>(malloc(N_BYTES));
    auto Y_init = static_cast<int8_t*>(malloc(N_BYTES));
    auto X = static_cast<int8_t*>(pim_malloc(N_BYTES, next_mat));
    auto Y = static_cast<int8_t*>(pim_malloc(N_BYTES, next_mat));
    auto Y_relu = static_cast<int8_t*>(pim_malloc(N_BYTES, next_mat));
    if (!X || !Y || !Y_relu) return 0;

    for (size_t i = 0; i < N_BYTES; ++i) {
        int x0 = (i % 8) - 4;
        int x1 = ((i + 1) % 8) - 4;
        X[i] = pack_int4(x0, x1);
        Y[i] = pack_int4(1, -1);
        X_init[i] = X[i];
        Y_init[i] = Y[i];
    }

    auto tmp = static_cast<int8_t*>(pim_malloc(N_BYTES, next_mat));
    auto a_vec = static_cast<int8_t*>(pim_malloc(N_BYTES, next_mat));
    int a = 3;
    for (size_t i = 0; i < N_BYTES; ++i) a_vec[i] = pack_int4(a, a);

    rowmult(tmp, X, a_vec, N_ELEMS, INT4_BITS);
    rowadd(Y, Y, tmp, N_ELEMS, INT4_BITS);

    bool ok_saxpy = true;
    for (size_t i = 0; i < N_BYTES; ++i) {
        for (int lane = 0; lane < 2; ++lane) {
            int x = unpack_int4(X_init[i], lane);
            int y = unpack_int4(Y_init[i], lane);
            int r = unpack_int4(Y[i], lane);
            int expected = sign_extend4((a * x + y) & 0xF);
            if (r != expected) {
                printf("INT4 SAXPY ERROR byte=%zu lane=%d got=%d exp=%d\n", i, lane, r, expected);
                ok_saxpy = false;
                goto relu;
            }
        }
    }

relu:
    auto zero = static_cast<int8_t*>(pim_malloc(N_BYTES, next_mat));
    for (size_t i = 0; i < N_BYTES; ++i) zero[i] = pack_int4(0, 0);

    rowmax(Y_relu, X, zero, N_ELEMS, INT4_BITS);

    bool ok_relu = true;
    for (size_t i = 0; i < N_BYTES && ok_relu; ++i) {
        for (int lane = 0; lane < 2; ++lane) {
            int x = unpack_int4(X_init[i], lane);
            int r = unpack_int4(Y_relu[i], lane);
            int expected = sign_extend4((x > 0 ? x : 0) & 0xF);
            if (r != expected) {
                printf("INT4 ReLU ERROR byte=%zu lane=%d got=%d exp=%d\n", i, lane, r, expected);
                ok_relu = false;
                break;
            }
        }
    }

    printf("INT4 SAXPY: %s\n", ok_saxpy ? "PASSED" : "FAILED");
    printf("INT4 ReLU:  %s\n", ok_relu ? "PASSED" : "FAILED");

    // pim_free(tmp);
    // pim_free(a_vec);
    // pim_free(zero);
    // pim_free(X);
    // pim_free(Y);
    // pim_free(Y_relu);
    free(X_init);
    free(Y_init);

    int passed = 0;
    if (ok_saxpy) passed++;
    if (ok_relu) passed++;
    return passed;
}

int main() {
    cout << "Running SAXPY + ReLU tests\n";

    int total_passed = 0;
    int max_tests = 0;

    cout << "\nINT4:\n";
    total_passed += run_int4_tests();
    max_tests += 2;

    cout << "\nINT8:\n";
    total_passed += run_generic_tests<int8_t>();
    max_tests += 2;

    cout << "\nINT16:\n";
    total_passed += run_generic_tests<int16_t>();
    max_tests += 2;

    cout << "\nINT32:\n";
    total_passed += run_generic_tests<int32_t>();
    max_tests += 2;

    cout << "\nINT64:\n";
    total_passed += run_generic_tests<int64_t>();
    max_tests += 2;

    cout << "\n=========================\n";
    cout << "Total workloads passed: " << total_passed
         << " / " << max_tests << "\n";

    return 0;
}
