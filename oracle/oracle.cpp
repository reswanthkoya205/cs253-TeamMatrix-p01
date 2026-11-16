#include <algorithm>
#include <functional>
#include <vector>
#include <string>
#include "interface.hpp"
#include "graph.hpp"
#include "result.hpp"

using namespace std;

// Independent, deliberately slow reference implementation.
// It must NOT call Structure A, Structure B, insertDependencyB,
// or computeSchedule.
Result runOracle(const Graph& g) {
    Result r;
    const int n = g.n;

    r.hasCycle = false;
    r.order.clear();
    r.cycle.clear();
    r.est.clear();
    r.eft.clear();
    r.lst.clear();
    r.lft.clear();
    r.slack.clear();
    r.criticalPath.clear();
    r.projectDuration = 0;

    // Count indegrees. Duplicate edges are harmless: each duplicate is
    // counted and later removed once, so the final ordering is unchanged.
    vector<int> indeg(n, 0);
    for (int u = 0; u < n; ++u)
        for (int v : g.adj[u])
            ++indeg[v];

    vector<char> removed(n, false);

    // O(V^2 + E) Kahn reference: each iteration linearly scans for the
    // smallest remaining zero-indegree vertex.
    for (int step = 0; step < n; ++step) {
        int chosen = -1;
        for (int v = 0; v < n; ++v) {
            if (!removed[v] && indeg[v] == 0) {
                chosen = v;
                break;
            }
        }

        if (chosen == -1) break;

        removed[chosen] = true;
        r.order.push_back(chosen);

        for (int v : g.adj[chosen])
            --indeg[v];
    }

    if ((int)r.order.size() != n) {
        r.hasCycle = true;
        r.order.clear();

        // Find one actual directed cycle among vertices left after Kahn.
        // Iterative DFS avoids recursion-depth problems on long chains.
        vector<int> color(n, 0), parent(n, -1), nextIndex(n, 0);

        for (int start = 0; start < n && r.cycle.empty(); ++start) {
            if (removed[start] || color[start] != 0) continue;

            vector<int> st;
            st.push_back(start);
            color[start] = 1;

            while (!st.empty() && r.cycle.empty()) {
                int u = st.back();

                if (nextIndex[u] == (int)g.adj[u].size()) {
                    color[u] = 2;
                    st.pop_back();
                    continue;
                }

                int v = g.adj[u][nextIndex[u]++];
                if (removed[v]) continue;

                if (color[v] == 0) {
                    parent[v] = u;
                    color[v] = 1;
                    st.push_back(v);
                } else if (color[v] == 1) {
                    // Back edge u -> v. Reconstruct v ... u -> v.
                    vector<int> tail;
                    int x = u;
                    tail.push_back(x);
                    while (x != v && x != -1) {
                        x = parent[x];
                        if (x != -1) tail.push_back(x);
                    }

                    if (x == v) {
                        reverse(tail.begin(), tail.end()); // v ... u
                        r.cycle = tail;
                        r.cycle.push_back(v);
                    }
                }
            }
        }

        // A cyclic result has no schedule fields.
        r.est.clear();
        r.eft.clear();
        r.lst.clear();
        r.lft.clear();
        r.slack.clear();
        r.criticalPath.clear();
        r.projectDuration = 0;
        return r;
    }

    // Independent naive schedule computation.
    // Forward relaxation until no value changes.
    r.est.assign(n, 0);
    r.eft.assign(n, 0);

    bool changed = true;
    for (int pass = 0; pass < n && changed; ++pass) {
        changed = false;
        for (int u = 0; u < n; ++u) {
            for (int v : g.adj[u]) {
                long long candidate = r.est[u] + g.duration[u];
                if (candidate > r.est[v]) {
                    r.est[v] = candidate;
                    changed = true;
                }
            }
        }
    }

    r.projectDuration = 0;
    for (int v = 0; v < n; ++v) {
        r.eft[v] = r.est[v] + g.duration[v];
        r.projectDuration = max(r.projectDuration, r.eft[v]);
    }

    // Backward relaxation. Initially every vertex can finish by the
    // project duration; edges propagate tighter deadlines backwards.
    r.lft.assign(n, r.projectDuration);
    r.lst.assign(n, 0);

    changed = true;
    for (int pass = 0; pass < n && changed; ++pass) {
        changed = false;
        for (int u = 0; u < n; ++u) {
            for (int v : g.adj[u]) {
                long long candidate = r.lft[v] - g.duration[v];
                if (candidate < r.lft[u]) {
                    r.lft[u] = candidate;
                    changed = true;
                }
            }
        }
    }

    r.slack.assign(n, 0);
    for (int v = 0; v < n; ++v) {
        r.lst[v] = r.lft[v] - g.duration[v];
        r.slack[v] = r.lst[v] - r.est[v];
    }

    // Construct one deterministic critical path. Try start vertices in
    // increasing id; at each step choose the smallest eligible successor.
    for (int start = 0; start < n && r.criticalPath.empty(); ++start) {
        if (r.est[start] != 0 || r.slack[start] != 0) continue;

        vector<int> path(1, start);
        int cur = start;
        vector<char> seen(n, false);
        seen[cur] = true;

        while (r.eft[cur] != r.projectDuration) {
            int next = -1;
            for (int v : g.adj[cur]) {
                if (!seen[v] && r.slack[v] == 0 &&
                    r.est[v] == r.eft[cur]) {
                    if (next == -1 || v < next) next = v;
                }
            }
            if (next == -1) {
                path.clear();
                break;
            }
            cur = next;
            seen[cur] = true;
            path.push_back(cur);
        }

        if (!path.empty() && r.eft[path.back()] == r.projectDuration)
            r.criticalPath = path;
    }

    return r;
}
