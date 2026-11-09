#pragma once
// The common interface. Contracts live in INTERFACE.md. Owner: Architect.
#include "common.hpp"
#include "counters.hpp"
#include "dynamic_order.hpp"
#include "graph.hpp"
#include "result.hpp"

// Structure A (src/structA): Kahn with a queue; min-heap variant gives the
// lexicographically smallest order when lexSmallest is true.
// Must NOT use std::priority_queue (write your own heap).
Result runStructureA(const Graph& g, bool lexSmallest = false, Counters* c = nullptr);

// Structure B (src/structB): iterative DFS, cycle via back edge.
// If out != nullptr and the graph is acyclic, fills the DynamicOrder from the order found.
Result runStructureB(const Graph& g, DynamicOrder* out = nullptr, Counters* c = nullptr);

// Structure B, incremental: add u->v and repair the order, touching only the affected region.
// g is modified (edge appended) ONLY if no cycle results; otherwise g and d are unchanged.
InsertResult insertDependencyB(Graph& g, DynamicOrder& d, int u, int v, Counters* c = nullptr);

// Oracle (oracle/): independent slow implementation (O(V^2) selection, naive schedule).
// Must NOT call runStructureA, runStructureB, insertDependencyB or computeSchedule.
Result runOracle(const Graph& g);
