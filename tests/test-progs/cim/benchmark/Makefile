# Makefile for CIM benchmark tests
#
# GPU Support:
#   The GPU target requires ROCm/HIP to be fully installed.
#   To build GPU variant: make gpu
#   This will use hipcc to compile HIP kernels for AMD GPUs.
#
#   If ROCm is not installed, install it first or the GPU target will fail.
#   See: https://rocm.docs.amd.com/en/latest/deploy/linux/quick_start.html

CXX := g++
HIPCC := hipcc
AR := ar
ARFLAGS := rcs

GEM5_DIR := /home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix
SRC_DIR := $(GEM5_DIR)/tests/test-progs/cim/src
M5_DIR := $(GEM5_DIR)/util/m5/build/x86/out
M5_LIB := $(M5_DIR)/libm5.a

# Common flags for all targets
CXXFLAGS_COMMON := -Wall -Wextra -std=c++20
CXXFLAGS_COMMON += -I$(GEM5_DIR)/include
CXXFLAGS_COMMON += -I$(SRC_DIR)

# Serial: NO SIMD, NO auto-vectorization (gem5 compatible)
CXXFLAGS_SERIAL := $(CXXFLAGS_COMMON) -O2 -fno-tree-vectorize -mno-sse -mno-avx

# SIMD: SSE4.1 optimization (gem5 compatible)
CXXFLAGS_SIMD := $(CXXFLAGS_COMMON) -O2 -msse4.1

# CIM: Similar to serial, no vectorization
CXXFLAGS_CIM := $(CXXFLAGS_COMMON) -O2 -fno-tree-vectorize -mno-avx

# GPU: HIP compilation flags
HIPFLAGS := -O2 -std=c++17
HIPFLAGS += -I$(GEM5_DIR)/include
HIPFLAGS += -I$(SRC_DIR)

# Library: Use serial flags (no SIMD)
CXXFLAGS_LIB := $(CXXFLAGS_COMMON) -O2 -fno-tree-vectorize

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj

LIB := $(SRC_DIR)/build/lib/libpim.a
CORE_OBJ := $(OBJ_DIR)/pim_core.o

SRCS := pim_test_cpu_serial.cpp pim_test_cpu_simd.cpp pim_test_cim.cpp
OBJS := $(SRCS:.cpp=.o)
BINS := pim_test_cpu_serial pim_test_cpu_simd pim_test_cim pim_test_gpu
GPU_OBJS := pim_test_gpu.o gpu_kernels.hip.o

.PHONY: all clean lib verify-serial gpu

all: lib $(BINS)

gpu: pim_test_gpu

lib: $(LIB)

$(LIB): $(CORE_OBJ)
	$(AR) $(ARFLAGS) $@ $^

$(OBJ_DIR)/pim_core.o: $(SRC_DIR)/pim_core.cpp $(SRC_DIR)/pim_core.h | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS_LIB) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

# Target-specific compilation rules (no generic pattern rule)
pim_test_cpu_serial.o: pim_test_cpu_serial.cpp
	$(CXX) $(CXXFLAGS_SERIAL) -c $< -o $@

pim_test_cpu_simd.o: pim_test_cpu_simd.cpp
	$(CXX) $(CXXFLAGS_SIMD) -c $< -o $@

pim_test_cim.o: pim_test_cim.cpp
	$(CXX) $(CXXFLAGS_CIM) -c $< -o $@

pim_test_cpu_serial: pim_test_cpu_serial.o $(LIB)
	$(CXX) $(CXXFLAGS_SERIAL) $< -L$(SRC_DIR)/build/lib -lpim $(M5_LIB) -o $@

pim_test_cpu_simd: pim_test_cpu_simd.o $(LIB)
	$(CXX) $(CXXFLAGS_SIMD) $< -L$(SRC_DIR)/build/lib -lpim -o $@

pim_test_cim: pim_test_cim.o $(LIB)
	$(CXX) $(CXXFLAGS_CIM) $< -L$(SRC_DIR)/build/lib -lpim $(M5_LIB) -o $@

# GPU target-specific compilation rules
pim_test_gpu.o: pim_test_gpu.cpp
	$(HIPCC) $(HIPFLAGS) -c $< -o $@

gpu_kernels.hip.o: gpu_kernels.hip
	$(HIPCC) $(HIPFLAGS) -c $< -o $@

pim_test_gpu: pim_test_gpu.o gpu_kernels.hip.o
	$(HIPCC) $(HIPFLAGS) $^ $(M5_LIB) -o $@

verify-serial:
	@echo "Checking for SIMD instructions in serial binary..."
	@objdump -d pim_test_cpu_serial | grep -E "vex|xmm|ymm|zmm" && \
		(echo "❌ ERROR: SIMD instructions found in serial binary!" && exit 1) || \
		echo "✅ Serial binary is clean (no SIMD instructions)"

clean:
	rm -f $(OBJS) $(GPU_OBJS) $(BINS)
	rm -rf $(BUILD_DIR)
