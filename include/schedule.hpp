#pragma once
// Forward/backward relaxation in topological order (derivation D3). Owner: Architect.
// Shared by A and B. NOT usable by the oracle.
#include <vector>
#include "counters.hpp"
#include "graph.hpp"
#include "result.hpp"

// Preconditions: g acyclic, order is a valid topological order of g.
// Fills est, eft, lst, lft, slack, criticalPath, projectDuration in r.
// Does not touch r.hasCycle, r.order or r.cycle.
void computeSchedule(const Graph& g, const std::vector<int>& order, Result& r,
                     Counters* c = nullptr);
