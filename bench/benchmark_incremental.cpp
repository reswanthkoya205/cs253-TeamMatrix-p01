#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include "graph.hpp"
#include "generators.hpp"
#include "counters.hpp"
#include "interface.hpp"
#include "dynamic_order.hpp"

using namespace std::chrono;

int main() {
    uint64_t seed = 20260928;
    std::cout << "[Bench] Running Incremental Maintenance vs Recomputation Benchmark..." << std::endl;

    std::ofstream out("results/raw/incremental_comparison.csv");
    out << "InsertionIndex,IncrementalTimeMs,RecompTimeMs,AffectedRegionSize,Seed\n";

    int n = 1000;
    InsertionSequence seq = genWorstInsertions(n, seed, 50);

    Graph g_inc = seq.initialGraph;
    DynamicOrder d;
    Counters c_init;
    runStructureB(g_inc, &d, &c_init);

    Graph g_recomp = seq.initialGraph;
    int sample_interval = std::max(1, static_cast<int>(seq.edgesToInsert.size()) / 50);

    for (size_t i = 0; i < seq.edgesToInsert.size(); ++i) {
        int u = seq.edgesToInsert[i].first;
        int v = seq.edgesToInsert[i].second;

        // 1. Measure Incremental Edge Insertion using high-precision duration
        Counters c_inc;
        c_inc.reset();
        auto start_inc = high_resolution_clock::now();
        InsertResult res = insertDependencyB(g_inc, d, u, v, &c_inc);
        auto end_inc = high_resolution_clock::now();
        
        duration<double, std::milli> elapsed_inc = end_inc - start_inc;
        double inc_ms = elapsed_inc.count();

        // 2. Measure Full Recomputation (sampled periodically for performance)
        double recomp_ms = 0.0;
        if (static_cast<int>(i) % sample_interval == 0) {
            g_recomp.addEdge(u, v);
            Counters c_rec;
            c_rec.reset();
            auto start_rec = high_resolution_clock::now();
            runStructureB(g_recomp, nullptr, &c_rec);
            auto end_rec = high_resolution_clock::now();
            
            duration<double, std::milli> elapsed_rec = end_rec - start_rec;
            recomp_ms = elapsed_rec.count();
        }

        out << (i + 1) << "," << inc_ms << "," << recomp_ms << "," << res.affected << "," << seed << "\n";
    }

    out.close();
    std::cout << "[Bench] Saved results to results/raw/incremental_comparison.csv" << std::endl;
    return 0;
}