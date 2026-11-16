#include "interface.hpp"
#include "schedule.hpp"
#include "fifo_queue.hpp"
#include "min_heap.hpp"

#include <vector>

Result runStructureA(const Graph& g, bool lexSmallest, Counters* c) {
    Result r;
    const int n = g.n;

    // Build a local adjacency list with duplicate edges removed.
    // The input graph itself is never modified.
    std::vector<std::vector<int>> adj(n);
    std::vector<int> indegree(n, 0);
    std::vector<int> lastSeen(n, -1);

    for (int u = 0; u < n; ++u) {
        for (int v : g.adj[u]) {
            if (c) ++c->edgesScanned;

            if (lastSeen[v] != u) {
                lastSeen[v] = u;
                adj[u].push_back(v);
                ++indegree[v];
            }
        }
    }

    // Kahn's algorithm: choose either FIFO order or smallest available ID.
    if (!lexSmallest) {
        FifoQueue queue;

        for (int v = 0; v < n; ++v) {
            if (indegree[v] == 0) queue.push(v);
        }

        while (!queue.empty()) {
            int u = queue.pop();
            if (c) ++c->verticesVisited;
            r.order.push_back(u);

            for (int v : adj[u]) {
                if (c) ++c->edgesScanned;
                --indegree[v];
                if (indegree[v] == 0) queue.push(v);
            }
        }
    } else {
        MinHeap heap(c);

        for (int v = 0; v < n; ++v) {
            if (indegree[v] == 0) heap.push(v);
        }

        while (!heap.empty()) {
            int u = heap.pop();
            if (c) ++c->verticesVisited;
            r.order.push_back(u);

            for (int v : adj[u]) {
                if (c) ++c->edgesScanned;
                --indegree[v];
                if (indegree[v] == 0) heap.push(v);
            }
        }
    }

    // If Kahn did not process every vertex, the residual graph contains a cycle.
    if (static_cast<int>(r.order.size()) != n) {
        r.hasCycle = true;

        std::vector<char> remaining(n, false);
        for (int v = 0; v < n; ++v) {
            if (indegree[v] > 0) remaining[v] = true;
        }

        // Reverse adjacency restricted to residual vertices.
        std::vector<std::vector<int>> reverseAdj(n);
        for (int u = 0; u < n; ++u) {
            if (!remaining[u]) continue;

            for (int v : adj[u]) {
                if (c) ++c->edgesScanned;
                if (remaining[v]) reverseAdj[v].push_back(u);
            }
        }

        // Follow residual predecessors until a vertex repeats.
        // Reversing the repeated segment restores the original edge direction.
        std::vector<int> firstPosition(n, -1);
        std::vector<int> path;

        int current = 0;
        while (current < n && !remaining[current]) ++current;

        while (firstPosition[current] == -1) {
            firstPosition[current] = static_cast<int>(path.size());
            path.push_back(current);
            if (c) ++c->verticesVisited;
            current = reverseAdj[current][0];
        }

        const int cycleStart = firstPosition[current];
        for (int i = static_cast<int>(path.size()) - 1; i >= cycleStart; --i)
            r.cycle.push_back(path[i]);
        r.cycle.push_back(path.back());

        // Cyclic results must not contain a partial order or schedule.
        r.order.clear();
        r.est.clear();
        r.eft.clear();
        r.lst.clear();
        r.lft.clear();
        r.slack.clear();
        r.criticalPath.clear();
        r.projectDuration = 0;
        return r;
    }

    r.hasCycle = false;
    computeSchedule(g, r.order, r, c);
    return r;
}
