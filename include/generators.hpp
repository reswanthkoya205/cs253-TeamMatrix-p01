
#pragma once

// Input generators. Owner: Adversary.
// Depend only on Graph and a seed; never call a structure under study.

#include <cstdint>
#include <utility>
#include <vector>
#include <string>

#include "graph.hpp"

// Stores an initial DAG and the edges to be inserted.
struct InsertionSequence {
    Graph initialGraph;
    std::vector<std::pair<int, int>> edgesToInsert;
};

// DAG with exponentially many valid topological orders.
Graph genExponentialDAG(
    int n,
    uint64_t seed,
    const std::string& mode,
    int width
);

// Cycle of length cycleLen reachable through a chain.
Graph genHiddenCycleBehindChain(
    int n,
    int cycleLen,
    uint64_t seed,
    int headLen
);

// Worst-case incremental insertions.
InsertionSequence genWorstInsertions(
    int n,
    uint64_t seed,
    int chainLen
);

// Random DAG with controllable in-degree skew.
Graph genRandomDAG(
    int n,
    int m,
    double skew,
    uint64_t seed
);

// Random graph that may contain cycles.
Graph genRandomGraph(
    int n,
    int m,
    uint64_t seed
);
