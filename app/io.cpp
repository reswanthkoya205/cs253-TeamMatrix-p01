#include "io.hpp"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <sstream>
#include <utility>

namespace {

std::string nameOf(int v, const std::vector<std::string>* names) {
    if (names && v >= 0 && static_cast<std::size_t>(v) < names->size()) return (*names)[v];
    return std::to_string(v);
}

bool parseDuration(const std::string& s, int& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    long long x = std::strtoll(s.c_str(), &end, 10);
    if (*end != '\0' || x < 0 || x > 1000000000LL) return false;
    out = static_cast<int>(x);
    return true;
}

std::vector<std::string> tokens(const std::string& s) {
    std::vector<std::string> t;
    std::istringstream is(s);
    std::string w;
    while (is >> w) t.push_back(w);
    return t;
}

struct Decl {
    std::string name;
    int duration;
    std::vector<std::string> deps;
    int lineNo;
};

bool fail(std::string& errorMessage, int lineNo, const std::string& msg) {
    errorMessage = "line " + std::to_string(lineNo) + ": " + msg;
    return false;
}

void printList(std::ostream& os, const char* label, const std::vector<int>& v,
               const char* sep, const std::vector<std::string>* names) {
    os << label;
    for (std::size_t i = 0; i < v.size(); ++i) {
        os << (i == 0 ? " " : sep) << nameOf(v[i], names);
    }
    os << '\n';
}

void printValues(std::ostream& os, const char* label, const std::vector<ll>& v) {
    os << label;
    for (std::size_t i = 0; i < v.size(); ++i) os << ' ' << i << '=' << v[i];
    os << '\n';
}

}  // namespace

bool readGraph(std::istream& in, Graph& g, std::string& errorMessage,
               std::vector<std::string>* names) {
    std::vector<Decl> decls;
    std::string raw;
    int lineNo = 0;

    while (std::getline(in, raw)) {
        ++lineNo;
        std::size_t hash = raw.find('#');
        if (hash != std::string::npos) raw.erase(hash);
        if (tokens(raw).empty()) continue;

        std::size_t colon = raw.find(':');
        if (colon == std::string::npos) return fail(errorMessage, lineNo, "missing ':'");
        std::string right = raw.substr(colon + 1);
        if (right.find(':') != std::string::npos) return fail(errorMessage, lineNo, "more than one ':'");

        std::vector<std::string> left = tokens(raw.substr(0, colon));
        if (left.empty() || left.size() > 2)
            return fail(errorMessage, lineNo, "expected '<target> [<duration>] : <dependencies>'");

        Decl d;
        d.name = left[0];
        d.duration = 1;
        d.lineNo = lineNo;
        if (left.size() == 2 && !parseDuration(left[1], d.duration))
            return fail(errorMessage, lineNo, "bad duration '" + left[1] + "'");
        d.deps = tokens(right);
        decls.push_back(d);
    }

    const int n = static_cast<int>(decls.size());
    std::vector<std::pair<std::string, int> > index;
    index.reserve(decls.size());
    for (int i = 0; i < n; ++i) index.push_back(std::make_pair(decls[i].name, i));
    std::sort(index.begin(), index.end());
    for (int i = 1; i < n; ++i) {
        if (index[i].first == index[i - 1].first)
            return fail(errorMessage, decls[index[i].second].lineNo,
                        "target '" + index[i].first + "' already declared on line " +
                            std::to_string(decls[index[i - 1].second].lineNo));
    }

    Graph tmp(n);
    for (int i = 0; i < n; ++i) {
        tmp.duration[i] = decls[i].duration;
        for (std::size_t k = 0; k < decls[i].deps.size(); ++k) {
            const std::string& dep = decls[i].deps[k];
            std::vector<std::pair<std::string, int> >::const_iterator it =
                std::lower_bound(index.begin(), index.end(), std::make_pair(dep, 0));
            if (it == index.end() || it->first != dep)
                return fail(errorMessage, decls[i].lineNo, "unknown dependency '" + dep + "'");
            tmp.addEdge(it->second, i);
        }
    }

    g = tmp;
    if (names) {
        names->clear();
        for (int i = 0; i < n; ++i) names->push_back(decls[i].name);
    }
    return true;
}

