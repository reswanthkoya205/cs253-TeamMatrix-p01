#include <iostream>
#include <string>
#include <vector>
#include "interface.hpp"
#include "validate.hpp"
#include "generators.hpp"

using namespace std;

static bool cycleInsertionMustRollback() {
    Graph g(4);
    g.addEdge(0, 1); g.addEdge(1, 2); g.addEdge(2, 3);
    DynamicOrder d;
    Result initial = runStructureB(g, &d, nullptr);
    string why;
    if (initial.hasCycle || !validateDynamic(g, d, why)) {
        cerr << "initial order invalid: " << why << '\n';
        return false;
    }

    Graph before = g;
    DynamicOrder beforeOrder = d;
    Graph candidate = g;
    candidate.addEdge(3, 0);

    InsertResult ir = insertDependencyB(g, d, 3, 0, nullptr);
    if (!ir.createdCycle || !validateCycle(candidate, ir.cycle, why)) {
        cerr << "cycle insertion did not return a valid cycle: " << why << '\n';
        return false;
    }
    if (g.adj != before.adj || g.duration != before.duration ||
        d.ord != beforeOrder.ord || d.pos != beforeOrder.pos) {
        cerr << "cycle-causing insertion modified graph or order\n";
        return false;
    }
    return true;
}

static bool selfLoopMustRollback() {
    Graph g(2); g.addEdge(0, 1);
    DynamicOrder d;
    runStructureB(g, &d, nullptr);
    Graph before = g;
    DynamicOrder old = d;
    InsertResult ir = insertDependencyB(g, d, 1, 1, nullptr);
    string why;
    Graph candidate = g; candidate.addEdge(1, 1);
    if (!ir.createdCycle || !validateCycle(candidate, ir.cycle, why)) return false;
    return g.adj == before.adj && d.ord == old.ord && d.pos == old.pos;
}

static bool successfulReordering() {
    Graph g(5);
    g.addEdge(0, 2); g.addEdge(1, 2);
    DynamicOrder d;
    Result initial = runStructureB(g, &d, nullptr);
    if (initial.hasCycle) return false;

    // With the usual DFS order, this edge can require repairing the interval.
    InsertResult ir = insertDependencyB(g, d, 4, 0, nullptr);
    string why;
    if (ir.createdCycle || !validateDynamic(g, d, why)) {
        cerr << "successful insertion failed: " << why << '\n';
        return false;
    }
    Result oracle = runOracle(g);
    DynamicOrder rebuilt;
    Result b = runStructureB(g, &rebuilt, nullptr);
    return !oracle.hasCycle && resultsEquivalent(g, b, oracle, why);
}

static bool generatedSequence() {
    const uint64_t seed = 20261110;
    InsertionSequence seq = genWorstInsertions(80, seed, 40);
    Graph g = seq.initialGraph;
    DynamicOrder d;
    Result initial = runStructureB(g, &d, nullptr);
    string why;
    if (initial.hasCycle || !validateDynamic(g, d, why)) return false;

    for (size_t i = 0; i < seq.edgesToInsert.size(); ++i) {
        const auto [u, v] = seq.edgesToInsert[i];
        Graph before = g;
        DynamicOrder old = d;
        InsertResult ir = insertDependencyB(g, d, u, v, nullptr);
        if (ir.createdCycle || !validateDynamic(g, d, why)) {
            cerr << "generated insertion " << i << " failed: " << why << '\n';
            return false;
        }
        for (int x = 0; x < g.n; ++x) {
            int p = old.pos[x];
            if ((p < ir.lo || p > ir.hi) && d.pos[x] != p) {
                cerr << "vertex outside affected interval moved\n";
                return false;
            }
        }
        Result expected = runOracle(g);
        DynamicOrder fresh;
        Result actual = runStructureB(g, &fresh, nullptr);
        if (expected.hasCycle || !resultsEquivalent(g, actual, expected, why)) {
            cerr << "oracle mismatch after insertion " << i << ": " << why << '\n';
            return false;
        }
        (void)before;
    }
    return true;
}

int main() {
    if (!cycleInsertionMustRollback()) return 1;
    if (!selfLoopMustRollback()) return 1;
    if (!successfulReordering()) return 1;
    if (!generatedSequence()) return 1;
    cout << "test_incremental: PASS (seed 20261110)\n";
    return 0;
}
