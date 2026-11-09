#include <iostream>
#include <cassert>
#include <string>
#include "validate.hpp"
#include "graph.hpp"
#include "result.hpp"
#include "dynamic_order.hpp"

using namespace std;

int main() {
    string why;

    Graph g(3);
    g.duration = {1, 2, 3};
    g.addEdge(0, 1);
    g.addEdge(1, 2);

    assert(validateOrder(g, vector<int>{0, 1, 2}, why));
    assert(!validateOrder(g, vector<int>{2, 1, 0}, why));
    assert(!validateOrder(g, vector<int>{0, 0, 2}, why));

    assert(validateCycle(g, vector<int>{0, 1, 2, 0}, why) == false);
    // The chain has no edge 2 -> 0, so this is correctly rejected.

    Graph cyclic(3);
    cyclic.addEdge(0, 1);
    cyclic.addEdge(1, 2);
    cyclic.addEdge(2, 0);
    assert(validateCycle(cyclic, vector<int>{0, 1, 2, 0}, why));

    DynamicOrder d;
    d.ord = {0, 1, 2};
    d.pos = {0, 1, 2};
    assert(validateDynamic(g, d, why));

    d.pos[0] = 2;
    assert(!validateDynamic(g, d, why));

    cout << "test_validators: PASS\n";
    return 0;
}
