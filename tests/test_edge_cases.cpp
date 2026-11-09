#include <iostream>
#include <string>
#include <vector>
#include "interface.hpp"
#include "validate.hpp"

using namespace std;

static bool checkAll(const Graph& g, const string& label) {
    string why;
    Result oracle = runOracle(g);
    Result a = runStructureA(g, false, nullptr);
    Result aLex = runStructureA(g, true, nullptr);
    DynamicOrder d;
    Result b = runStructureB(g, &d, nullptr);

    if (!validateResult(g, oracle, why) || !validateResult(g, a, why) ||
        !validateResult(g, aLex, why) || !validateResult(g, b, why) ||
        !resultsEquivalent(g, a, oracle, why) ||
        !resultsEquivalent(g, b, oracle, why)) {
        cerr << "[" << label << "] " << why << '\n';
        return false;
    }
    if (!g.n && !a.order.empty()) return false;
    if (!oracle.hasCycle && !validateDynamic(g, d, why)) {
        cerr << "[" << label << "] dynamic order: " << why << '\n';
        return false;
    }
    return true;
}

int main() {
    // Empty graph and a single isolated vertex.
    if (!checkAll(Graph(0), "empty graph")) return 1;
    Graph one(1); one.duration[0] = 7;
    if (!checkAll(one, "single vertex")) return 1;

    // Disconnected components and isolated vertices.
    Graph disconnected(7);
    disconnected.duration = {2, 3, 4, 5, 0, 1, 6};
    disconnected.addEdge(0, 1);
    disconnected.addEdge(2, 3);
    disconnected.addEdge(3, 4);
    if (!checkAll(disconnected, "disconnected components")) return 1;

    // Duplicate edges must behave as one dependency.
    Graph duplicates(3);
    duplicates.duration = {1, 2, 3};
    duplicates.addEdge(0, 1); duplicates.addEdge(0, 1);
    duplicates.addEdge(1, 2); duplicates.addEdge(1, 2);
    if (!checkAll(duplicates, "duplicate edges")) return 1;

    // Zero-duration tasks.
    Graph zero(4);
    zero.duration = {0, 0, 5, 0};
    zero.addEdge(0, 1); zero.addEdge(1, 2); zero.addEdge(2, 3);
    if (!checkAll(zero, "zero durations")) return 1;

    // Two distinct critical paths of equal total duration.
    Graph twoCritical(4);
    twoCritical.duration = {2, 3, 3, 2};
    twoCritical.addEdge(0, 1); twoCritical.addEdge(0, 2);
    twoCritical.addEdge(1, 3); twoCritical.addEdge(2, 3);
    if (!checkAll(twoCritical, "multiple critical paths")) return 1;

    // Self-loop and a short directed cycle.
    Graph selfLoop(1); selfLoop.addEdge(0, 0);
    if (!checkAll(selfLoop, "self loop")) return 1;
    Graph twoCycle(2); twoCycle.addEdge(0, 1); twoCycle.addEdge(1, 0);
    if (!checkAll(twoCycle, "two vertex cycle")) return 1;

    cout << "test_edge_cases: PASS\n";
    return 0;
}
