#!/usr/bin/env bash
set -e

echo "--------------------------------------------------------"
echo " CS253/MA253 Lab Project 1: Analyst Automation Runner"
echo "--------------------------------------------------------"

# 1. Prepare workspace directories
mkdir -p results/raw results/figures build bin

# 2. Build binaries using Makefile
echo "[1/4] Building Project..."
make clean && make -j4

# 3. Run correctness unit tests & oracle differential tests
echo "[2/4] Running Unit & Differential Tests..."
if [ -f "./bin/test_oracle" ]; then
    ./bin/test_oracle
fi

# 4. Run Benchmark Harnesses and save raw logs
echo "[3/4] Running Benchmarks (logging to results/raw/)..."
if [ -f "./bin/benchmark_structs" ] && [ -f "./bin/benchmark_incremental" ]; then
    ./bin/benchmark_structs
    ./bin/benchmark_incremental
else
    echo "Error: Benchmark binaries not found in bin/"
    exit 1
fi

# 5. Generate Figures
echo "[4/4] Generating Analysis Figures (saving to results/figures/)..."
python3 scripts/generate_figures.py

echo "--------------------------------------------------------"
echo " Execution Complete! All raw data and figures updated."
echo "--------------------------------------------------------"