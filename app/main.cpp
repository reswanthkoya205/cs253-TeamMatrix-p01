#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "common.hpp"
#include "counters.hpp"
#include "dynamic_order.hpp"
#include "graph.hpp"
#include "interface.hpp"
#include "io.hpp"
#include "result.hpp"
#include "schedule.hpp"

namespace {

struct Options {
    std::string input;
    bool useB = false;
    bool heap = false;
    bool interactive = false;
};

struct Session {
    Graph g;
    std::vector<std::string> names;
    bool useB = false;
    bool heap = false;
    Counters c;
    DynamicOrder d;
    Result r;
    InsertResult last;
    bool haveLast = false;
};

void usage(std::ostream& os) {
    os << "usage: app --input FILE [--structure A|B] [--heap] [--interactive]\n";
}

bool parseArgs(int argc, char** argv, Options& o, std::string& err) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--input" || a == "--structure") {
            if (i + 1 >= argc) {
                err = a + " needs a value";
                return false;
            }
            std::string val = argv[++i];
            if (a == "--input") {
                o.input = val;
            } else if (val == "A" || val == "a") {
                o.useB = false;
            } else if (val == "B" || val == "b") {
                o.useB = true;
            } else {
                err = "--structure must be A or B";
                return false;
            }
        } else if (a == "--heap") {
            o.heap = true;
        } else if (a == "--interactive") {
            o.interactive = true;
        } else {
            err = "unknown option '" + a + "'";
            return false;
        }
    }
    if (o.input.empty()) {
        err = "--input is required";
        return false;
    }
    if (o.heap && o.useB) {
        err = "--heap only applies to --structure A";
        return false;
    }
    return true;
}

int findId(const std::vector<std::string>& names, const std::string& s) {
    for (std::size_t i = 0; i < names.size(); ++i)
        if (names[i] == s) return static_cast<int>(i);
    return -1;
}

void printNamed(std::ostream& os, const char* label, const std::vector<ll>& v,
                const std::vector<std::string>& names) {
    os << label;
    for (std::size_t i = 0; i < v.size(); ++i) os << ' ' << names[i] << '=' << v[i];
    os << '\n';
}

void printScheduleNamed(std::ostream& os, const Result& r, const std::vector<std::string>& names) {
    printNamed(os, "START:", r.est, names);
    printNamed(os, "LATEST:", r.lst, names);
    printNamed(os, "SLACK:", r.slack, names);
}

void printAnalysis(std::ostream& os, const Result& r, const std::vector<std::string>& names) {
    printTopologicalOrder(r, os, &names);
    printScheduleNamed(os, r, names);
    printCriticalPath(r, os, &names);
}

void printCycleOf(std::ostream& os, const std::vector<int>& cycle, const std::vector<std::string>& names) {
    Result t;
    t.hasCycle = true;
    t.cycle = cycle;
    printCycle(t, os, &names);
}

void addCommand(Session& s, const std::string& from, const std::string& to, std::ostream& os) {
    const int u = findId(s.names, from);
    const int v = findId(s.names, to);
    if (u < 0 || v < 0) {
        os << "INVALID\n";
        return;
    }
    for (std::size_t k = 0; k < s.g.adj[u].size(); ++k) {
        if (s.g.adj[u][k] == v) {
            os << "DUPLICATE\n";
            return;
        }
    }
    s.c.reset();
    if (s.useB) {
        InsertResult ir = insertDependencyB(s.g, s.d, u, v, &s.c);
        if (ir.createdCycle) {
            printCycleOf(os, ir.cycle, s.names);
            return;
        }
        s.r.hasCycle = false;
        s.r.order = s.d.ord;
        computeSchedule(s.g, s.d.ord, s.r, &s.c);
        s.last = ir;
        s.haveLast = true;
    } else {
        Graph trial = s.g;
        trial.addEdge(u, v);
        Result nr = runStructureA(trial, s.heap, &s.c);
        if (nr.hasCycle) {
            printCycleOf(os, nr.cycle, s.names);
            return;
        }
        s.g = trial;
        s.r = nr;
    }
    os << "OK\n";
}

void statsCommand(const Session& s, std::ostream& os) {
    os << "STATS: edgesScanned=" << s.c.edgesScanned << " verticesVisited=" << s.c.verticesVisited
       << " heapPushes=" << s.c.heapPushes << " heapPops=" << s.c.heapPops
       << " comparisons=" << s.c.comparisons << " verticesMoved=" << s.c.verticesMoved
       << " regionSize=" << s.c.regionSize << '\n';
    if (s.useB && s.haveLast) {
        os << "LAST_INSERT: region=[" << s.last.lo << ',' << s.last.hi << "] affected="
           << s.last.affected << " moved=" << s.last.moved << '\n';
    }
}

void interactiveLoop(Session& s, std::istream& in, std::ostream& os) {
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream ls(line);
        std::string cmd;
        if (!(ls >> cmd)) continue;
        if (cmd == "quit" || cmd == "exit") break;
        if (cmd == "add") {
            std::string a, b, extra;
            if (!(ls >> a >> b) || (ls >> extra)) {
                os << "ERROR: usage: add <from> <to>\n";
                continue;
            }
            addCommand(s, a, b, os);
        } else if (cmd == "order") {
            printTopologicalOrder(s.r, os, &s.names);
        } else if (cmd == "schedule") {
            printScheduleNamed(os, s.r, s.names);
        } else if (cmd == "critical") {
            printCriticalPath(s.r, os, &s.names);
        } else if (cmd == "stats") {
            statsCommand(s, os);
        } else {
            os << "ERROR: unknown command '" << cmd << "'\n";
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    Options opt;
    std::string err;
    if (!parseArgs(argc, argv, opt, err)) {
        std::cerr << "error: " << err << '\n';
        usage(std::cerr);
        return 1;
    }

    std::ifstream file;
    std::istream* in = &std::cin;
    if (opt.input != "-") {
        file.open(opt.input.c_str());
        if (!file) {
            std::cerr << "error: cannot open '" << opt.input << "'\n";
            return 1;
        }
        in = &file;
    }

    Session s;
    s.useB = opt.useB;
    s.heap = opt.heap;
    if (!readGraph(*in, s.g, err, &s.names)) {
        std::cerr << "error: " << err << '\n';
        return 1;
    }

    s.r = s.useB ? runStructureB(s.g, &s.d, &s.c) : runStructureA(s.g, s.heap, &s.c);
    if (s.r.hasCycle) {
        printCycle(s.r, std::cout, &s.names);
        return 2;
    }

    printAnalysis(std::cout, s.r, s.names);
    if (opt.interactive) interactiveLoop(s, std::cin, std::cout);
    return 0;
}