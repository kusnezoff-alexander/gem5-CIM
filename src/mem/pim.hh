#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
namespace gem5 {

namespace memory {

class PIM{
	public:
	// Helper: Extract an element of arbitrary bit size from a bit array
	static void extract_element(const uint64_t* src, int bit_offset, int elem_size, uint64_t* out) {
		int word_idx = bit_offset / 64;
		int bit_idx = bit_offset % 64;
		int words_needed = (elem_size + 63) / 64;

		if (bit_idx == 0) {
			// Aligned case - simple copy
			for (int i = 0; i < words_needed; i++) {
				out[i] = src[word_idx + i];
			}
		} else {
			// Unaligned case - need to stitch together parts
			for (int i = 0; i < words_needed; i++) {
				out[i] = src[word_idx + i] >> bit_idx;
				if (word_idx + i + 1 < (bit_offset + elem_size + 63) / 64) {
					out[i] |= src[word_idx + i + 1] << (64 - bit_idx);
				}
			}
		}

		// Mask off excess bits in the last word
		int excess_bits = (words_needed * 64) - elem_size;
		if (excess_bits > 0) {
			out[words_needed - 1] &= (1ULL << (64 - excess_bits)) - 1;
		}
	}

	// Helper: Insert an element of arbitrary bit size into a bit array
	static void insert_element(uint64_t* dest, int bit_offset, int elem_size, const uint64_t* value) {
		int word_idx = bit_offset / 64;
		int bit_idx = bit_offset % 64;
		int words_needed = (elem_size + 63) / 64;

		if (bit_idx == 0) {
			// Aligned case
			for (int i = 0; i < words_needed; i++) {
				dest[word_idx + i] = value[i];
			}
			// Clear excess bits in last word if elem_size not multiple of 64
			int excess_bits = (words_needed * 64) - elem_size;
			if (excess_bits > 0 && words_needed > 0) {
				uint64_t mask = (1ULL << (64 - excess_bits)) - 1;
				dest[word_idx + words_needed - 1] &= mask;
			}
		} else {
			// Unaligned case - need to merge with existing bits
			int bits_written = 0;
			int value_word = 0;

			while (bits_written < elem_size) {
				int bits_in_current_word = 64 - bit_idx;
				int bits_to_write = (elem_size - bits_written < bits_in_current_word)
					? (elem_size - bits_written)
					: bits_in_current_word;

				// Create mask for bits we're writing
				uint64_t write_mask = ((1ULL << bits_to_write) - 1) << bit_idx;

				// Extract bits from value
				uint64_t bits_from_value = (value[value_word] >> (bits_written % 64)) & ((1ULL << bits_to_write) - 1);

				// Clear destination bits and write new bits
				dest[word_idx] = (dest[word_idx] & ~write_mask) | (bits_from_value << bit_idx);

				bits_written += bits_to_write;
				word_idx++;
				bit_idx = 0;  // After first word, we're aligned to word boundary

				if (bits_written % 64 == 0 && bits_written < elem_size) {
					value_word++;
				}
			}
		}
	}

	// Multi-precision addition
	static void mp_add(const uint64_t* a, const uint64_t* b, uint64_t* result, int words) {
		uint64_t carry = 0;
		for (int i = 0; i < words; i++) {
			__uint128_t sum = (__uint128_t)a[i] + b[i] + carry;
			result[i] = (uint64_t)sum;
			carry = sum >> 64;
		}
	}

	// Multi-precision subtraction
	static void mp_sub(const uint64_t* a, const uint64_t* b, uint64_t* result, int words) {
		uint64_t borrow = 0;
		for (int i = 0; i < words; i++) {
			__uint128_t diff = (__uint128_t)a[i] - b[i] - borrow;
			result[i] = (uint64_t)diff;
			borrow = (diff >> 64) ? 1 : 0;
		}
	}

