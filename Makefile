# CS253 TeamMatrix - Windows PowerShell + GNU Make + g++ (C++17)
#
# Run these commands from the project root in the VS Code PowerShell terminal:
#   make clean
#   make tests
#
# IMPORTANT:
# - No echo commands are used. Headings are printed with GNU Make's $(info ...).
# - cmd.exe is selected for Windows commands and executable launching.
# - Compiler commands are hidden to keep the test report readable.
# - A failed test executable stops the build; the final PASS banner is printed
#   only if every test group succeeds.

SHELL := cmd.exe
.SHELLFLAGS := /C

CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude
BIN_DIR  := bin

ORACLE_SRC    := oracle/oracle.cpp
VALIDATOR_SRC := oracle/validators.cpp
STRUCT_A_SRC  := src/structA/struct_a.cpp
STRUCT_B_SRC  := src/structB/struct_b.cpp
SCHEDULE_SRC  := app/schedule.cpp
GENERATOR_SRC := adversary/generators.cpp

COMMON_TEST_SRC := $(ORACLE_SRC) $(VALIDATOR_SRC) \
                   $(STRUCT_A_SRC) $(STRUCT_B_SRC) \
                   $(SCHEDULE_SRC) $(GENERATOR_SRC)

.PHONY: all clean unit-test structure-test differential edge-test \
        incremental-test adversarial-test large-test tests test

all: tests

# Create bin/ if it is not already present.
$(BIN_DIR):
	@if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"

# ============================================================================
# BUILD THE TEST EXECUTABLES
# Compilation is intentionally quiet. Any compiler error is still displayed.
# ============================================================================

$(BIN_DIR)/test_oracle.exe: tests/test_oracle.cpp $(ORACLE_SRC) | $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DIR)/test_validators.exe: tests/test_validators.cpp $(ORACLE_SRC) $(VALIDATOR_SRC) | $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DIR)/test_structures.exe: tests/test_structures.cpp $(COMMON_TEST_SRC) | $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DIR)/differential_test.exe: oracle/differential_test.cpp $(COMMON_TEST_SRC) | $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DIR)/test_edge_cases.exe: tests/test_edge_cases.cpp $(COMMON_TEST_SRC) | $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DIR)/test_incremental.exe: tests/test_incremental.cpp $(COMMON_TEST_SRC) | $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DIR)/test_adversarial.exe: tests/test_adversarial.cpp $(COMMON_TEST_SRC) | $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DIR)/test_large_scale.exe: tests/test_large_scale.cpp $(COMMON_TEST_SRC) | $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) $^ -o $@

# ============================================================================
# [1/7] ORACLE AND VALIDATOR TESTS
# ============================================================================

unit-test: $(BIN_DIR)/test_oracle.exe $(BIN_DIR)/test_validators.exe
	$(info )
	$(info [1/7] ORACLE AND VALIDATOR TESTS)
	$(info --------------------------------)
	$(info Oracle: checking reference results for graph and scheduling operations.)
	$(info Validators: checking topological orders, cycle reports and schedule invariants.)
	@.\bin\test_oracle.exe
	@.\bin\test_validators.exe
	$(info [PASS] Oracle and validator tests completed successfully.)

# ============================================================================
# [2/7] STRUCTURE A AND STRUCTURE B TESTS
# ============================================================================

structure-test: $(BIN_DIR)/test_structures.exe
	$(info )
	$(info [2/7] STRUCTURE A AND STRUCTURE B TESTS)
	$(info ----------------------------------------)
	$(info Checking both implementations on representative directed graphs.)
	$(info Checking their reported orders and computed scheduling information.)
	@.\bin\test_structures.exe
	$(info [PASS] Structure A and Structure B tests completed successfully.)

# ============================================================================
# [3/7] STATIC AND INCREMENTAL DIFFERENTIAL TESTS
# ============================================================================

