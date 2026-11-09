#include "schedule.hpp"

#include <cstddef>

void computeSchedule(const Graph& g, const std::vector<int>& order, Result& r, Counters* c) {
    const int n = g.n;

    r.est.assign(n, 0);
    r.eft.assign(n, 0);
    r.lst.assign(n, 0);
    r.lft.assign(n, 0);
    r.slack.assign(n, 0);
    r.criticalPath.clear();
    r.projectDuration = 0;
    if (n == 0) return;

    for (std::size_t i = 0; i < order.size(); ++i) {
        const int u = order[i];
        r.eft[u] = r.est[u] + g.duration[u];
        if (r.eft[u] > r.projectDuration) r.projectDuration = r.eft[u];
        if (c) c->verticesVisited++;
        for (std::size_t k = 0; k < g.adj[u].size(); ++k) {
            const int v = g.adj[u][k];
            if (c) c->edgesScanned++;
            if (r.eft[u] > r.est[v]) r.est[v] = r.eft[u];
        }
    }

    for (std::size_t i = order.size(); i-- > 0;) {
        const int u = order[i];
        ll lft = r.projectDuration;
        for (std::size_t k = 0; k < g.adj[u].size(); ++k) {
            const int v = g.adj[u][k];
            if (c) c->edgesScanned++;
            if (r.lst[v] < lft) lft = r.lst[v];
        }
        r.lft[u] = lft;
        r.lst[u] = lft - g.duration[u];
        r.slack[u] = r.lst[u] - r.est[u];
    }

    int cur = NONE;
    for (int v = 0; v < n; ++v) {
        if (r.est[v] == 0 && r.slack[v] == 0) {
            cur = v;
            break;
        }
    }
    while (cur != NONE) {
        r.criticalPath.push_back(cur);
        int next = NONE;
        for (std::size_t k = 0; k < g.adj[cur].size(); ++k) {
            const int v = g.adj[cur][k];
            if (r.slack[v] == 0 && r.est[v] == r.eft[cur] && (next == NONE || v < next)) next = v;
        }
        cur = next;
    }
}