bool readGraphPlain(std::istream& in, Graph& g, std::string& errorMessage) {
    long long n = 0, m = 0;
    if (!(in >> n >> m)) {
        errorMessage = "expected 'n m'";
        return false;
    }
    if (n < 0 || m < 0 || n > 100000000LL) {
        errorMessage = "n or m out of range";
        return false;
    }
    Graph tmp(static_cast<int>(n));
    for (long long i = 0; i < n; ++i) {
        long long d;
        if (!(in >> d)) {
            errorMessage = "expected " + std::to_string(n) + " durations";
            return false;
        }
        if (d < 0 || d > 1000000000LL) {
            errorMessage = "duration of vertex " + std::to_string(i) + " out of range";
            return false;
        }
        tmp.duration[i] = static_cast<int>(d);
    }
    for (long long j = 0; j < m; ++j) {
        long long u, v;
        if (!(in >> u >> v)) {
            errorMessage = "expected " + std::to_string(m) + " edges";
            return false;
        }
        if (u < 0 || v < 0 || u >= n || v >= n) {
            errorMessage = "edge " + std::to_string(j) + " has a vertex outside 0.." + std::to_string(n - 1);
            return false;
        }
        tmp.addEdge(static_cast<int>(u), static_cast<int>(v));
    }
    g = tmp;
    return true;
}

void writeGraph(std::ostream& out, const Graph& g) {
    out << g.n << ' ' << g.edgeCount() << '\n';
    for (int i = 0; i < g.n; ++i) out << (i ? " " : "") << g.duration[i];
    out << '\n';
    for (int u = 0; u < g.n; ++u)
        for (std::size_t k = 0; k < g.adj[u].size(); ++k) out << u << ' ' << g.adj[u][k] << '\n';
}

std::string serializeResult(const Result& r) {
    std::ostringstream s;
    if (r.hasCycle) {
        s << "CYCLE:";
        for (std::size_t i = 0; i < r.cycle.size(); ++i) s << ' ' << r.cycle[i];
        s << '\n';
        return s.str();
    }
    s << "ORDER:";
    for (std::size_t i = 0; i < r.order.size(); ++i) s << ' ' << r.order[i];
    s << '\n';
    s << "EST:";
    for (std::size_t i = 0; i < r.est.size(); ++i) s << ' ' << r.est[i];
    s << '\n';
    s << "LST:";
    for (std::size_t i = 0; i < r.lst.size(); ++i) s << ' ' << r.lst[i];
    s << '\n';
    s << "SLACK:";
    for (std::size_t i = 0; i < r.slack.size(); ++i) s << ' ' << r.slack[i];
    s << '\n';
    s << "MAKESPAN: " << r.projectDuration << '\n';
    s << "CRITICAL:";
    for (std::size_t i = 0; i < r.criticalPath.size(); ++i) s << ' ' << r.criticalPath[i];
    s << '\n';
    return s.str();
}

void printTopologicalOrder(const Result& r, std::ostream& os, const std::vector<std::string>* names) {
    printList(os, "ORDER:", r.order, " ", names);
}

void printCycle(const Result& r, std::ostream& os, const std::vector<std::string>* names) {
    printList(os, "CYCLE:", r.cycle, " -> ", names);
}

void printSchedule(const Result& r, std::ostream& os) {
    printValues(os, "START:", r.est);
    printValues(os, "FINISH:", r.eft);
    printValues(os, "LATEST:", r.lst);
    printValues(os, "LATEST_FINISH:", r.lft);
}

void printSlack(const Result& r, std::ostream& os) {
    printValues(os, "SLACK:", r.slack);
}

void printCriticalPath(const Result& r, std::ostream& os, const std::vector<std::string>* names) {
    os << "MAKESPAN: " << r.projectDuration << '\n';
    printList(os, "CRITICAL:", r.criticalPath, " -> ", names);
}

void printResult(const Result& r, std::ostream& os, const std::vector<std::string>* names) {
    if (r.hasCycle) {
        printCycle(r, os, names);
        return;
    }
    printTopologicalOrder(r, os, names);
    printSchedule(r, os);
    printSlack(r, os);
    printCriticalPath(r, os, names);
}