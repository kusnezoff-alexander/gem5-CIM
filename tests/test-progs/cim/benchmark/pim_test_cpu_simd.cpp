#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <sys/types.h>
#include <type_traits>
#include <functional>
#include <iostream>
#include <string>
#include <chrono>
#include <immintrin.h>

using namespace std;
using namespace std::chrono;

const size_t N_ELEMS = 3000;

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

using dtype = int16_t;

const char* op_names[] = {
    "rowand", "rowadd", "rowsub", "rowmult", "rowdiv",
    "rowmin", "rowmax", "rowequal", "rowgreater", "rowgreater_equal",
    "rowif_else", "rowabs", "bitcount"
};

constexpr size_t SIMD_WIDTH = 8;

void simd_rowand(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    size_t i = 0;
    for (; i + SIMD_WIDTH <= size; i += SIMD_WIDTH) {
        __m128i a = _mm_loadu_si128((__m128i const*)(src1 + i));
        __m128i b = _mm_loadu_si128((__m128i const*)(src2 + i));
        __m128i result = _mm_and_si128(a, b);
        _mm_storeu_si128((__m128i*)(dst + i), result);
    }
	for (; i < size; ++i) {
        dst[i] = src1[i] & src2[i];
    }
}

void simd_rowadd(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    size_t i = 0;
    for (; i + SIMD_WIDTH <= size; i += SIMD_WIDTH) {
        __m128i a = _mm_loadu_si128((__m128i const*)(src1 + i));
        __m128i b = _mm_loadu_si128((__m128i const*)(src2 + i));
        __m128i result = _mm_adds_epi16(a, b);
        _mm_storeu_si128((__m128i*)(dst + i), result);
    }
    for (; i < size; ++i) {
        dst[i] = src1[i] + src2[i];
    }
}

void simd_rowsub(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    size_t i = 0;
    for (; i + SIMD_WIDTH <= size; i += SIMD_WIDTH) {
        __m128i a = _mm_loadu_si128((__m128i const*)(src1 + i));
        __m128i b = _mm_loadu_si128((__m128i const*)(src2 + i));
        __m128i result = _mm_subs_epi16(a, b);
        _mm_storeu_si128((__m128i*)(dst + i), result);
    }
    for (; i < size; ++i) {
        dst[i] = src1[i] - src2[i];
    }
}

void simd_rowmult(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    size_t i = 0;
    for (; i + SIMD_WIDTH <= size; i += SIMD_WIDTH) {
        __m128i a = _mm_loadu_si128((__m128i const*)(src1 + i));
        __m128i b = _mm_loadu_si128((__m128i const*)(src2 + i));
        __m128i result = _mm_mullo_epi16(a, b);
        _mm_storeu_si128((__m128i*)(dst + i), result);
    }
    for (; i < size; ++i) {
        dst[i] = src1[i] * src2[i];
    }
}

void simd_rowdiv(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        dst[i] = src2[i] != 0 ? src1[i] / src2[i] : 0;
    }
}

void simd_rowmin(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    size_t i = 0;
    for (; i + SIMD_WIDTH <= size; i += SIMD_WIDTH) {
        __m128i a = _mm_loadu_si128((__m128i const*)(src1 + i));
        __m128i b = _mm_loadu_si128((__m128i const*)(src2 + i));
        __m128i result = _mm_min_epi16(a, b);
        _mm_storeu_si128((__m128i*)(dst + i), result);
    }
    for (; i < size; ++i) {
        dst[i] = src1[i] < src2[i] ? src1[i] : src2[i];
    }
}

void simd_rowmax(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    size_t i = 0;
    for (; i + SIMD_WIDTH <= size; i += SIMD_WIDTH) {
        __m128i a = _mm_loadu_si128((__m128i const*)(src1 + i));
        __m128i b = _mm_loadu_si128((__m128i const*)(src2 + i));
        __m128i result = _mm_max_epi16(a, b);
        _mm_storeu_si128((__m128i*)(dst + i), result);
    }
    for (; i < size; ++i) {
        dst[i] = src1[i] > src2[i] ? src1[i] : src2[i];
    }
}

void simd_rowequal(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    size_t i = 0;
    __m128i all_ones = _mm_set1_epi16(0xFFFF);
    __m128i all_zeros = _mm_set1_epi16(0x0000);
    for (; i + SIMD_WIDTH <= size; i += SIMD_WIDTH) {
        __m128i a = _mm_loadu_si128((__m128i const*)(src1 + i));
        __m128i b = _mm_loadu_si128((__m128i const*)(src2 + i));
        __m128i cmp = _mm_cmpeq_epi16(a, b);
        __m128i result = _mm_blendv_epi8(all_zeros, all_ones, cmp);
        _mm_storeu_si128((__m128i*)(dst + i), result);
    }
    for (; i < size; ++i) {
        dst[i] = (src1[i] == src2[i]) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
    }
}