differential: $(BIN_DIR)/differential_test.exe
	$(info )
	$(info [3/7] STATIC AND INCREMENTAL DIFFERENTIAL TESTS)
	$(info ------------------------------------------------)
	$(info Comparing Structure A and Structure B against the independent Oracle.)
	$(info Checking generated graph instances and their topological results.)
	$(info Checking incremental edge insertions and the maintained vertex order.)
	@.\bin\differential_test.exe
	$(info [PASS] Static and incremental differential tests completed successfully.)

# ============================================================================
# [4/7] EDGE-CASE REGRESSION TESTS
# ============================================================================

edge-test: $(BIN_DIR)/test_edge_cases.exe
	$(info )
	$(info [4/7] EDGE-CASE REGRESSION TESTS)
	$(info --------------------------------)
	$(info Checking boundary and structurally unusual graph inputs.)
	$(info Checking empty graphs, isolated vertices and disconnected components.)
	$(info Checking self-loops, duplicate dependencies and cycle handling.)
	$(info Checking schedule values and structural invariants on edge cases.)
	@.\bin\test_edge_cases.exe
	$(info [PASS] Edge-case regression tests completed successfully.)

# ============================================================================
# [5/7] INCREMENTAL INSERTION AND CYCLE-REJECTION TESTS
# ============================================================================

incremental-test: $(BIN_DIR)/test_incremental.exe
	$(info )
	$(info [5/7] INCREMENTAL INSERTION AND CYCLE-REJECTION TESTS)
	$(info -------------------------------------------------------)
	$(info Applying new dependency edges to graphs with an existing valid order.)
	$(info Checking accepted insertions and the resulting topological order.)
	$(info Checking that cycle-producing insertions are rejected.)
	$(info Checking that rejected insertions do not corrupt the graph or its order.)
	$(info Comparing updated states with the Oracle and invariant checks.)
	@.\bin\test_incremental.exe
	$(info [PASS] Incremental insertion tests completed successfully.)

# ============================================================================
# [6/7] ADVERSARIAL GRAPH AND INSERTION TESTS
# ============================================================================

adversarial-test: $(BIN_DIR)/test_adversarial.exe
	$(info )
	$(info [6/7] ADVERSARIAL GRAPH AND INSERTION TESTS)
	$(info ------------------------------------------------)
	$(info Testing DAGs designed to have many valid topological orders.)
	$(info Testing cycles hidden behind long dependency chains.)
	$(info Testing insertion sequences designed to force substantial reordering.)
	$(info Checking cycle extraction, returned orders and invariant preservation.)
	$(info Comparing adversarial results with the Oracle where applicable.)
	@.\bin\test_adversarial.exe
	$(info [PASS] Adversarial graph and insertion tests completed successfully.)

# ============================================================================
# [7/7] LARGE-SCALE AND EXTENDED DIFFERENTIAL TESTS
# ============================================================================

large-test: $(BIN_DIR)/test_large_scale.exe
	$(info )
	$(info [7/7] LARGE-SCALE AND EXTENDED DIFFERENTIAL TESTS)
	$(info --------------------------------------------------)
	$(info Running extended generated workloads with reproducible test seeds.)
	$(info Checking Oracle-backed differential results on manageable graphs.)
	$(info Exercising the implementations on a large graph workload.)
	$(info Checking topological-order and scheduling invariants at larger scale.)
	@.\bin\test_large_scale.exe
	$(info [PASS] Large-scale and extended differential tests completed successfully.)

# ============================================================================
# RUN THE COMPLETE TEST SUITE
# Make executes these groups in order and stops immediately on a failure.
# ============================================================================

tests: unit-test structure-test differential edge-test incremental-test adversarial-test large-test
	$(info )
	$(info ================================================================)
	$(info                  ALL SEVEN TEST GROUPS PASSED)
	$(info ================================================================)

test: tests

# Remove generated test executables. Source files and test results are kept.
clean:
	@if exist "$(BIN_DIR)\*.exe" del /q "$(BIN_DIR)\*.exe"
	$(info Generated test executables cleaned.)
