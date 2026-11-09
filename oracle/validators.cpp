#include <algorithm>
#include <climits>
#include <sstream>
#include <vector>
#include <string>
#include "validate.hpp"
#include "graph.hpp"
#include "result.hpp"
#include "dynamic_order.hpp"

using namespace std;

static bool fail(string& why, const string& message) {
    why = message;
    return false;
}

static bool hasEdge(const Graph& g, int u, int v) {
    if (u < 0 || u >= g.n) return false;
    for (int x : g.adj[u]) if (x == v) return true;
    return false;
}

bool validateOrder(const Graph& g, const vector<int>& order, string& why) {
    why.clear();
    if ((int)order.size() != g.n)
        return fail(why, "order length is not equal to number of vertices");

    vector<int> pos(g.n, -1);
    for (int i = 0; i < g.n; ++i) {
        int v = order[i];
        if (v < 0 || v >= g.n)
            return fail(why, "order contains an out-of-range vertex");
        if (pos[v] != -1)
            return fail(why, "order contains a duplicate vertex");
        pos[v] = i;
    }

    for (int u = 0; u < g.n; ++u) {
        for (int v : g.adj[u]) {
            if (v < 0 || v >= g.n)
                return fail(why, "graph contains an out-of-range edge endpoint");
            if (pos[u] >= pos[v])
                return fail(why, "topological order violates edge " +
                                  to_string(u) + " -> " + to_string(v));
        }
    }
    return true;
}

bool validateCycle(const Graph& g, const vector<int>& cycle, string& why) {
    why.clear();
    if (cycle.size() < 2)
        return fail(why, "cycle must contain at least one edge");
    if (cycle.front() != cycle.back())
        return fail(why, "cycle is not closed");

    vector<char> seen(g.n, false);
    for (size_t i = 0; i + 1 < cycle.size(); ++i) {
        int u = cycle[i], v = cycle[i + 1];
        if (u < 0 || u >= g.n || v < 0 || v >= g.n)
            return fail(why, "cycle contains an out-of-range vertex");
        if (seen[u])
            return fail(why, "cycle repeats an interior vertex");
        seen[u] = true;
        if (!hasEdge(g, u, v))
            return fail(why, "cycle uses a non-existent directed edge");
    }
    return true;
}

static bool validateCriticalPath(const Graph& g, const Result& r,
                                 string& why) {
    if (g.n == 0) {
        if (!r.criticalPath.empty())
            return fail(why, "empty graph must have an empty critical path");
        return true;
    }
    if (r.criticalPath.empty())
        return fail(why, "critical path is empty");

    long long sum = 0;
    for (size_t i = 0; i < r.criticalPath.size(); ++i) {
        int v = r.criticalPath[i];
        if (v < 0 || v >= g.n)
            return fail(why, "critical path contains an invalid vertex");
        if (r.slack[v] != 0)
            return fail(why, "critical path contains a non-critical vertex");
        if (i == 0 && r.est[v] != 0)
            return fail(why, "critical path does not start at earliest time zero");
        if (i > 0) {
            int u = r.criticalPath[i - 1];
            if (!hasEdge(g, u, v))
                return fail(why, "consecutive critical-path vertices are not connected");
            if (r.est[v] != r.eft[u])
                return fail(why, "critical path has a timing gap");
        }
        sum += g.duration[v];
    }

    int last = r.criticalPath.back();
    if (r.eft[last] != r.projectDuration)
        return fail(why, "critical path does not end at project duration");
    if (sum != r.projectDuration)
        return fail(why, "critical-path durations do not sum to project duration");
    return true;
}

