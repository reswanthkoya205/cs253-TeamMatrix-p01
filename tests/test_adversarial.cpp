#include <iostream>
#include <string>
#include "interface.hpp"
#include "validate.hpp"
#include "generators.hpp"

using namespace std;

static bool compareWithOracle(const Graph& g, const string& label) {
    string why;
    Result expected = runOracle(g);
    Result a = runStructureA(g, false, nullptr);
    Result b = runStructureB(g, nullptr, nullptr);
    if (!validateResult(g, expected, why) || !validateResult(g, a, why) ||
        !validateResult(g, b, why) || !resultsEquivalent(g, a, expected, why) ||
        !resultsEquivalent(g, b, expected, why)) {
        cerr << label << " failed: " << why << '\n';
        return false;
    }
    return true;
}

int main() {
    const uint64_t seed = 20261110;

    // Adversarial target 1: many valid topological orders.
    Graph layered = genExponentialDAG(120, seed, "layers", 4);
    Graph chains = genExponentialDAG(120, seed + 1, "chains", 5);
    Graph antichain = genExponentialDAG(120, seed + 2, "antichain", 1);
    if (!compareWithOracle(layered, "layered exponential-order DAG")) return 1;
    if (!compareWithOracle(chains, "chain-family DAG")) return 1;
    if (!compareWithOracle(antichain, "antichain DAG")) return 1;

    // Adversarial target 2: cycle placed behind a long directed chain.
    Graph hidden = genHiddenCycleBehindChain(300, 7, seed + 3, 145);
    if (!compareWithOracle(hidden, "hidden cycle behind chain")) return 1;
    Result cycleA = runStructureA(hidden, false, nullptr);
    Result cycleB = runStructureB(hidden, nullptr, nullptr);
    string why;
    if (!cycleA.hasCycle || !cycleB.hasCycle ||
        !validateCycle(hidden, cycleA.cycle, why) ||
        !validateCycle(hidden, cycleB.cycle, why)) {
        cerr << "hidden-cycle generator did not produce valid cycle reports: " << why << '\n';
        return 1;
    }

    // Adversarial target 3: every insertion is checked against the invariant.
    InsertionSequence seq = genWorstInsertions(100, seed + 4, 50);
    Graph g = seq.initialGraph;
    DynamicOrder d;
    Result initial = runStructureB(g, &d, nullptr);
    if (initial.hasCycle || !validateDynamic(g, d, why)) {
        cerr << "worst-insertion initial state invalid: " << why << '\n';
        return 1;
    }
    for (size_t i = 0; i < seq.edgesToInsert.size(); ++i) {
        auto [u, v] = seq.edgesToInsert[i];
        InsertResult ir = insertDependencyB(g, d, u, v, nullptr);
        if (ir.createdCycle || !validateDynamic(g, d, why)) {
            cerr << "worst-insertion step " << i << " failed: " << why << '\n';
            return 1;
        }
        Result expected = runOracle(g);
        DynamicOrder fresh;
        Result actual = runStructureB(g, &fresh, nullptr);
        if (expected.hasCycle || !resultsEquivalent(g, actual, expected, why)) {
            cerr << "worst-insertion oracle mismatch at step " << i << ": " << why << '\n';
            return 1;
        }
    }

    cout << "test_adversarial: PASS (seed " << seed << ")\n";
    return 0;
}
