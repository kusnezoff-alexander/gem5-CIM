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
CXXFLAGS  := -Wall -Wextra -O2
TESTFLAGS := -std=c++20

# Library
LIB       := $(LIB_DIR)/libpim.a
CORE_OBJ  := $(OBJ_DIR)/pim_core.o

# Program Objects
FULL_OBJ      := $(OBJ_DIR)/pim_full_program.o
TEST_OBJ      := $(OBJ_DIR)/pim_test_primitives.o
PIMMALLOC_OBJ := $(OBJ_DIR)/pim_test_pimmalloc.o

# Binaries
FULL_PROG      := $(BIN_DIR)/pim_full_program
TEST_PROG      := $(BIN_DIR)/pim_test_primitives
PIMMALLOC_PROG := $(BIN_DIR)/pim_test_pimmalloc

# Selectable test (TEST=primitives | pimmalloc)
TEST ?= all

TEST_primitives := $(TEST_PROG)
TEST_pimmalloc  := $(PIMMALLOC_PROG)
TEST_all        := $(TEST_PROG) $(PIMMALLOC_PROG)

VALID_TESTS := all primitives pimmalloc
ifeq ($(filter $(TEST),$(VALID_TESTS)),)
$(error Invalid TEST='$(TEST)'. Valid values: $(VALID_TESTS))
endif

# Default targets
all: $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR) $(LIB) $(FULL_PROG)
test: $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR) $(LIB) $(TEST_$(TEST))
	@echo "Built test(s): $(TEST)"

# Create directories
$(BIN_DIR) $(OBJ_DIR) $(LIB_DIR):
	mkdir -p $@

# Object files
$(OBJ_DIR)/pim_core.o: pim_core.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/pim_full_program.o: pim_full_program.cpp pim_core.h | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/pim_test_primitives.o: pim_test_primitives.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(TESTFLAGS) -c $< -o $@

$(OBJ_DIR)/pim_test_pimmalloc.o: pim_test_pimmalloc.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(TESTFLAGS) -c $< -o $@

# Static library
$(LIB): $(CORE_OBJ) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^

# Link executables
$(FULL_PROG): $(FULL_OBJ) $(LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -L$(LIB_DIR) -lpim -o $@

$(TEST_PROG): $(TEST_OBJ) $(LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -L$(LIB_DIR) -lpim -o $@

$(PIMMALLOC_PROG): $(PIMMALLOC_OBJ) $(LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -L$(LIB_DIR) -lpim -o $@

# Cleanup
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(BIN_DIR)/*
