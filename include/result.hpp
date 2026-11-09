#pragma once
// Output of a full analysis. Owner: Architect.
// See INTERFACE.md for which fields are filled in the acyclic / cyclic case.
#include <vector>
#include "common.hpp"

struct Result {
    bool hasCycle = false;

    std::vector<int> order;      // topological order (empty if cyclic)
    std::vector<int> cycle;      // closed simple cycle: first == last (empty if acyclic)

    std::vector<ll> est, eft;    // earliest start / finish
    std::vector<ll> lst, lft;    // latest start / finish
    std::vector<ll> slack;       // lst - est, >= 0
    std::vector<int> criticalPath;
    ll projectDuration = 0;      // max eft
};
