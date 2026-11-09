#include "interface.hpp"
#include "schedule.hpp"
#include <algorithm>
#include <vector>
using namespace std;

Result runStructureB(const Graph& g, DynamicOrder* out, Counters* c) {
    Result r;
    int n = g.n;

    vector<int> state(n, 0);
    vector<int> next(n, 0);
    vector<int> finish;
    vector<int> st;

    for (int s = 0; s < n; ++s) {
        if (state[s] != 0) continue;

        state[s] = 1;
        st.push_back(s);

        if (c) ++c->verticesVisited;

        while (!st.empty()) {
            int u = st.back();

            if (next[u] < (int)g.adj[u].size()) {
                int v = g.adj[u][next[u]++];

                if (c) ++c->edgesScanned;

                if (state[v] == 0) {
                    state[v] = 1;
                    st.push_back(v);

                    if (c) ++c->verticesVisited;
                }
                else if (state[v] == 1) {
                    r.hasCycle = true;

                    int start = 0;
                    while (st[start] != v) {
                        ++start;
                    }

                    for (int i = start; i < (int)st.size(); ++i) {
                        r.cycle.push_back(st[i]);
                    }

                    r.cycle.push_back(v);
                    return r;
                }
            }
            else {
                state[u] = 2;
                finish.push_back(u);
                st.pop_back();
            }
        }
    }

    std::reverse(finish.begin(), finish.end());

    r.order = finish;
    r.hasCycle = false;

    if (out != nullptr) {
        out->ord = r.order;
        out->pos.resize(n);

        for (int i = 0; i < n; ++i) {
            out->pos[r.order[i]] = i;
        }
    }

    computeSchedule(g, r.order, r, c);

    return r;
}


InsertResult insertDependencyB(
    Graph& g, DynamicOrder& d, int u, int v, Counters* c
) {
    InsertResult res;

    if (u == v) {
        res.createdCycle = true;
        res.cycle = {u, u};
        res.lo = d.pos[u];
        res.hi = d.pos[u];
        res.affected = 1;

        if (c) {
            c->regionSize = 1;
            c->verticesMoved = 0;
        }

        return res;
    }

    if (d.pos[u] < d.pos[v]) {
        g.addEdge(u, v);

        if (c) {
            c->regionSize = 0;
            c->verticesMoved = 0;
        }

        return res;
    }

    const int lo = d.pos[v];
    const int hi = d.pos[u];

    res.lo = lo;
    res.hi = hi;

    const int n = g.n;

    std::vector<char> reachable(n, false);

    std::vector<int> parent(n, -1);

    std::vector<int> st;

    reachable[v] = true;
    st.push_back(v);

    bool cycleFound = false;
    int examined = 0;

    while (!st.empty()) {
        int x = st.back();
        st.pop_back();

        ++examined;

        if (c) ++c->verticesVisited;

        for (int w : g.adj[x]) {
            if (c) ++c->edgesScanned;

            int p = d.pos[w];

            if (p < lo || p > hi)
                continue;

            if (reachable[w])
                continue;

            reachable[w] = true;
            parent[w] = x;


            if (w == u) {
                cycleFound = true;
                break;
            }

            st.push_back(w);
        }

        if (cycleFound)
            break;
    }

    if (cycleFound) {
        res.createdCycle = true;

        std::vector<int> path;

        int x = u;
        while (x != -1) {
            path.push_back(x);
            x = parent[x];
        }

        std::reverse(path.begin(), path.end());

        res.cycle = path;
        res.cycle.push_back(v);

        res.affected = examined;

        if (c) {
            c->regionSize = examined;
            c->verticesMoved = 0;
        }

        return res;
    }

    std::vector<int> newOrder;
    newOrder.reserve(hi - lo + 1);

    for (int i = lo; i <= hi; ++i) {
        int x = d.ord[i];

        if (!reachable[x])
            newOrder.push_back(x);
    }

    for (int i = lo; i <= hi; ++i) {
        int x = d.ord[i];

        if (reachable[x])
            newOrder.push_back(x);
    }

    int moved = 0;

    for (int i = 0; i < (int)newOrder.size(); ++i) {
        int oldPosition = d.pos[newOrder[i]];
        int newPosition = lo + i;

        if (oldPosition != newPosition)
            ++moved;
    }

    for (int i = 0; i < (int)newOrder.size(); ++i) {
        int x = newOrder[i];

        d.ord[lo + i] = x;
        d.pos[x] = lo + i;
    }

    g.addEdge(u, v);

    res.affected = hi - lo + 1;
    res.moved = moved;

    if (c) {
        c->regionSize = res.affected;
        c->verticesMoved += moved;
    }

    return res;
}

