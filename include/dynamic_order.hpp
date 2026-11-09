#pragma once
// State for incremental topological order maintenance (Pearce-Kelly). Owner: Architect.
#include <vector>

struct DynamicOrder {
    std::vector<int> ord;   // ord[i] = vertex at position i
    std::vector<int> pos;   // pos[v] = position of v; invariant pos[ord[i]] == i
};

struct InsertResult {
    bool createdCycle = false;
    std::vector<int> cycle;  // closed simple cycle if createdCycle (first == last)
    int lo = 0, hi = 0;      // affected region [lo, hi] in OLD positions (pos[v], pos[u])
    int affected = 0;        // vertices examined in the region
    int moved = 0;           // vertices whose position actually changed
};
