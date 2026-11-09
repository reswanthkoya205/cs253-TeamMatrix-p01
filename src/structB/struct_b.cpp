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