void simd_rowgreater(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    size_t i = 0;
    __m128i all_ones = _mm_set1_epi16(0xFFFF);
    __m128i all_zeros = _mm_set1_epi16(0x0000);
    for (; i + SIMD_WIDTH <= size; i += SIMD_WIDTH) {
        __m128i a = _mm_loadu_si128((__m128i const*)(src1 + i));
        __m128i b = _mm_loadu_si128((__m128i const*)(src2 + i));
        __m128i cmp = _mm_cmpgt_epi16(a, b);
        __m128i result = _mm_blendv_epi8(all_zeros, all_ones, cmp);
        _mm_storeu_si128((__m128i*)(dst + i), result);
    }
    for (; i < size; ++i) {
        dst[i] = (src1[i] > src2[i]) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
    }
}

void simd_rowgreater_equal(dtype* dst, const dtype* src1, const dtype* src2, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        dst[i] = (src1[i] >= src2[i]) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
    }
}

void simd_rowif_else(dtype* dst, const dtype* src1, const dtype* src2, const dtype* mask, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        dst[i] = (mask[i] != 0) ? src1[i] : src2[i];
    }
}

void simd_rowabs(dtype* dst, const dtype* src, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        dst[i] = src[i] < 0 ? -src[i] : src[i];
    }
}

void simd_rowbitcount(dtype* dst, const dtype* src, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        unsigned int count = 0;
        unsigned int val = static_cast<unsigned int>(src[i]);
        while (val) {
            count += val & 1;
            val >>= 1;
        }
        dst[i] = static_cast<dtype>(count);
    }
}

int main(int argc, char* argv[])
{
    bool run_checks = false;
    int op_id = 1;

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

    if (op_id < 1 || op_id > 13) {
        cerr << "Invalid op_id: " << op_id << endl;
        return 1;
    }

    cout << "CPU SIMD: Running " << op_names[op_id-1] << " (op_id=" << op_id << ")" << endl;

    auto array1_initial_val = static_cast<dtype*>(malloc(N_ELEMS*sizeof(dtype)));
    auto array2_initial_val = static_cast<dtype*>(malloc(N_ELEMS*sizeof(dtype)));
    auto array1 = static_cast<dtype*>(malloc(N_ELEMS*sizeof(dtype)));
    auto array2 = static_cast<dtype*>(malloc(N_ELEMS*sizeof(dtype)));

    if (!array1 || !array2 || !array1_initial_val || !array2_initial_val) {
        printf("ERROR: malloc failed\n");
        return 1;
    }

    init_data(array1, array2, array1_initial_val, array2_initial_val);

    auto start = high_resolution_clock::now();

    bool passed = false;

    switch(op_id) {
        case 1: {
            simd_rowand(array1, array2, array1, N_ELEMS);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, bit_and<dtype>{});
            break;
        }
        case 2: {
            simd_rowadd(array1, array2, array1, N_ELEMS);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, plus<dtype>{});
            break;
        }
        case 3: {
            simd_rowsub(array1, array1, array2, N_ELEMS);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, minus<dtype>{});
            break;
        }
        case 4: {
            simd_rowmult(array1, array2, array1, N_ELEMS);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, multiplies<dtype>{});
            break;
        }
        case 5: {
            simd_rowdiv(array1, array1, array2, N_ELEMS);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, divides<dtype>{});
            break;
        }
        case 6: {
            auto min_op = [](auto a, auto b) { return a < b ? a : b; };
            simd_rowmin(array1, array1, array2, N_ELEMS);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, min_op);
            break;
        }
        case 7: {
            auto max_op = [](auto a, auto b) { return a > b ? a : b; };
            simd_rowmax(array1, array1, array2, N_ELEMS);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, max_op);
            break;
        }
        case 8: {
            auto row_equal = [](dtype a, dtype b) -> dtype {
                return (a == b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
            };
            simd_rowequal(array1, array1, array2, N_ELEMS);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, row_equal);
            break;
        }
        case 9: {
            auto row_greater = [](dtype a, dtype b) -> dtype {
                return (a > b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
            };
            simd_rowgreater(array1, array1, array2, N_ELEMS);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, row_greater);
            break;
        }
        case 10: {
            auto row_greater_equal = [](dtype a, dtype b) -> dtype {
                return (a >= b) ? static_cast<dtype>(0xFFFF) : static_cast<dtype>(0);
            };
            simd_rowgreater_equal(array1, array1, array2, N_ELEMS);
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
            simd_rowif_else(array1, array1, array2, array1_initial_val, N_ELEMS);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, mask_initial_val, row_ifelse);
            delete[] mask_initial_val;
            break;
        }
        case 12: {
            auto row_abs = [](dtype a, dtype) -> dtype {
                return (a < 0) ? static_cast<dtype>(-a) : a;
            };
            simd_rowabs(array1, array1, N_ELEMS);
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
            simd_rowbitcount(array1, array1, N_ELEMS);
            if (run_checks) passed = check_result(array1, array1_initial_val, array2_initial_val, row_bitcount);
            break;
        }
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(end - start);

    cout << "Runtime: " << duration.count() << " ns" << endl;
    if (run_checks) cout << "Result: " << (passed ? "PASSED" : "FAILED") << endl;

    free(array1);
    free(array2);
    free(array1_initial_val);
    free(array2_initial_val);

    return passed ? 0 : 1;
}
