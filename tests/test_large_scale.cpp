#include <iostream>
#include <string>
#include <cstdint>
#include "generators.hpp"
#include "interface.hpp"
#include "validate.hpp"

using namespace std;

int main() {
    // Oracle-backed differential workload: 1,000 independently generated
    // 64-vertex DAGs with 1,000 dependency operations each (1,000,000 total).
    // The Oracle remains small-graph-only, consistent with its O(V^2) design.
    constexpr uint64_t BASE_SEED = 20261110;
    constexpr long long REQUIRED_OPERATIONS = 1000000;
    long long oracleDifferentialOperations = 0;
    for (int batch = 0; batch < 1000; ++batch) {
        Graph small = genRandomDAG(64, 1000, 0.0, BASE_SEED + batch);
        oracleDifferentialOperations += small.edgeCount();
        string batchWhy;
        Result expected = runOracle(small);
        Result aSmall = runStructureA(small, false, nullptr);
        Result bSmall = runStructureB(small, nullptr, nullptr);
        if (expected.hasCycle || !validateResult(small, expected, batchWhy) ||
            !validateResult(small, aSmall, batchWhy) ||
            !validateResult(small, bSmall, batchWhy) ||
            !resultsEquivalent(small, aSmall, expected, batchWhy) ||
            !resultsEquivalent(small, bSmall, expected, batchWhy)) {
            cerr << "Oracle differential batch " << batch << " failed: "
                 << batchWhy << '\n';
            return 1;
        }
    }
    if (oracleDifferentialOperations < REQUIRED_OPERATIONS) {
        cerr << "Oracle differential workload did not reach one million operations\n";
        return 1;
    }

    // Project 1 scenario: 100,000 targets and at least 1,000,000 dependency
    // edges/operations. Edges only point forward, so this is a deterministic DAG.
    constexpr int N = 100000;
    constexpr int OUT_DEGREE = 11;
    Graph g(N);

    long long operations = 0;
    for (int u = 0; u < N; ++u) {
        g.duration[u] = (u % 17);
        for (int delta = 1; delta <= OUT_DEGREE && u + delta < N; ++delta) {
            g.addEdge(u, u + delta);
            ++operations;
        }
    }
    if (operations < REQUIRED_OPERATIONS) {
        cerr << "large-scale generator did not reach one million operations\n";
        return 1;
    }

    string why;
    Result a = runStructureA(g, false, nullptr);
    DynamicOrder dynamic;
    Result b = runStructureB(g, &dynamic, nullptr);
    if (!validateResult(g, a, why) || !validateResult(g, b, why) ||
        !resultsEquivalent(g, b, a, why) || !validateDynamic(g, dynamic, why)) {
        cerr << "large-scale validation failed: " << why << '\n';
        return 1;
    }

    cout << "test_large_scale: PASS\n";
    cout << "oracle_differential_seed=" << BASE_SEED
         << " oracle_differential_operations=" << oracleDifferentialOperations << '\n';
    cout << "large_graph_vertices=" << N << " large_graph_dependency_operations=" << operations << '\n';
    cout << "Large-graph stress compares A/B and validators; the O(V^2) Oracle is used on the seeded small-graph batches.\n";
    return 0;
}
