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

# Default values for configurable parameters
N_ELEMS ?= 3000
N_RUNS ?= 10
BITWIDTH ?= 16

# Common flags for all targets
CXXFLAGS_COMMON := -Wall -Wextra -std=c++20
CXXFLAGS_COMMON += -I$(GEM5_DIR)/include
CXXFLAGS_COMMON += -I$(SRC_DIR)

# SIMD: SSE4.1 optimization (gem5 compatible)
CXXFLAGS_SIMD := $(CXXFLAGS_COMMON) -O2 -msse4.1

# CIM: Similar to serial, no vectorization
CXXFLAGS_CIM := $(CXXFLAGS_COMMON) -O2 -fno-tree-vectorize -mno-avx
CXXFLAGS_CIM += -DN_ELEMS=$(N_ELEMS) -DBITWIDTH=$(BITWIDTH)

# CIM with configurable params (for pim_axpy, combined_knn)
CXXFLAGS_CIM_PARAM := $(CXXFLAGS_COMMON) -O2 -fno-tree-vectorize -mno-avx
CXXFLAGS_CIM_PARAM += -DN_ELEMS=$(N_ELEMS) -DN_RUNS=$(N_RUNS) -DBITWIDTH=$(BITWIDTH)

# CPU SIMD with configurable params (for pim_axpy_cpu) - let compiler auto-vectorize
CXXFLAGS_CPU_SIMD_PARAM := $(CXXFLAGS_COMMON) -O3
CXXFLAGS_CPU_SIMD_PARAM += -DN_ELEMS=$(N_ELEMS) -DN_RUNS=$(N_RUNS) -DBITWIDTH=$(BITWIDTH)

# GPU: HIP compilation flags
HIPFLAGS := -O2 -std=c++17
HIPFLAGS += -mno-avx -mno-sse4.2 -mno-sse4.1
HIPFLAGS += -I$(GEM5_DIR)/include
HIPFLAGS += -I$(SRC_DIR)
HIPFLAGS += -DN_ELEMS=$(N_ELEMS) -DN_RUNS=$(N_RUNS)

# Library: Use serial flags (no SIMD)
CXXFLAGS_LIB := $(CXXFLAGS_COMMON) -O2 -fno-tree-vectorize

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin

LIB := $(SRC_DIR)/build/lib/libpim.a
CORE_OBJ := $(OBJ_DIR)/pim_core.o

# Source files
SRCS := pim_test_cpu.cpp pim_test_cim.cpp pim_axpy.cpp pim_axpy_cpu.cpp pim_axpy_gpu.cpp combined_knn.cpp combined_knn_cpu.cpp combined_knn_gpu.cpp

# Object files in build/obj/
OBJS := $(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))

# Binary files in build/bin/
BINS := $(addprefix $(BIN_DIR)/, pim_test_cpu pim_test_cim pim_test_gpu pim_axpy pim_axpy_cpu pim_axpy_gpu combined_knn combined_knn_cpu combined_knn_gpu)

GPU_OBJS := $(OBJ_DIR)/pim_test_gpu.o $(OBJ_DIR)/gpu_kernels.hip.o

.PHONY: all clean lib verify-serial gpu pim_axpy pim_axpy_cpu pim_axpy_gpu combined_knn combined_knn_cpu combined_knn_gpu pim_test_cpu pim_test_cim pim_test_gpu

all: lib $(BINS)

gpu: $(BIN_DIR)/pim_test_gpu

# Phony targets for convenience
pim_axpy: $(BIN_DIR)/pim_axpy
pim_axpy_cpu: $(BIN_DIR)/pim_axpy_cpu
pim_axpy_gpu: $(BIN_DIR)/pim_axpy_gpu
combined_knn: $(BIN_DIR)/combined_knn
combined_knn_cpu: $(BIN_DIR)/combined_knn_cpu
combined_knn_gpu: $(BIN_DIR)/combined_knn_gpu
pim_test_cpu: $(BIN_DIR)/pim_test_cpu
pim_test_cim: $(BIN_DIR)/pim_test_cim
pim_test_gpu: $(BIN_DIR)/pim_test_gpu

# Create directories
$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

$(dir $(LIB)):
	mkdir -p $@

# Linking rules
$(BIN_DIR)/pim_axpy: $(OBJ_DIR)/pim_axpy.o $(LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS_CIM_PARAM) $< -L$(SRC_DIR)/build/lib -lpim $(M5_LIB) -o $@