	// Multi-precision left shift
	static void mp_lshift(const uint64_t* a, int shift, uint64_t* result, int words, int elem_size) {
		shift = shift % elem_size;  // Wrap shift amount

		if (shift == 0) {
			memcpy(result, a, words * sizeof(uint64_t));
			return;
		}

		int word_shift = shift / 64;
		int bit_shift = shift % 64;

		// Clear result first
		memset(result, 0, words * sizeof(uint64_t));

		for (int i = words - 1; i >= 0; i--) {
			int src_idx = i - word_shift;
			if (src_idx < 0) continue;

			result[i] = a[src_idx] << bit_shift;
			if (bit_shift > 0 && src_idx > 0) {
				result[i] |= a[src_idx - 1] >> (64 - bit_shift);
			}
		}

		// Mask off bits beyond elem_size
		int excess_bits = (words * 64) - elem_size;
		if (excess_bits > 0) {
			result[words - 1] &= (1ULL << (64 - excess_bits)) - 1;
		}
	}

	// Multi-precision right shift
	static void mp_rshift(const uint64_t* a, int shift, uint64_t* result, int words, int elem_size) {
		shift = shift % elem_size;  // Wrap shift amount

		if (shift == 0) {
			memcpy(result, a, words * sizeof(uint64_t));
			return;
		}

		int word_shift = shift / 64;
		int bit_shift = shift % 64;

		// Clear result first
		memset(result, 0, words * sizeof(uint64_t));

		for (int i = 0; i < words; i++) {
			int src_idx = i + word_shift;
			if (src_idx >= words) continue;

			result[i] = a[src_idx] >> bit_shift;
			if (bit_shift > 0 && src_idx < words - 1) {
				result[i] |= a[src_idx + 1] << (64 - bit_shift);
			}
		}
	}

	enum class Operation {
		ADD,
		SUB,
		LSHIFT,
		RSHIFT
	};

	static void perform_bitwise_operation(Operation op, uint64_t* dest, const uint64_t* src1,
			const uint64_t* src2, int elem_size, int num_elements) {
		int words_per_elem = (elem_size + 63) / 64;

		// Temporary buffers for extraction and computation
		uint64_t val1[8], val2[8], result[8];  // 8 * 64 = 512 bits max

		for (int elem = 0; elem < num_elements; elem++) {
			int bit_offset = elem * elem_size;

			// Extract elements
			extract_element(src1, bit_offset, elem_size, val1);
			extract_element(src2, bit_offset, elem_size, val2);

			// Perform operation
			switch(op) {
				case Operation::ADD:
					mp_add(val1, val2, result, words_per_elem);
					break;
				case Operation::SUB:
					mp_sub(val1, val2, result, words_per_elem);
					break;
				case Operation::LSHIFT: {
											int shift = (int)(val2[0] % elem_size);
											mp_lshift(val1, shift, result, words_per_elem, elem_size);
											break;
										}
				case Operation::RSHIFT: {
											int shift = (int)(val2[0] % elem_size);
											mp_rshift(val1, shift, result, words_per_elem, elem_size);
											break;
										}
			}

			// Mask result to elem_size
			int excess_bits = (words_per_elem * 64) - elem_size;
			if (excess_bits > 0) {
				result[words_per_elem - 1] &= (1ULL << (64 - excess_bits)) - 1;
			}

			// Insert result back
			insert_element(dest, bit_offset, elem_size, result);
		}
	}

	// Helper function to extract and return a single element value (for elements <= 64 bits)
	static uint64_t get_element_value(const uint64_t* arr, int elem_idx, int elem_size) {
		assert(elem_size <= 64);
		uint64_t temp[1];
		extract_element(arr, elem_idx * elem_size, elem_size, temp);
		return temp[0];
	}

	// Helper function to compare multi-word elements
	static bool compare_element(const uint64_t* arr, int elem_idx, int elem_size, const uint64_t* expected) {
		int words_per_elem = (elem_size + 63) / 64;
		uint64_t temp[8];
		extract_element(arr, elem_idx * elem_size, elem_size, temp);

		for (int i = 0; i < words_per_elem; i++) {
			if (temp[i] != expected[i]) return false;
		}
		return true;
	}

	// Helper function to print elements
	static void print_elements(const char* name, const uint64_t* arr, int elem_size, int num_elements) {
		printf("%s:\n", name);
		int words_per_elem = (elem_size + 63) / 64;
		uint64_t temp[8];

		for (int i = 0; i < num_elements; i++) {
			extract_element(arr, i * elem_size, elem_size, temp);
			printf("  [%d] = ", i);

			// Print in hex, most significant word first
			bool started = false;
			for (int w = words_per_elem - 1; w >= 0; w--) {
				if (temp[w] != 0 || started || w == 0) {
					if (started) {
						printf("%016llx", (unsigned long long)temp[w]);
					} else {
						printf("%llx", (unsigned long long)temp[w]);
					}
					started = true;
				}
			}
			printf("\n");
		}
	}

