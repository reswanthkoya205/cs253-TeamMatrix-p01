# ==============================================================================
# CS 253 / MA 253 Lab Project 1 Makefile
# Team Matrix - Build System (Analyst + Oracle Integrated)
# ==============================================================================

# Compiler & Flags
CXX       ?= g++
CXXFLAGS  := -std=c++17 -O3 -Wall -Wextra -Iinclude -Isrc
LDFLAGS   := 

# Target Directories
BUILD_DIR := build
BIN_DIR   := bin
RAW_DIR   := results/raw
FIG_DIR   := results/figures

# Source files & Objects
ADVERSARY_SRCS := adversary/generators.cpp
STRUCT_A_SRCS  := src/structA/struct_a.cpp
STRUCT_B_SRCS  := src/structB/struct_b.cpp
APP_SRCS       := app/io.cpp app/schedule.cpp
ORACLE_SRCS    := oracle/oracle.cpp oracle/validators.cpp

# Executable Targets
BENCH_STRUCTS_BIN  := $(BIN_DIR)/benchmark_structs
BENCH_INCREM_BIN   := $(BIN_DIR)/benchmark_incremental
TEST_ORACLE_BIN    := $(BIN_DIR)/test_oracle

.PHONY: all clean test bench directories help

# Default Target (building benchmarks and oracle tests)
all: directories $(TEST_ORACLE_BIN) $(BENCH_STRUCTS_BIN) $(BENCH_INCREM_BIN)

# Ensure Required Directories Exist
directories:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR) $(RAW_DIR) $(FIG_DIR)

# ------------------------------------------------------------------------------
# Executable Build Rules
# ------------------------------------------------------------------------------

# 1. Oracle & Differential Tests (excluding differential_test.cpp main conflict)
$(TEST_ORACLE_BIN): $(ORACLE_SRCS) $(STRUCT_A_SRCS) $(STRUCT_B_SRCS) $(ADVERSARY_SRCS) $(APP_SRCS) tests/test_oracle.cpp | directories
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# 2. Analyst Benchmark 1: Structure Comparison
$(BENCH_STRUCTS_BIN): bench/benchmark_structs.cpp $(STRUCT_A_SRCS) $(STRUCT_B_SRCS) $(ADVERSARY_SRCS) $(APP_SRCS) | directories
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# 3. Analyst Benchmark 2: Incremental Maintenance
$(BENCH_INCREM_BIN): bench/benchmark_incremental.cpp $(STRUCT_A_SRCS) $(STRUCT_B_SRCS) $(ADVERSARY_SRCS) $(APP_SRCS) | directories
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# ------------------------------------------------------------------------------
# Helper Commands
# ------------------------------------------------------------------------------

# Run Unit & Differential Tests (Oracle)
test: $(TEST_ORACLE_BIN)
	@echo "[Testing] Running Oracle Differential Tests..."
	./$(TEST_ORACLE_BIN)

# Run Benchmarks
bench: $(BENCH_STRUCTS_BIN) $(BENCH_INCREM_BIN)
	@echo "[Benchmarking] Running Analyst Benchmarks..."
	./$(BENCH_STRUCTS_BIN)
	./$(BENCH_INCREM_BIN)

# Clean Build Outputs
clean:
	@echo "[Clean] Removing build artifacts and binaries..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "[Clean] Done."

# Help Message
help:
	@echo "Available Makefile targets:"
	@echo "  make          - Build all test and benchmark binaries"
	@echo "  make test     - Build and execute oracle differential tests"
	@echo "  make bench    - Build and execute analyst benchmarks"
	@echo "  make clean    - Remove binary and build directories"