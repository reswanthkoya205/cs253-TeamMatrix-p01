#pragma once
// Operation counters, the variables of the Analyst's cost model (D6).
// Every structure takes an optional Counters*; nullptr means "don't count".
#include "common.hpp"

struct Counters {
    ll edgesScanned = 0;
    ll verticesVisited = 0;
    ll heapPushes = 0;
    ll heapPops = 0;
    ll comparisons = 0;
    ll verticesMoved = 0;   // vertices whose position changed in an incremental insert
    ll regionSize = 0;      // size of the affected region of the last insertion

    void reset() { *this = Counters(); }
};
