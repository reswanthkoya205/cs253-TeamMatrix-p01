#pragma once
// Input generators. Owner: Adversary. Implemented in adversary/.
// Depend only on Graph and a seed; never call a structure under study.
#include <cstdint>
#include <utility>
#include <vector>
#include "graph.hpp"

// DAG with exponentially many valid topological orders (e.g. layers of independent vertices).
Graph genExponentialDAG(int n, uint64_t seed);

// Cycle of length cycleLen reachable only through a long chain, so a naive
// "report the leftover vertices" method returns chain vertices.
Graph genHiddenCycleBehindChain(int n, int cycleLen, uint64_t seed);

// Insertion sequence for a DAG on n vertices that forces Theta(V) reorder per insertion.
// Returns the initial graph in `initial` and the edges to insert in order.
std::vector<std::pair<int, int>> genWorstInsertions(int n, uint64_t seed, Graph& initial);

// Random DAG with controllable in-degree skew (skew = 0 uniform; larger = more skewed).
Graph genRandomDAG(int n, long long m, double skew, uint64_t seed);

// Random graph that may contain cycles (for differential tests).
Graph genRandomGraph(int n, long long m, uint64_t seed);
