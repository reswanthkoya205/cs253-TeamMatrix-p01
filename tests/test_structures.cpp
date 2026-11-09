#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include "interface.hpp"
#include "validate.hpp"

int main() {
    std::string why;
    Graph g(4);
    g.duration = {2, 3, 5, 7};
    g.addEdge(0, 2);
    g.addEdge(1, 2);
    g.addEdge(2, 3);

    Result oracle = runOracle(g);
    Result a = runStructureA(g, false, nullptr);
    Result aLex = runStructureA(g, true, nullptr);
    DynamicOrder d;
    Result b = runStructureB(g, &d, nullptr);

    assert(validateResult(g, oracle, why));
    assert(validateResult(g, a, why));
    assert(validateResult(g, aLex, why));
    assert(validateResult(g, b, why));
    assert(resultsEquivalent(g, a, oracle, why));
    assert(resultsEquivalent(g, b, oracle, why));
    assert(aLex.order == oracle.order);
    assert(validateDynamic(g, d, why));

    // A cyclic graph must produce a valid cycle and no partial schedule.
    Graph cyclic(3);
    cyclic.addEdge(0, 1);
    cyclic.addEdge(1, 2);
    cyclic.addEdge(2, 0);
    Result oracleCycle = runOracle(cyclic);
    Result aCycle = runStructureA(cyclic, false, nullptr);
    Result bCycle = runStructureB(cyclic, nullptr, nullptr);
    assert(validateResult(cyclic, oracleCycle, why));
    assert(validateResult(cyclic, aCycle, why));
    assert(validateResult(cyclic, bCycle, why));
    assert(resultsEquivalent(cyclic, aCycle, oracleCycle, why));
    assert(resultsEquivalent(cyclic, bCycle, oracleCycle, why));

    std::cout << "test_structures: PASS\n";
}