bool validateResult(const Graph& g, const Result& r, string& why) {
    why.clear();

    if (g.n < 0 || (int)g.adj.size() != g.n ||
        (int)g.duration.size() != g.n)
        return fail(why, "graph dimensions are inconsistent");

    for (int u = 0; u < g.n; ++u) {
        if (g.duration[u] < 0)
            return fail(why, "graph contains a negative duration");
        for (int v : g.adj[u])
            if (v < 0 || v >= g.n)
                return fail(why, "graph contains an invalid edge endpoint");
    }

    if (r.hasCycle) {
        if (!validateCycle(g, r.cycle, why)) return false;
        if (!r.order.empty() || !r.est.empty() || !r.eft.empty() ||
            !r.lst.empty() || !r.lft.empty() || !r.slack.empty() ||
            !r.criticalPath.empty() || r.projectDuration != 0)
            return fail(why, "cyclic result must not contain schedule fields");
        return true;
    }

    if (!r.cycle.empty())
        return fail(why, "acyclic result must have an empty cycle");
    if (!validateOrder(g, r.order, why)) return false;

    auto sizeOK = [&](const vector<long long>& x) {
        return (int)x.size() == g.n;
    };
    if (!sizeOK(r.est) || !sizeOK(r.eft) || !sizeOK(r.lst) ||
        !sizeOK(r.lft) || !sizeOK(r.slack))
        return fail(why, "schedule vector has incorrect length");

    vector<vector<int>> pred(g.n), succ(g.n);
    for (int u = 0; u < g.n; ++u)
        for (int v : g.adj[u]) {
            succ[u].push_back(v);
            pred[v].push_back(u);
        }

    long long project = 0;
    for (int v = 0; v < g.n; ++v) {
        long long expectedEst = 0;
        for (int u : pred[v])
            expectedEst = max(expectedEst, r.est[u] + g.duration[u]);
        if (r.est[v] != expectedEst)
            return fail(why, "incorrect earliest start for vertex " + to_string(v));

        if (r.eft[v] != r.est[v] + g.duration[v])
            return fail(why, "incorrect earliest finish for vertex " + to_string(v));
        project = max(project, r.eft[v]);
    }

    if (r.projectDuration != project)
        return fail(why, "incorrect project duration");

    for (int v = 0; v < g.n; ++v) {
        long long expectedLft = r.projectDuration;
        if (!succ[v].empty()) {
            expectedLft = LLONG_MAX;
            for (int w : succ[v])
                expectedLft = min(expectedLft, r.lst[w]);
        }
        if (r.lft[v] != expectedLft)
            return fail(why, "incorrect latest finish for vertex " + to_string(v));
        if (r.lst[v] != r.lft[v] - g.duration[v])
            return fail(why, "incorrect latest start for vertex " + to_string(v));
        if (r.slack[v] != r.lst[v] - r.est[v])
            return fail(why, "incorrect slack for vertex " + to_string(v));
        if (r.slack[v] < 0)
            return fail(why, "negative slack for vertex " + to_string(v));
    }

    return validateCriticalPath(g, r, why);
}

bool validateDynamic(const Graph& g, const DynamicOrder& d, string& why) {
    why.clear();
    if ((int)d.ord.size() != g.n || (int)d.pos.size() != g.n)
        return fail(why, "dynamic order vectors have incorrect length");

    vector<char> seen(g.n, false);
    for (int i = 0; i < g.n; ++i) {
        int v = d.ord[i];
        if (v < 0 || v >= g.n)
            return fail(why, "dynamic order contains an invalid vertex");
        if (seen[v])
            return fail(why, "dynamic order contains a duplicate vertex");
        seen[v] = true;
        if (d.pos[v] != i)
            return fail(why, "pos and ord are not inverses");
    }

    for (int u = 0; u < g.n; ++u)
        for (int v : g.adj[u])
            if (d.pos[u] >= d.pos[v])
                return fail(why, "dynamic order violates edge " +
                                  to_string(u) + " -> " + to_string(v));
    return true;
}

bool resultsEquivalent(const Graph& g, const Result& got,
                       const Result& expected, string& why) {
    why.clear();

    if (got.hasCycle != expected.hasCycle)
        return fail(why, "hasCycle differs from Oracle");
    if (got.projectDuration != expected.projectDuration)
        return fail(why, "projectDuration differs from Oracle");

    if (got.hasCycle) {
        if (!validateCycle(g, got.cycle, why)) return false;
        return true;
    }

    if (got.est != expected.est)
        return fail(why, "earliest-start values differ from Oracle");
    if (got.lst != expected.lst)
        return fail(why, "latest-start values differ from Oracle");
    if (got.slack != expected.slack)
        return fail(why, "slack values differ from Oracle");

    if (!validateOrder(g, got.order, why)) return false;
    if (!validateCriticalPath(g, got, why)) return false;
    return true;
}