$(BIN_DIR)/pim_axpy_cpu: $(OBJ_DIR)/pim_axpy_cpu.o | $(BIN_DIR)
	$(CXX) $(CXXFLAGS_CPU_SIMD_PARAM) $< $(M5_LIB) -o $@

$(BIN_DIR)/pim_axpy_gpu: $(OBJ_DIR)/pim_axpy_gpu.o | $(BIN_DIR)
	$(HIPCC) $(HIPFLAGS) $< $(M5_LIB) -o $@

$(BIN_DIR)/combined_knn: $(OBJ_DIR)/combined_knn.o $(LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS_CIM_PARAM) $< -L$(SRC_DIR)/build/lib -lpim $(M5_LIB) -o $@

$(BIN_DIR)/combined_knn_cpu: $(OBJ_DIR)/combined_knn_cpu.o | $(BIN_DIR)
	$(CXX) $(CXXFLAGS_CIM_PARAM) $< $(M5_LIB) -o $@

$(BIN_DIR)/combined_knn_gpu: $(OBJ_DIR)/combined_knn_gpu.o | $(BIN_DIR)
	$(HIPCC) $(HIPFLAGS) $< $(M5_LIB) -o $@

$(BIN_DIR)/pim_test_cpu: $(OBJ_DIR)/pim_test_cpu.o $(LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS_CPU_SIMD_PARAM) $< -L$(SRC_DIR)/build/lib -lpim $(M5_LIB) -o $@

$(BIN_DIR)/pim_test_cim: $(OBJ_DIR)/pim_test_cim.o $(LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS_CIM) $< -L$(SRC_DIR)/build/lib -lpim $(M5_LIB) -o $@

$(BIN_DIR)/pim_test_gpu: $(GPU_OBJS) | $(BIN_DIR)
	$(HIPCC) $(HIPFLAGS) $^ $(M5_LIB) -o $@

lib: $(LIB)

$(LIB): $(CORE_OBJ) | $(dir $(LIB))
	$(AR) $(ARFLAGS) $@ $^

$(OBJ_DIR)/pim_core.o: $(SRC_DIR)/libpim/pim_core.cpp $(SRC_DIR)/libpim/pim_core.h | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS_LIB) -c $< -o $@

# Compilation rules - output to build/obj/
$(OBJ_DIR)/pim_test_cpu.o: pim_test_cpu.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS_CPU_SIMD_PARAM) -c $< -o $@

$(OBJ_DIR)/pim_test_cim.o: pim_test_cim.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS_CIM) -c $< -o $@

$(OBJ_DIR)/pim_axpy.o: pim_axpy.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS_CIM_PARAM) -c $< -o $@

$(OBJ_DIR)/pim_axpy_cpu.o: pim_axpy_cpu.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS_CPU_SIMD_PARAM) -c $< -o $@

$(OBJ_DIR)/pim_axpy_gpu.o: pim_axpy_gpu.cpp | $(OBJ_DIR)
	$(HIPCC) $(HIPFLAGS) -c $< -o $@

$(OBJ_DIR)/combined_knn.o: combined_knn.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS_CIM_PARAM) -c $< -o $@

$(OBJ_DIR)/combined_knn_cpu.o: combined_knn_cpu.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS_CIM_PARAM) -c $< -o $@

$(OBJ_DIR)/combined_knn_gpu.o: combined_knn_gpu.cpp | $(OBJ_DIR)
	$(HIPCC) $(HIPFLAGS) -c $< -o $@

$(OBJ_DIR)/pim_test_gpu.o: pim_test_gpu.cpp | $(OBJ_DIR)
	$(HIPCC) $(HIPFLAGS) -c $< -o $@

$(OBJ_DIR)/gpu_kernels.hip.o: gpu_kernels.hip | $(OBJ_DIR)
	$(HIPCC) $(HIPFLAGS) -c $< -o $@

verify-serial:
	@echo "Checking for SIMD instructions in serial binary..."
	@objdump -d $(BIN_DIR)/pim_test_cpu | grep -E "vex|xmm|ymm|zmm" && \
		(echo "❌ ERROR: SIMD instructions found in serial binary!" && exit 1) || \
		echo "✅ Serial binary is clean (no SIMD instructions)"

clean:
	rm -rf $(BUILD_DIR)
