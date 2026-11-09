#pragma once
// Build graph. Owner: Architect.
// Edge u->v means: target u must be built BEFORE target v.
// adj[u] = targets that depend on u. Vertices are 0-indexed.
#include <stdexcept>
#include <vector>

struct Graph {
    int n = 0;
    std::vector<std::vector<int>> adj;
    std::vector<int> duration;  // non-negative

    Graph() = default;
    explicit Graph(int vertices) : n(vertices), adj(vertices), duration(vertices, 0) {}

    // Range-checked. Duplicate edges allowed; u->u is a cycle.
    void addEdge(int u, int v) {
        if (u < 0 || v < 0 || u >= n || v >= n)
            throw std::out_of_range("Graph::addEdge: vertex out of range");
        adj[u].push_back(v);
    }

    long long edgeCount() const {
        long long m = 0;
        for (const auto& a : adj) m += (long long)a.size();
        return m;
    }
};
