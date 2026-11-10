#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <cmath>
#include "graph.hpp"
#include "generators.hpp"
#include "counters.hpp"
#include "interface.hpp"

using namespace std::chrono;

struct Stats {
    double mean;
    double sd;
    long long avgOps;
};

Stats compute_stats(const std::vector<double>& times, const std::vector<long long>& ops) {
    double sum = 0.0;
    long long ops_sum = 0;
    for (size_t i = 0; i < times.size(); ++i) {
        sum += times[i];
        ops_sum += ops[i];
    }
    double mean = sum / times.size();
    double variance = 0.0;
    for (double t : times) {
        variance += (t - mean) * (t - mean);
    }
    double sd = times.size() > 1 ? std::sqrt(variance / (times.size() - 1)) : 0.0;
    return {mean, sd, ops_sum / static_cast<long long>(ops.size())};
}

int main() {
    uint64_t seed = 20260928;
    std::cout << "[Bench] Running Structure Comparison Benchmark..." << std::endl;

    std::ofstream out("results/raw/struct_comparison.csv");
    out << "V,E,Skew,Structure,MeanTimeMs,SdTimeMs,AvgOpsScanned,Seed\n";

    std::vector<int> vertex_sizes = {1000, 3000, 5000};
    std::vector<double> skews = {0.0, 1.0, 2.0};

    for (int V : vertex_sizes) {
        int M = V * 2; 
        for (double skew : skews) {
            Graph g = genRandomDAG(V, M, skew, seed);

            // 1. Structure A: FIFO Queue (lexSmallest = false)
            {
                std::vector<double> times;
                std::vector<long long> ops;
                for (int run = 0; run < 5; ++run) {
                    Counters c;
                    c.reset();
                    auto start = high_resolution_clock::now();
                    Result r = runStructureA(g, false, &c);
                    auto end = high_resolution_clock::now();
                    double ms = duration_cast<microseconds>(end - start).count() / 1000.0;
                    times.push_back(ms);
                    ops.push_back(c.edgesScanned + c.verticesVisited);
                }
                Stats s = compute_stats(times, ops);
                out << V << "," << M << "," << skew << ",StructureA_FIFO," << s.mean << "," << s.sd << "," << s.avgOps << "," << seed << "\n";
            }

            // 2. Structure A: Min-Heap (lexSmallest = true)
            {
                std::vector<double> times;
                std::vector<long long> ops;
                for (int run = 0; run < 5; ++run) {
                    Counters c;
                    c.reset();
                    auto start = high_resolution_clock::now();
                    Result r = runStructureA(g, true, &c);
                    auto end = high_resolution_clock::now();
                    double ms = duration_cast<microseconds>(end - start).count() / 1000.0;
                    times.push_back(ms);
                    ops.push_back(c.edgesScanned + c.verticesVisited + c.heapPushes);
                }
                Stats s = compute_stats(times, ops);
                out << V << "," << M << "," << skew << ",StructureA_Heap," << s.mean << "," << s.sd << "," << s.avgOps << "," << seed << "\n";
            }

            // 3. Structure B: DFS Finish-Time
            {
                std::vector<double> times;
                std::vector<long long> ops;
                for (int run = 0; run < 5; ++run) {
                    Counters c;
                    c.reset();
                    auto start = high_resolution_clock::now();
                    Result r = runStructureB(g, nullptr, &c);
                    auto end = high_resolution_clock::now();
                    double ms = duration_cast<microseconds>(end - start).count() / 1000.0;
                    times.push_back(ms);
                    ops.push_back(c.edgesScanned + c.verticesVisited);
                }
                Stats s = compute_stats(times, ops);
                out << V << "," << M << "," << skew << ",StructureB_DFS," << s.mean << "," << s.sd << "," << s.avgOps << "," << seed << "\n";
            }
        }
    }
    out.close();
    std::cout << "[Bench] Saved results to results/raw/struct_comparison.csv" << std::endl;
    return 0;
}