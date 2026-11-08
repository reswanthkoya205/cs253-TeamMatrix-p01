#pragma once
// Validators and invariant checkers. Owner: Oracle owner. Implemented in oracle/.
// Each returns true on success; on failure returns false and writes a reason to `why`.
// Must not depend on interface.hpp or schedule.hpp.
#include <string>
#include <vector>
#include "dynamic_order.hpp"
#include "graph.hpp"
#include "result.hpp"

// order is a permutation of 0..n-1 and every edge u->v has pos[u] < pos[v].
bool validateOrder(const Graph& g, const std::vector<int>& order, std::string& why);

// cycle is closed (first == last), simple (no repeated interior vertex), every step is an edge.
bool validateCycle(const Graph& g, const std::vector<int>& cycle, std::string& why);

// Structural checks of a whole Result (acyclic or cyclic case) against the invariants
// in INTERFACE.md, including est/eft/lst/lft/slack and critical path.
bool validateResult(const Graph& g, const Result& r, std::string& why);

// pos[ord[i]] == i, ord is a permutation, and every edge of g is respected.
bool validateDynamic(const Graph& g, const DynamicOrder& d, std::string& why);

// Differential comparison per the equivalence contract: hasCycle, projectDuration,
// est, lst, slack compared directly; order, cycle, criticalPath via validators.
bool resultsEquivalent(const Graph& g, const Result& got, const Result& expected,
                       std::string& why);
