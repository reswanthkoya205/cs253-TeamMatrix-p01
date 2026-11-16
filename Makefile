# CS253 / MA253 Lab Project 1 - Team Matrix
# Linux and MSYS2 GNU Make (including invocation from PowerShell)
SHELL := /bin/sh
.SHELLFLAGS := -c
CXX ?= g++
CXXFLAGS ?= -std=c++17 -O3 -Wall -Wextra
CPPFLAGS += -Iinclude -Isrc
LDFLAGS ?=
LDLIBS ?=
PYTHON ?= python
.DEFAULT_GOAL := all
BUILD_DIR := build
BIN_DIR := bin
RAW_DIR := results/raw
FIG_DIR := results/figures

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

CORE := src/structA/struct_a.cpp src/structB/struct_b.cpp app/io.cpp app/schedule.cpp
SUPPORT := adversary/generators.cpp oracle/oracle.cpp oracle/validators.cpp
TEST_SRCS := $(wildcard tests/test_*.cpp) oracle/differential_test.cpp
BENCH_SRCS := bench/benchmark_structs.cpp bench/benchmark_incremental.cpp
APP_BIN := $(BIN_DIR)/app$(EXE)
TEST_BINS := $(addprefix $(BIN_DIR)/,$(notdir $(TEST_SRCS:.cpp=$(EXE))))
BENCH_BINS := $(addprefix $(BIN_DIR)/,$(notdir $(BENCH_SRCS:.cpp=$(EXE))))
obj = $(addprefix $(BUILD_DIR)/,$(1:.cpp=.o))
ALL_OBJS := $(call obj,$(CORE) $(SUPPORT) $(TEST_SRCS) $(BENCH_SRCS) app/main.cpp)

.PHONY: all app tests test test-build bench benchmarks plots clean directories help
all: directories app test-build benchmarks
app: $(APP_BIN)
test-build: $(TEST_BINS)
tests: test
benchmarks: $(BENCH_BINS)
directories:
	@/usr/bin/mkdir -p "$(BUILD_DIR)" "$(BIN_DIR)" "$(RAW_DIR)" "$(FIG_DIR)"

$(BUILD_DIR)/%.o: %.cpp
	@/usr/bin/mkdir -p "$(@D)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(APP_BIN): $(call obj,app/main.cpp $(CORE)) | directories
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BIN_DIR)/test_%$(EXE): $(BUILD_DIR)/tests/test_%.o $(call obj,$(CORE) $(SUPPORT)) | directories
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BIN_DIR)/differential_test$(EXE): $(call obj,oracle/differential_test.cpp $(CORE) $(SUPPORT)) | directories
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BIN_DIR)/benchmark_%$(EXE): $(BUILD_DIR)/bench/benchmark_%.o $(call obj,$(CORE) $(SUPPORT)) | directories
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

.SECONDARY: $(ALL_OBJS)
-include $(ALL_OBJS:.o=.d)

test: test-build
	@for t in $(TEST_BINS); do if ./$$t; then echo "$${t##*/}: PASS"; else echo "$${t##*/}: FAIL"; exit 1; fi; done
	@echo "All tests passed."
bench: benchmarks
	./$(BIN_DIR)/benchmark_structs$(EXE)
	./$(BIN_DIR)/benchmark_incremental$(EXE)
	$(PYTHON) scripts/generate_figures.py
plots: | directories
	$(PYTHON) scripts/generate_figures.py
clean:
	/usr/bin/rm -rf -- "$(BUILD_DIR)" "$(BIN_DIR)"
help:
	@echo "make             Build application, tests and benchmarks"
	@echo "make app         Build application"
	@echo "make tests       Build and run all tests"
	@echo "make test        Build and run all tests"
	@echo "make test-build  Build test executables only"
	@echo "make benchmarks  Build benchmark executables"
	@echo "make bench       Run benchmarks and regenerate figures"
	@echo "make plots       Regenerate figures from existing CSV files"
	@echo "make clean       Remove build and bin directories"
