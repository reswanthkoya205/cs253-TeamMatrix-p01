#include <algorithm>
#include <vector>
#include <string>

#include "interface.hpp"
#include "graph.hpp"
#include "result.hpp"

using namespace std;

// Reference implementation ("oracle") used to check the main solution.
// It is deliberately simple and does NOT use Structure A, Structure B
// or computeSchedule. Everything is done in the most straightforward way:
//   1. Recursive DFS  -> topological order + cycle detection
//   2. Repeated relaxation -> earliest / latest times
//   3. Slack and one deterministic critical path

namespace {

// Vertex colours used by the DFS:
//   WHITE = not visited yet
//   GRAY  = currently on the recursion stack
//   BLACK = completely finished
enum Color { WHITE = 0, GRAY = 1, BLACK = 2 };

// Everything the recursive DFS needs to remember between calls.
struct DfsState {
    vector<int> color;        // colour of every vertex
    vector<int> parent;       // who discovered each vertex (to rebuild a cycle)
    vector<int> finishOrder;  // vertices in the order they finished
    vector<int> cycle;        // filled in only if a cycle is found
    bool foundCycle = false;

    explicit DfsState(int n) : color(n, WHITE), parent(n, -1) {}
};

// Recursive DFS from vertex u.
// Returns true as soon as a cycle is found, so callers can stop early.
bool dfs(const Graph& g, int u, DfsState& s) {
    s.color[u] = GRAY;

    for (int v : g.adj[u]) {
        if (s.color[v] == WHITE) {
            // Unvisited neighbour: go deeper.
            s.parent[v] = u;
            if (dfs(g, v, s)) return true;
        } else if (s.color[v] == GRAY) {
            // We reached a vertex that is still on the stack,
            // so the edge u -> v closes a cycle.
            s.foundCycle = true;

            // Walk back through the parents from u until we hit v.
            vector<int> path;
            for (int x = u; x != v; x = s.parent[x]) {
                path.push_back(x);
            }
            path.push_back(v);

            // Flip it so it reads v ... u, then close it with v again.
            reverse(path.begin(), path.end());
            path.push_back(v);

            s.cycle = path;
            return true;
        }
        // BLACK neighbour: already fully explored, nothing to do.
    }

    // All neighbours done, so u is finished.
    s.color[u] = BLACK;
    s.finishOrder.push_back(u);
    return false;
}

}  // namespace

Result runOracle(const Graph& g) {
    Result r;
    const int n = g.n;

    // Start from an empty, "no cycle" result.
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

    // ---------- Step 1: DFS (topological order + cycle check) ----------

    DfsState state(n);

    // Start a DFS from every unvisited vertex so disconnected graphs work too.
    for (int start = 0; start < n; ++start) {
        if (state.color[start] == WHITE) {
            if (dfs(g, start, state)) break;  // stop at the first cycle
        }
    }

    // A cyclic graph has no order and no schedule, so return right away.
    if (state.foundCycle) {
        r.hasCycle = true;
        r.cycle = state.cycle;
        return r;
    }

    // Reversing the finish order gives a valid topological order.
    reverse(state.finishOrder.begin(), state.finishOrder.end());
    r.order = state.finishOrder;

    // ---------- Step 2: earliest start / finish (forward pass) ----------

    r.est.assign(n, 0);
    r.eft.assign(n, 0);

    // Keep relaxing edges until nothing changes. A DAG needs at most n passes.
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

    // Finish times, and the overall project duration (latest finish of all).
    for (int v = 0; v < n; ++v) {
        r.eft[v] = r.est[v] + g.duration[v];
        r.projectDuration = max(r.projectDuration, r.eft[v]);
    }

    // ---------- Step 3: latest start / finish (backward pass) ----------

    // Every task may finish at the project end unless a successor forces it earlier.
    r.lft.assign(n, r.projectDuration);
    r.lst.assign(n, 0);

    changed = true;
    for (int pass = 0; pass < n && changed; ++pass) {
        changed = false;
        for (int u = 0; u < n; ++u) {
            for (int v : g.adj[u]) {
                // u must finish before v has to start.
                long long candidate = r.lft[v] - g.duration[v];
                if (candidate < r.lft[u]) {
                    r.lft[u] = candidate;
                    changed = true;
                }
            }
        }
    }

    // ---------- Step 4: slack ----------

    // Slack = how much a task can be delayed without delaying the project.
    r.slack.assign(n, 0);
    for (int v = 0; v < n; ++v) {
        r.lst[v] = r.lft[v] - g.duration[v];
        r.slack[v] = r.lst[v] - r.est[v];
    }

    // ---------- Step 5: one critical path ----------

    // To stay deterministic we always take the smallest valid start vertex
    // and then the smallest valid successor at every step.
    for (int start = 0; start < n && r.criticalPath.empty(); ++start) {

        // A path must begin at a zero-slack task that starts at time 0.
        if (r.est[start] != 0 || r.slack[start] != 0) continue;

        vector<int> path(1, start);
        vector<char> seen(n, false);
        int cur = start;
        seen[cur] = true;

        // Walk forward until we reach a task that ends exactly at project end.
        while (r.eft[cur] != r.projectDuration) {
            int next = -1;

            for (int v : g.adj[cur]) {
                // Next task must be critical and start right when cur finishes.
                if (!seen[v] && r.slack[v] == 0 && r.est[v] == r.eft[cur]) {
                    if (next == -1 || v < next) next = v;
                }
            }

            if (next == -1) {  // dead end: this start does not work
                path.clear();
                break;
            }

            cur = next;
            seen[cur] = true;
            path.push_back(cur);
        }

        if (!path.empty() && r.eft[path.back()] == r.projectDuration) {
            r.criticalPath = path;
        }
    }

    return r;
}