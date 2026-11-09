// adversary/generators.cpp
// Full implementation of adversary generators declared in include/generators.hpp.
// Uses standard C++17 library containers for test data generation.
#include "generators.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <stdexcept>
#include <unordered_set>

namespace {

// Deterministic 64-bit RNG ensuring identical output across all systems/compilers
struct Rng {
    std::mt19937_64 engine;

    explicit Rng(uint64_t seed) : engine(seed) {}

    uint64_t below(uint64_t n) {
        return engine() % n;
    }

    int range(int lo, int hi) {
        return lo + static_cast<int>(below(static_cast<uint64_t>(hi - lo + 1)));
    }
};

// Maps (u, v) vertex pair into a unique 64-bit lookup key for unordered_set
inline uint64_t edge_key(int u, int v) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(u)) << 32) | static_cast<uint32_t>(v);
}

// Assigns random build durations in range [1, maxdur] to graph targets
void assign_durations(Graph& g, Rng& rng, int maxdur = 100) {
    for (int i = 0; i < g.n; ++i) {
        g.duration[i] = rng.range(1, maxdur);
    }
}

} // anonymous namespace

// -----------------------------------------------------------------------------
// ADVERSARIAL TARGET 1: Exponentially Many Topological Orders
// -----------------------------------------------------------------------------
Graph genExponentialDAG(int n, uint64_t seed, const std::string& mode, int width) {
    if (n < 2) {
        throw std::invalid_argument("n must be at least 2");
    }

    Graph g(n);
    Rng rng(seed);

    if (mode == "layers") {
        if (width < 1) throw std::invalid_argument("width must be >= 1");
        int L = (n + width - 1) / width;
        auto layer_begin = [width](int i) { return i * width; };
        auto layer_end = [n, width](int i) { return std::min(n, (i + 1) * width); };

        for (int i = 0; i + 1 < L; ++i) {
            for (int u = layer_begin(i); u < layer_end(i); ++u) {
                for (int v = layer_begin(i + 1); v < layer_end(i + 1); ++v) {
                    g.addEdge(u, v);
                }
            }
        }
    } else if (mode == "chains") {
        int k = std::max(2, width);
        for (int x = 0; x + k < n; ++x) {
            g.addEdge(x, x + k);
        }
    } else if (mode == "antichain") {
        // No edges (n! valid orders)
    } else {
        throw std::invalid_argument("Unknown mode: " + mode);
    }

    assign_durations(g, rng);
    return g;
}

// -----------------------------------------------------------------------------
// ADVERSARIAL TARGET 2: Hidden Cycle Behind a Deep Chain
// -----------------------------------------------------------------------------
Graph genHiddenCycleBehindChain(int n, int cycleLen, uint64_t seed, int headLen) {
    int R = cycleLen;
    if (R < 2) throw std::invalid_argument("cycleLen must be >= 2");

    int H = (headLen >= 0) ? headLen : (n - R) / 2;
    int T = n - H - R;

    if (H < 0 || T < 0 || H + R + T != n) {
        throw std::invalid_argument("Invalid graph segment partition sizes");
    }

    Graph g(n);
    Rng rng(seed);

    // 1. Head chain: h0 -> h1 -> ... -> h(H-1)
    for (int i = 0; i + 1 < H; ++i) {
        g.addEdge(i, i + 1);
    }
    if (H > 0) g.addEdge(H - 1, H); // Connect head to c0

    // 2. The cycle: c0 -> c1 -> ... -> c(R-1) -> c0
    for (int j = 0; j < R; ++j) {
        g.addEdge(H + j, H + (j + 1) % R);
    }

    // 3. Tail chain: c(R-1) -> t0 -> ... -> t(T-1)
    if (T > 0) g.addEdge(H + R - 1, H + R);
    for (int i = 0; i + 1 < T; ++i) {
        g.addEdge(H + R + i, H + R + i + 1);
    }

    assign_durations(g, rng);
    return g;
}

// -----------------------------------------------------------------------------
// ADVERSARIAL TARGET 3: Worst-Case Incremental Insertions
// -----------------------------------------------------------------------------
InsertionSequence genWorstInsertions(int n, uint64_t seed, int chainLen) {
    if (n < 4) throw std::invalid_argument("n must be >= 4");
    int K = (chainLen >= 2 && chainLen < n) ? chainLen : n / 2;

    InsertionSequence seq;
    seq.initialGraph = Graph(n);
    Rng rng(seed);

    // Initial DAG: chain c_0 -> c_1 -> ... -> c_{K-1}
    for (int i = 0; i + 1 < K; ++i) {
        seq.initialGraph.addEdge(i, i + 1);
    }

    // Insertions w_j -> c_0 for free vertices w_j = K..n-1
    // Forces shifting K = Theta(V) vertices on every single insertion
    for (int j = K; j < n; ++j) {
        seq.edgesToInsert.push_back({j, 0});
    }

    assign_durations(seq.initialGraph, rng);
    return seq;
}

// -----------------------------------------------------------------------------
// RANDOM DAG GENERATOR
// -----------------------------------------------------------------------------
Graph genRandomDAG(int n, int m, double skew, uint64_t seed) {
    Graph g(n);
    Rng rng(seed);
    std::unordered_set<uint64_t> seen;

    int attempts = 0;
    while (g.edgeCount() < m && attempts < m * 50 + 1000) {
        attempts++;
        int u = static_cast<int>(rng.below(n));
        int v = static_cast<int>(rng.below(n));

        if (u >= v) std::swap(u, v); // Enforces DAG invariant u < v
        if (u == v) continue;

        if (skew > 0.0) {
            double r = static_cast<double>(rng.below(100000)) / 100000.0;
            u = static_cast<int>(std::pow(r, 1.0 + skew) * n);
            u = std::clamp(u, 0, n - 1);
            if (u >= v) continue;
        }

        if (seen.insert(edge_key(u, v)).second) {
            g.addEdge(u, v);
        }
    }

    assign_durations(g, rng);
    return g;
}

// -----------------------------------------------------------------------------
// RANDOM GENERAL GRAPH GENERATOR
// -----------------------------------------------------------------------------
Graph genRandomGraph(int n, int m, uint64_t seed) {
    Graph g(n);
    Rng rng(seed);
    std::unordered_set<uint64_t> seen;

    int attempts = 0;
    while (g.edgeCount() < m && attempts < m * 50 + 1000) {
        attempts++;
        int u = static_cast<int>(rng.below(n));
        int v = static_cast<int>(rng.below(n));
        if (u == v) continue;

        if (seen.insert(edge_key(u, v)).second) {
            g.addEdge(u, v);
        }
    }

    assign_durations(g, rng);
    return g;
}