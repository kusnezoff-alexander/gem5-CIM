# NOTE: only x86 tested so far
ARCH = x86

# Tools
CXX      := g++
AR       := ar
ARFLAGS  := rcs

# Directories
BIN_DIR   := ../bin
BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj
LIB_DIR   := $(BUILD_DIR)/lib

# Flags
CXXFLAGS  := -Wall -Wextra -O2 -std=c++20
CXXFLAGS  += -I/home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix/include
TESTFLAGS := -std=c++20

# Library
LIB       := $(LIB_DIR)/libpim.a
CORE_OBJ  := $(OBJ_DIR)/pim_core.o

# Program Objects
HELLO_WORLD_OBJ:= $(OBJ_DIR)/hello_world.o
FULL_OBJ       := $(OBJ_DIR)/pim_full_program.o
TEST_OBJ       := $(OBJ_DIR)/pim_test_primitives.o
PIMMALLOC_OBJ  := $(OBJ_DIR)/pim_test_pimmalloc.o
WORKLOADS_OBJ  := $(OBJ_DIR)/pim_workloads.o
KNN_OBJ        := $(OBJ_DIR)/combined_knn.o
KNN_CPU_OBJ    := $(OBJ_DIR)/combined_knn_cpu.o
KNN_GPU_OBJ    := $(OBJ_DIR)/combined_knn_gpu.o
AXPY_OBJ       := $(OBJ_DIR)/pim_axpy.o
AXPY_CPU_OBJ   := $(OBJ_DIR)/pim_axpy_cpu.o
AXPY_GPU_OBJ   := $(OBJ_DIR)/pim_axpy_gpu.o

# Binaries
HELLO_WORLD_PROG:= $(BIN_DIR)/hello_world
FULL_PROG       := $(BIN_DIR)/pim_full_program
TEST_PROG       := $(BIN_DIR)/pim_test_primitives
PIMMALLOC_PROG  := $(BIN_DIR)/pim_test_pimmalloc
WORKLOADS_PROG  := $(BIN_DIR)/pim_workloads
KNN_PROG        := $(BIN_DIR)/combined_knn
KNN_CPU_PROG    := $(BIN_DIR)/combined_knn_cpu
KNN_GPU_PROG    := $(BIN_DIR)/combined_knn_gpu
AXPY_PROG       := $(BIN_DIR)/pim_axpy
AXPY_CPU_PROG   := $(BIN_DIR)/pim_axpy_cpu
AXPY_GPU_PROG   := $(BIN_DIR)/pim_axpy_gpu

# Selectable test (TEST=primitives | pimmalloc | workloads | knn | axpy)
TEST ?= all

TEST_helloworld := $(HELLO_WORLD_PROG)
TEST_primitives := $(TEST_PROG)
TEST_pimmalloc  := $(PIMMALLOC_PROG)
TEST_workloads  := $(WORKLOADS_PROG)
TEST_knn        := $(KNN_PROG)
TEST_axpy       := $(AXPY_PROG)
TEST_all        := $(HELLO_WORLD_PROG) $(TEST_PROG) $(PIMMALLOC_PROG) $(WORKLOADS_PROG) $(KNN_PROG) $(AXPY_PROG)

VALID_TESTS := all primitives pimmalloc workloads knn axpy
ifeq ($(filter $(TEST),$(VALID_TESTS)),)
$(error Invalid TEST='$(TEST)'. Valid values: $(VALID_TESTS))
endif

# Default targets
all: $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR) $(LIB) $(FULL_PROG) $(KNN_PROG) $(KNN_CPU_PROG)

knn_all: $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR) $(LIB) $(KNN_PROG) $(KNN_CPU_PROG) $(KNN_GPU_PROG)

test: $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR) $(LIB) $(TEST_$(TEST))
	@echo "Built test(s): $(TEST)"

# Create directories
$(BIN_DIR) $(OBJ_DIR) $(LIB_DIR):
	mkdir -p $@

# Object files
$(OBJ_DIR)/hello_world.o: hello_world.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(TESTFLAGS) -c $< -o $@

$(OBJ_DIR)/pim_core.o: pim_core.cpp pim_core.h | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/pim_full_program.o: pim_full_program.cpp pim_core.h | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/pim_test_primitives.o: pim_test_primitives.cpp pim_core.h | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(TESTFLAGS) -c $< -o $@

$(OBJ_DIR)/pim_test_pimmalloc.o: pim_test_pimmalloc.cpp pim_core.h | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(TESTFLAGS) -c $< -o $@

$(OBJ_DIR)/pim_workloads.o: pim_workloads.cpp pim_core.h | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(TESTFLAGS) -c $< -o $@

$(OBJ_DIR)/combined_knn.o: combined_knn.cpp pim_core.h | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(TESTFLAGS) -c $< -o $@

$(OBJ_DIR)/combined_knn_cpu.o: combined_knn_cpu.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(TESTFLAGS) -c $< -o $@

$(OBJ_DIR)/combined_knn_gpu.o: combined_knn_gpu.cpp | $(OBJ_DIR)
	hipcc $(TESTFLAGS) -c $< -o $@

$(OBJ_DIR)/pim_axpy.o: pim_axpy.cpp pim_core.h | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(TESTFLAGS) -c $< -o $@

$(OBJ_DIR)/pim_axpy_cpu.o: pim_axpy_cpu.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(TESTFLAGS) -c $< -o $@

$(OBJ_DIR)/pim_axpy_gpu.o: pim_axpy_gpu.cpp | $(OBJ_DIR)
	hipcc $(TESTFLAGS) -c $< -o $@

# Static library
$(LIB): $(CORE_OBJ) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^

# Link executables
$(HELLO_WORLD_PROG): $(HELLO_WORLD_OBJ) $(LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -L$(LIB_DIR) -lpim -o $@

$(FULL_PROG): $(FULL_OBJ) $(LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -L$(LIB_DIR) -lpim -o $@

$(TEST_PROG): $(TEST_OBJ) $(LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -L$(LIB_DIR) -lpim -L/home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix/util/m5/build/x86/out -lm5 -o $@

$(PIMMALLOC_PROG): $(PIMMALLOC_OBJ) $(LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -L$(LIB_DIR) -lpim -o $@

$(WORKLOADS_PROG): $(WORKLOADS_OBJ) $(LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -L$(LIB_DIR) -lpim -o $@

$(KNN_PROG): $(KNN_OBJ) $(LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -L$(LIB_DIR) -lpim -o $@

$(KNN_CPU_PROG): $(KNN_CPU_OBJ) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

$(KNN_GPU_PROG): $(KNN_GPU_OBJ) | $(BIN_DIR)
	hipcc $< -o $@

$(AXPY_PROG): $(AXPY_OBJ) $(LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -L$(LIB_DIR) -lpim -L/home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix/util/m5/build/x86/out -lm5 -o $@

$(AXPY_CPU_PROG): $(AXPY_CPU_OBJ) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -L/home/alex/Documents/Studium/Sem7/Grosser_Beleg_inf_d_950/gem5-CIM-fix/util/m5/build/x86/out -lm5 -o $@

$(AXPY_GPU_PROG): $(AXPY_GPU_OBJ) | $(BIN_DIR)
	hipcc $< -o $@

# Cleanup
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(BIN_DIR)/*