	static void test_operation(const char* test_name, Operation op, int elem_size) {
		printf("\n=== %s (elem_size=%d bits) ===\n", test_name, elem_size);

		int num_elements = 4;
		int total_bits = num_elements * elem_size;
		int total_words = (total_bits + 63) / 64;

		uint64_t* src1 = new uint64_t[total_words]();
		uint64_t* src2 = new uint64_t[total_words]();
		uint64_t* dest = new uint64_t[total_words]();

		// Initialize with test values
		uint64_t val1[8], val2[8];
		for (int i = 0; i < num_elements; i++) {
			memset(val1, 0, sizeof(val1));
			memset(val2, 0, sizeof(val2));

			// Test values
			val1[0] = (i + 1) * 10;
			val2[0] = (i + 1) * 3;

			printf("Inserting elem %d: val1[0]=%llu, val2[0]=%llu\n",
					i, (unsigned long long)val1[0], (unsigned long long)val2[0]);

			insert_element(src1, i * elem_size, elem_size, val1);
			insert_element(src2, i * elem_size, elem_size, val2);

			// Verify insertion
			uint64_t check[8];
			extract_element(src1, i * elem_size, elem_size, check);
			printf("  Verification: extracted src1[%d] = %llu\n", i, (unsigned long long)check[0]);
		}

		print_elements("src1", src1, elem_size, num_elements);
		print_elements("src2", src2, elem_size, num_elements);

		perform_bitwise_operation(op, dest, src1, src2, elem_size, num_elements);

		print_elements("result", dest, elem_size, num_elements);

		// Assertions for elem_size <= 64
		if (elem_size <= 64) {
			uint64_t mask = (elem_size == 64) ? ~0ULL : ((1ULL << elem_size) - 1);
			for (int i = 0; i < num_elements; i++) {
				uint64_t v1 = get_element_value(src1, i, elem_size);
				uint64_t v2 = get_element_value(src2, i, elem_size);
				uint64_t result = get_element_value(dest, i, elem_size);
				uint64_t expected;

				switch(op) {
					case Operation::ADD:
						expected = (v1 + v2) & mask;
						assert(result == expected);
						printf("  Assert passed: [%d] %llu + %llu = %llu\n",
								i, (unsigned long long)v1, (unsigned long long)v2, (unsigned long long)result);
						break;
					case Operation::SUB:
						expected = (v1 - v2) & mask;
						assert(result == expected);
						printf("  Assert passed: [%d] %llu - %llu = %llu\n",
								i, (unsigned long long)v1, (unsigned long long)v2, (unsigned long long)result);
						break;
					default:
						break;
				}
			}
		}

		delete[] src1;
		delete[] src2;
		delete[] dest;
	}

