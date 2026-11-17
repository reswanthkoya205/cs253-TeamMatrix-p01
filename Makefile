# CS253 / MA253 Lab Project 1 - Team Matrix
CXX ?= g++
CXXFLAGS ?= -std=c++17 -O3 -Wall -Wextra
CPPFLAGS += -Iinclude -Isrc
LDFLAGS ?=
LDLIBS ?=
.DEFAULT_GOAL := all
BUILD_DIR := build
BIN_DIR := bin
RAW_DIR := results/raw
FIG_DIR := results/figures

CORE := src/structA/struct_a.cpp src/structB/struct_b.cpp app/io.cpp app/schedule.cpp
SUPPORT := adversary/generators.cpp oracle/oracle.cpp oracle/validators.cpp
TEST_SRCS := $(wildcard tests/test_*.cpp) oracle/differential_test.cpp
BENCH_SRCS := bench/benchmark_structs.cpp bench/benchmark_incremental.cpp
APP_BIN := $(BIN_DIR)/app
TEST_BINS := $(addprefix $(BIN_DIR)/,$(notdir $(TEST_SRCS:.cpp=)))
BENCH_BINS := $(addprefix $(BIN_DIR)/,$(notdir $(BENCH_SRCS:.cpp=)))
obj = $(addprefix $(BUILD_DIR)/,$(1:.cpp=.o))
ALL_OBJS := $(call obj,$(CORE) $(SUPPORT) $(TEST_SRCS) $(BENCH_SRCS) app/main.cpp)

.PHONY: all app tests test bench benchmarks clean directories help
all: directories app tests benchmarks
app: $(APP_BIN)
tests: $(TEST_BINS)
benchmarks: $(BENCH_BINS)
directories:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR) $(RAW_DIR) $(FIG_DIR)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(APP_BIN): $(call obj,app/main.cpp $(CORE)) | directories
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BIN_DIR)/test_%: $(BUILD_DIR)/tests/test_%.o $(call obj,$(CORE) $(SUPPORT)) | directories
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BIN_DIR)/differential_test: $(call obj,oracle/differential_test.cpp $(CORE) $(SUPPORT)) | directories
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BIN_DIR)/benchmark_%: $(BUILD_DIR)/bench/benchmark_%.o $(call obj,$(CORE) $(SUPPORT)) | directories
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

.SECONDARY: $(ALL_OBJS)
-include $(ALL_OBJS:.o=.d)

test: tests
	@set -e; for t in $(TEST_BINS); do echo "[Testing] $$t"; ./$$t; done
bench: benchmarks
	./$(BIN_DIR)/benchmark_structs
	./$(BIN_DIR)/benchmark_incremental
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
help:
	@echo 'make             Build application, tests and benchmarks'
	@echo 'make app         Build bin/app'
	@echo 'make tests       Build all test executables'
	@echo 'make test        Build and run all tests'
	@echo 'make benchmarks  Build benchmark executables'
	@echo 'make bench       Build and run benchmarks'
	@echo 'make clean       Remove build and bin directories'
