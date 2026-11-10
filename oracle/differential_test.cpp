#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include "interface.hpp"
#include "validate.hpp"
#include "generators.hpp"

using namespace std;

static bool checkOneGraph(const Graph& g, int testNumber) {
    string why;

    Result expected = runOracle(g);
    Result a = runStructureA(g, false, nullptr);
    Result aLex = runStructureA(g, true, nullptr);

    DynamicOrder d;
    Result b = runStructureB(g, &d, nullptr);

    if (!validateResult(g, expected, why)) {
        cerr << "Oracle validation failed on test " << testNumber
             << ": " << why << '\n';
        return false;
    }
    if (!validateResult(g, a, why)) {
        cerr << "Structure A validation failed on test " << testNumber
             << ": " << why << '\n';
        return false;
    }
    if (!validateResult(g, aLex, why)) {
        cerr << "Structure A lexicographic validation failed on test " << testNumber
             << ": " << why << '\n';
        return false;
    }
    if (!validateResult(g, b, why)) {
        cerr << "Structure B validation failed on test " << testNumber
             << ": " << why << '\n';
        return false;
    }
    if (!resultsEquivalent(g, a, expected, why)) {
        cerr << "A vs Oracle failed on test " << testNumber
             << ": " << why << '\n';
        return false;
    }
    if (!resultsEquivalent(g, aLex, expected, why)) {
        cerr << "A lexicographic vs Oracle failed on test " << testNumber
             << ": " << why << '\n';
        return false;
    }
    if (!expected.hasCycle && aLex.order != expected.order) {
        cerr << "Lexicographically smallest order differs from Oracle on test "
             << testNumber << '\n';
        return false;
    }
    if (!resultsEquivalent(g, b, expected, why)) {
        cerr << "B vs Oracle failed on test " << testNumber
             << ": " << why << '\n';
        return false;
    }

    if (!expected.hasCycle && !validateDynamic(g, d, why)) {
        cerr << "Dynamic-order validation failed on test " << testNumber
             << ": " << why << '\n';
        return false;
    }
    return true;
}

static bool checkIncrementalInsertions(int n, unsigned int seed) {
    string why;
    Graph g(n);

    // genWorstInsertions fills g with an initial DAG and returns
    // the sequence of new directed edges to insert.
    InsertionSequence sequence = genWorstInsertions(n, seed, n/2);
    g = sequence.initialGraph;
    const vector<pair<int, int>>& insertions = sequence.edgesToInsert;

    DynamicOrder d;
    Result initial = runStructureB(g, &d, nullptr);
    if (initial.hasCycle || !validateDynamic(g, d, why)) {
        cerr << "Invalid initial graph/order: " << why << '\n';
        return false;
    }

    for (size_t i = 0; i < insertions.size(); ++i) {
        int u = insertions[i].first;
        int v = insertions[i].second;

        Graph beforeGraph = g;
        DynamicOrder beforeOrder = d;

        // The proposed edge is included only in this temporary graph.
        // A cycle-causing insertion must leave the real graph unchanged.
        Graph candidate = g;
        candidate.addEdge(u, v);

        InsertResult ir = insertDependencyB(g, d, u, v, nullptr);

        if (ir.createdCycle) {
            if (!validateCycle(candidate, ir.cycle, why)) {
                cerr << "Invalid reported insertion cycle at step "
                     << i << ": " << why << '\n';
                return false;
            }
            if (g.adj != beforeGraph.adj ||
                g.duration != beforeGraph.duration ||
                d.ord != beforeOrder.ord || d.pos != beforeOrder.pos) {
                cerr << "Cycle-causing insertion changed graph/order at step "
                     << i << '\n';
                return false;
            }
            continue;
        }

        if (!validateDynamic(g, d, why)) {
            cerr << "Dynamic order invalid after insertion " << i
                 << ": " << why << '\n';
            return false;
        }

        // INTERFACE.md invariant 5.3: vertices outside the OLD affected
        // interval [lo, hi] must retain their positions.
        for (int vertex = 0; vertex < n; ++vertex) {
            int oldPosition = beforeOrder.pos[vertex];
            if ((oldPosition < ir.lo || oldPosition > ir.hi) &&
                d.pos[vertex] != oldPosition) {
                cerr << "Vertex outside old affected interval moved at step "
                     << i << ": vertex " << vertex << '\n';
                return false;
            }
        }

        // Check the incrementally maintained order on the updated graph.
        if (!validateOrder(g, d.ord, why)) {
            cerr << "Incremental order invalid at step " << i
                 << ": " << why << '\n';
            return false;
        }

        // Oracle is run on the updated graph after every successful insertion.
        Result expected = runOracle(g);
        if (expected.hasCycle) {
            cerr << "Successful insertion unexpectedly produced a cycle at "
                 << "step " << i << '\n';
            return false;
        }

        // The interface exposes DynamicOrder, not a complete Result from
        // insertDependencyB. Therefore validate d directly, and separately
        // verify the complete schedule using a fresh B run.
        DynamicOrder rebuilt;
        Result bNow = runStructureB(g, &rebuilt, nullptr);
        if (!resultsEquivalent(g, bNow, expected, why)) {
            cerr << "Post-insertion B/Oracle mismatch at step " << i
                 << ": " << why << '\n';
            return false;
        }
    }
    return true;
}

int main() {
    const int TESTS = 1000;
    const unsigned int BASE_SEED = 20261109;

    cout << "Starting static differential tests...\n";

    for (int t = 0; t < TESTS; ++t) {
        unsigned int seed = BASE_SEED + t;
        Graph g = genRandomGraph(30, 80, seed);

        if (!checkOneGraph(g, t)) {
            cerr << "FAILED seed = " << seed << '\n';
            return 1;
        }

        if (t % 100 == 0)
            cout << "Completed " << t << " tests\n";
    }

    cout << "Static differential tests passed.\n";

    if (!checkIncrementalInsertions(100, BASE_SEED + 99999)) {
        cerr << "Incremental insertion tests FAILED\n";
        return 1;
    }

    cout << "Incremental insertion tests passed.\n";
    cout << "ALL DIFFERENTIAL TESTS PASSED\n";
    return 0;
}