	static void test_large_values() {
		printf("\n=== Test Large 128-bit Values ===\n");

		int elem_size = 128;
		int num_elements = 2;
		int total_words = (num_elements * elem_size + 63) / 64;

		uint64_t* src1 = new uint64_t[total_words]();
		uint64_t* src2 = new uint64_t[total_words]();
		uint64_t* dest = new uint64_t[total_words]();

		// Test 1: Overflow test
		uint64_t val1[2] = {0xFFFFFFFFFFFFFFFFULL, 0x1};
		uint64_t val2[2] = {0x1, 0x0};

		insert_element(src1, 0, elem_size, val1);
		insert_element(src2, 0, elem_size, val2);

		print_elements("src1", src1, elem_size, num_elements);
		print_elements("src2", src2, elem_size, num_elements);

		perform_bitwise_operation(Operation::ADD, dest, src1, src2, elem_size, num_elements);
		print_elements("ADD result (should overflow and wrap)", dest, elem_size, num_elements);

		// Assert: 0x1FFFFFFFFFFFFFFFF + 0x1 = 0x200000000000000000 (but wrapped to 128 bits = 0x0)
		uint64_t expected1[2] = {0x0, 0x2};
		assert(compare_element(dest, 0, elem_size, expected1));
		printf("  Assert passed: 128-bit overflow handled correctly\n");

		// Test 2: Simple addition without overflow
		memset(dest, 0, total_words * sizeof(uint64_t));
		val1[0] = 0x1234567890ABCDEFULL;
		val1[1] = 0xFEDCBA9876543210ULL;
		val2[0] = 0x1111111111111111ULL;
		val2[1] = 0x2222222222222222ULL;

		insert_element(src1, 0, elem_size, val1);
		insert_element(src2, 0, elem_size, val2);

		printf("\nTest 2: Addition with carry propagation\n");
		print_elements("src1", src1, elem_size, 1);
		print_elements("src2", src2, elem_size, 1);

		perform_bitwise_operation(Operation::ADD, dest, src1, src2, elem_size, num_elements);
		print_elements("result", dest, elem_size, 1);

		// Calculate expected using __uint128_t for verification
		__uint128_t a = ((__uint128_t)val1[1] << 64) | val1[0];
		__uint128_t b = ((__uint128_t)val2[1] << 64) | val2[0];
		__uint128_t sum = a + b;

		uint64_t expected2[2];
		expected2[0] = (uint64_t)sum;
		expected2[1] = (uint64_t)(sum >> 64);

		uint64_t result[2];
		extract_element(dest, 0, elem_size, result);

		printf("  Expected: [0]=0x%llx, [1]=0x%llx\n",
				(unsigned long long)expected2[0], (unsigned long long)expected2[1]);
		printf("  Got:      [0]=0x%llx, [1]=0x%llx\n",
				(unsigned long long)result[0], (unsigned long long)result[1]);

		assert(compare_element(dest, 0, elem_size, expected2));
		printf("  Assert passed: 128-bit addition with carry\n");

		delete[] src1;
		delete[] src2;
		delete[] dest;
	}

	static void test_shifts() {
		printf("\n=== Test Shifts (64-bit elements) ===\n");

		int elem_size = 64;
		int num_elements = 3;
		int total_words = num_elements;

		uint64_t* src1 = new uint64_t[total_words]();
		uint64_t* src2 = new uint64_t[total_words]();
		uint64_t* dest = new uint64_t[total_words]();

		uint64_t val1[1], val2[1];

		// Element 0: shift 0xF left by 4
		val1[0] = 0xF;
		val2[0] = 4;
		insert_element(src1, 0, elem_size, val1);
		insert_element(src2, 0, elem_size, val2);

		// Element 1: shift 0xFF00 right by 8
		val1[0] = 0xFF00;
		val2[0] = 8;
		insert_element(src1, elem_size, elem_size, val1);
		insert_element(src2, elem_size, elem_size, val2);

		// Element 2: shift 1 left by 63
		val1[0] = 1;
		val2[0] = 63;
		insert_element(src1, 2 * elem_size, elem_size, val1);
		insert_element(src2, 2 * elem_size, elem_size, val2);

		print_elements("src1", src1, elem_size, num_elements);
		print_elements("shift amounts", src2, elem_size, num_elements);

		// Test left shifts
		perform_bitwise_operation(Operation::LSHIFT, dest, src1, src2, elem_size, num_elements);
		print_elements("LEFT SHIFT results", dest, elem_size, num_elements);

		assert(get_element_value(dest, 0, elem_size) == 0xF0);
		printf("  Assert passed: 0xF << 4 = 0xF0\n");

		assert(get_element_value(dest, 1, elem_size) == 0xFF0000);
		printf("  Assert passed: 0xFF00 << 8 = 0xFF0000\n");

		assert(get_element_value(dest, 2, elem_size) == 0x8000000000000000ULL);
		printf("  Assert passed: 1 << 63 = 0x8000000000000000\n");

		// Test right shifts
		memset(dest, 0, total_words * sizeof(uint64_t));
		perform_bitwise_operation(Operation::RSHIFT, dest, src1, src2, elem_size, num_elements);
		print_elements("RIGHT SHIFT results", dest, elem_size, num_elements);

		assert(get_element_value(dest, 0, elem_size) == 0x0);
		printf("  Assert passed: 0xF >> 4 = 0x0\n");

		assert(get_element_value(dest, 1, elem_size) == 0xFF);
		printf("  Assert passed: 0xFF00 >> 8 = 0xFF\n");

		assert(get_element_value(dest, 2, elem_size) == 0x0);
		printf("  Assert passed: 1 >> 63 = 0x0\n");

		delete[] src1;
		delete[] src2;
		delete[] dest;
	}

	};

}

}
