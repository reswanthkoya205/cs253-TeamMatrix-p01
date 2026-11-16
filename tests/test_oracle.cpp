#include <iostream>
#include <cassert>
#include "interface.hpp"
#include "graph.hpp"
#include "result.hpp"

using namespace std;

int main() {
    // Empty graph.
    {
        Graph g(0);
        Result r = runOracle(g);
        assert(!r.hasCycle);
        assert(r.order.empty());
        assert(r.projectDuration == 0);
    }

    // One isolated vertex.
    {
        Graph g(1);
        g.duration = {7};
        Result r = runOracle(g);
        assert(!r.hasCycle);
        assert(r.order.size() == 1 && r.order[0] == 0);
        assert(r.projectDuration == 7);
        assert(r.est[0] == 0 && r.eft[0] == 7);
    }

    // Chain: 0 -> 1 -> 2.
    {
        Graph g(3);
        g.duration = {2, 3, 4};
        g.addEdge(0, 1);
        g.addEdge(1, 2);
        Result r = runOracle(g);
        assert(!r.hasCycle);
        assert((r.order == vector<int>{0, 1, 2}));
        assert(r.projectDuration == 9);
        assert((r.est == vector<long long>{0, 2, 5}));
    }

    // Self-loop.
    {
        Graph g(1);
        g.addEdge(0, 0);
        Result r = runOracle(g);
        assert(r.hasCycle);
        assert((r.cycle == vector<int>{0, 0}));
        assert(r.order.empty());
    }

    // Three-vertex cycle.
    {
        Graph g(3);
        g.addEdge(0, 1);
        g.addEdge(1, 2);
        g.addEdge(2, 0);
        Result r = runOracle(g);
        assert(r.hasCycle);
        assert(r.cycle.size() >= 2);
        assert(r.cycle.front() == r.cycle.back());
    }

    cout << "test_oracle: PASS\n";
    return 0;
}
