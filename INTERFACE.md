# INTERFACE.md: Project 1, Build ordering and critical paths

Team Matrix. CS 253 / MA 253.
Written by the Architect before any structure was implemented.

This file is the contract between Structure A, Structure B, the oracle, the adversary, the application and the benchmarks. The headers in `include/` are its code form. If a header and this file disagree, this file is the specification and the header is the bug.

## 1. Conventions

- **Vertices** are targets, numbered `0 .. n-1`.
- **Edge `u -> v`** means target `u` must be built before target `v`. `adj[u]` holds the targets that depend on `u`. In the Makefile line `app: lib`, the edge is `lib -> app`.
- **Durations** are non-negative `int`. All derived times (earliest, latest, project duration) are `long long`, because a chain of 10^5 targets can exceed the range of `int`.
- **Duplicate edges** are allowed and behave like a single edge.
- **A self-loop `u -> u`** is a cycle. Its cycle is reported as `[u, u]`.
- **Empty graph** (`n = 0`): no cycle, empty order, empty schedule, `projectDuration = 0`.
- **Cycle format:** a closed walk that follows edge direction, with the first vertex repeated at the end, for example `[3, 7, 5, 3]`. It is simple: no vertex repeats except first = last.
- **Language and standard:** C++17. The structures and the oracle are written in the same language.

## 2. Header map (`include/`)

All headers are flat in `include/` and compiled with `-Iinclude`. They contain declarations only, except the small data types. Each uses an `#ifndef` guard.

| Header | Declares | Implemented in | Owner |
|---|---|---|---|
| `common.hpp` | `ll`, `NONE = -1`, `INF` | header only | Architect |
| `graph.hpp` | `Graph` | header only | Architect |
| `result.hpp` | `Result` | header only | Architect |
| `counters.hpp` | `Counters` | header only | Architect, agreed with Analyst |
| `dynamic_order.hpp` | `DynamicOrder`, `InsertResult` | header only | Architect |
| `interface.hpp` | `runStructureA`, `runStructureB`, `insertDependencyB`, `runOracle` | `src/structA/`, `src/structB/`, `oracle/` | Architect |
| `schedule.hpp` | `computeSchedule` | `app/` (or `src/common/`) | Architect |
| `validate.hpp` | `validateOrder`, `validateCycle`, `validateResult`, `validateDynamic`, `resultsEquivalent` | `oracle/` | Oracle owner |
| `io.hpp` | `readGraph`, `readGraphPlain`, `writeGraph`, `serializeResult`, `print*` | `app/` | Architect |
| `generators.hpp` | `genExponentialDAG`, `genHiddenCycleBehindChain`, `genWorstInsertions`, `genRandomDAG`, `genRandomGraph` | `adversary/` | Adversary |

## 3. Data types

### 3.1 `common.hpp`

- `using ll = long long;`
- `constexpr int NONE = -1;` marks "no vertex", "no parent", "unvisited".
- `constexpr ll INF = LLONG_MAX / 4;` is a sentinel that cannot overflow when two of them are added.

### 3.2 `graph.hpp`: `Graph`

| Member | Meaning |
|---|---|
| `int n` | number of targets |
| `vector<vector<int>> adj` | `adj[u]` lists targets that depend on `u` |
| `vector<int> duration` | `duration[u]` is the build time of `u` |
| `Graph(int vertices)` | creates `n` vertices, empty `adj`, all durations 0 |
| `addEdge(u, v)` | appends `v` to `adj[u]`; throws `std::out_of_range` if `u` or `v` is outside `0 .. n-1` |
| `edgeCount()` | total number of stored edges |

Structures treat the graph as read-only. The only function allowed to modify it is `insertDependencyB`.

### 3.3 `result.hpp`: `Result`

| Field | Type | Acyclic graph | Cyclic graph |
|---|---|---|---|
| `hasCycle` | `bool` | `false` | `true` |
| `order` | `vector<int>` | a topological order | empty |
| `cycle` | `vector<int>` | empty | one closed simple cycle |
| `est`, `eft` | `vector<ll>` | earliest start and finish | empty |
| `lst`, `lft` | `vector<ll>` | latest start and finish | empty |
| `slack` | `vector<ll>` | `lst - est` | empty |
| `criticalPath` | `vector<int>` | one critical path, in build order | empty |
| `projectDuration` | `ll` | shortest total time with unlimited workers | 0 |

### 3.4 `counters.hpp`: `Counters`

Optional work counters. Every structure takes a `Counters*`; `nullptr` means "do not count". Counting must not change the result. The Analyst's cost model (D6) is written in terms of these fields.

| Field | Increment it when |
|---|---|
| `edgesScanned` | an edge `u -> v` is examined (once per examination, in any loop) |
| `verticesVisited` | a vertex is dequeued, popped from the DFS stack for the first time, or otherwise processed |
| `heapPushes`, `heapPops` | an element is pushed to or popped from Structure A's heap |
| `comparisons` | two keys are compared in the heap |
| `verticesMoved` | a vertex changes position in an incremental insertion |
| `regionSize` | set at the end of each insertion to the number of vertices in the affected region |

`reset()` zeroes all fields.

### 3.5 `dynamic_order.hpp`

`DynamicOrder` is the state Structure B keeps between insertions.

| Field | Meaning |
|---|---|
| `ord` | `ord[i]` is the vertex at position `i` |
| `pos` | `pos[v]` is the position of vertex `v` |

Invariant: `pos[ord[i]] == i` for every `i`, and `ord` is a permutation of `0 .. n-1`.

`InsertResult` reports one insertion.

| Field | Meaning |
|---|---|
| `createdCycle` | `true` if `u -> v` would close a cycle |
| `cycle` | the closed simple cycle if `createdCycle`, otherwise empty |
| `lo`, `hi` | the affected region `[lo, hi]` in the old positions, `lo = pos[v]`, `hi = pos[u]` |
| `affected` | number of vertices examined in the region |
| `moved` | number of vertices whose position changed |

## 4. Function contracts

### 4.1 `interface.hpp`

```cpp
Result runStructureA(const Graph& g, bool lexSmallest = false, Counters* c = nullptr);
Result runStructureB(const Graph& g, DynamicOrder* out = nullptr, Counters* c = nullptr);
InsertResult insertDependencyB(Graph& g, DynamicOrder& d, int u, int v, Counters* c = nullptr);
Result runOracle(const Graph& g);
```

**`runStructureA`** (Structure A owner)
- Precondition: `g` satisfies the graph invariants in section 5.1.
- Computes the topological order with Kahn's algorithm.
- If `lexSmallest` is `false`, it uses a queue. If `true`, it uses a min-heap and returns the lexicographically smallest order by vertex id.
- If the algorithm halts with vertices remaining, the graph is cyclic. The function extracts an actual simple cycle by walking predecessors among the remaining vertices until a vertex repeats, and reports it in `cycle`. It never reports the leftover vertices, because they include vertices downstream of the cycle.
- Fills the schedule fields by calling `computeSchedule` with its own order.
- Complexity: O(V + E) with the queue, O(V log V + E) with the heap.
- The heap is Structure A's own code. `std::priority_queue` is not allowed.

**`runStructureB`** (Structure B owner)
- Precondition: as above.
- Computes the order with an iterative DFS (no recursion, so a chain of 10^5 does not overflow the stack). The order is the reverse of the finish order.
- On a back edge, extracts the cycle from the DFS stack and reports it in `cycle`.
- If the graph is acyclic and `out != nullptr`, fills `*out` so that `out->ord` equals `order` and `out->pos` is its inverse.
- Fills the schedule fields by calling `computeSchedule` with its own order.
- Complexity: O(V + E).

**`insertDependencyB`** (Structure B owner)
- Preconditions: `g` is acyclic, `d` is consistent with `g` (section 5.3), and `0 <= u, v < n`.
- If `pos[u] < pos[v]`, the order is already valid. Append the edge, return `affected = 0`, `moved = 0`.
- If `u == v`, report the cycle `[u, u]`.
- Otherwise, search only inside the region `[pos[v], pos[u]]`. If `v` reaches `u` there, set `createdCycle = true`, fill `cycle` with the path from `v` to `u` closed by the new edge, and leave `g` and `d` unchanged.
- If `v` does not reach `u`, move the vertices that reach `u` before the vertices reachable from `v`, inside the region only. Vertices outside the region do not move. Append the edge to `g` and update `d`.
- `g` is modified only when no cycle results. `counters->regionSize` and `verticesMoved` are set for every insertion, because the report needs them.
- Complexity: proportional to the size of the affected region and the edges inside it.

**`runOracle`** (Oracle owner)
- The slow, obviously correct reference. It repeatedly removes a zero in-degree vertex found by linear scan, O(V^2), always taking the smallest id, so its order is the lexicographically smallest one.
- Detects a cycle when no zero in-degree vertex remains, and extracts a simple cycle independently of A and B.
- Computes the schedule by its own naive method (for example, repeated relaxation until nothing changes), not by `computeSchedule`.
- Must not call `runStructureA`, `runStructureB`, `insertDependencyB` or `computeSchedule`.

### 4.2 `schedule.hpp`

```cpp
void computeSchedule(const Graph& g, const std::vector<int>& order, Result& r, Counters* c = nullptr);
```

- Preconditions: `g` is acyclic and `order` is a valid topological order of `g`.
- Forward pass in `order`: `est[v] = 0` for a vertex with no predecessor, otherwise the maximum over predecessors `u` of `est[u] + duration[u]`. Edges are relaxed in topological order while keeping the maximum (derivation D3). A shortest-path routine on negated weights is not used.
- `eft[v] = est[v] + duration[v]`, and `projectDuration` is the maximum `eft`.
- Backward pass in reverse order: `lft[v] = projectDuration` for a vertex with no successor, otherwise the minimum over successors `w` of `lst[w]`. `lst[v] = lft[v] - duration[v]`.
- `slack[v] = lst[v] - est[v]`.
- `criticalPath`: a chain of zero-slack vertices from a vertex with `est = 0` to a vertex with `eft = projectDuration`, where each consecutive pair is an edge with `est[next] = eft[prev]`. When several choices exist, the smallest vertex id is taken, so the output is deterministic.
- Does not touch `hasCycle`, `order` or `cycle`.
- Complexity: O(V + E).
- Used by A, B and the application. It is not used by the oracle.

### 4.3 `validate.hpp`

Every function returns `true` on success. On failure it returns `false` and writes a short reason to `why`. These functions depend only on `graph.hpp`, `result.hpp` and `dynamic_order.hpp`.

| Function | Checks |
|---|---|
| `validateOrder(g, order, why)` | `order` is a permutation of `0 .. n-1` and every edge `u -> v` has `u` before `v` |
| `validateCycle(g, cycle, why)` | closed (first = last), length at least 2, simple (no repeated interior vertex), every consecutive pair is an edge of `g` |
| `validateResult(g, r, why)` | the full invariants in section 5.2 for the acyclic or cyclic case |
| `validateDynamic(g, d, why)` | `pos[ord[i]] == i`, `ord` is a permutation, every edge of `g` is respected |
| `resultsEquivalent(g, got, expected, why)` | the equivalence contract in section 6 |

### 4.4 `io.hpp`

- `readGraph(in, g, errorMessage, names)`: reads the Makefile-like format of section 7. Returns `false` and fills `errorMessage` on malformed input. `names` (optional) receives the id-to-name mapping.
- `readGraphPlain(in, g, errorMessage)` and `writeGraph(out, g)`: the plain format of section 7, used by generators, tests and shrunk failing cases.
- `serializeResult(r)`: a canonical text form of a `Result`, used for diffing and for the hidden-test output.
- `printTopologicalOrder`, `printCycle`, `printSchedule`, `printSlack`, `printCriticalPath`, `printResult`: human-readable output. Each takes an `ostream&` (default `std::cout`) and an optional name table.

### 4.5 `generators.hpp`

All generators are deterministic for a given seed and depend only on `Graph`. They never call a structure.

| Function | Produces |
|---|---|
| `genExponentialDAG(n, seed)` | a DAG with exponentially many valid topological orders |
| `genHiddenCycleBehindChain(n, cycleLen, seed)` | a graph whose only cycle is reachable only through a long chain, so reporting leftover vertices gives chain vertices |
| `genWorstInsertions(n, seed, initial)` | fills `initial` with a starting DAG and returns an edge sequence in which every insertion forces a Theta(V) reorder |
| `genRandomDAG(n, m, skew, seed)` | a random DAG with controllable in-degree skew (`skew = 0` uniform) |
| `genRandomGraph(n, m, seed)` | a random graph that may contain cycles, for differential tests |

## 5. Structural invariants

The checkers in `validate.hpp` and the tests in `tests/` test exactly these.

### 5.1 Graph

1. `adj.size() == duration.size() == n`.
2. Every `v` in every `adj[u]` satisfies `0 <= v < n`.
3. Every `duration[u] >= 0`.

### 5.2 Result

**Acyclic result** (`hasCycle == false`)
1. `cycle` is empty.
2. `order` is a permutation of `0 .. n-1`, and for every edge `u -> v`, `u` appears before `v`.
3. `est[v] = 0` if `v` has no predecessor, otherwise `max(est[u] + duration[u])` over predecessors `u`.
4. `eft[v] = est[v] + duration[v]`, and `projectDuration = max eft[v]` (0 if `n = 0`).
5. `lft[v] = projectDuration` if `v` has no successor, otherwise `min(lst[w])` over successors `w`; `lst[v] = lft[v] - duration[v]`.
6. `slack[v] = lst[v] - est[v] >= 0` for every `v`.
7. The critical path starts at a vertex with `est = 0`, ends at a vertex with `eft = projectDuration`, every vertex on it has slack 0, every consecutive pair is an edge with `est[next] = eft[prev]`, and the durations along it sum to `projectDuration`.

**Cyclic result** (`hasCycle == true`)
1. `cycle` is closed (first = last), simple, and every consecutive pair is an edge of `g`.
2. `order`, `est`, `eft`, `lst`, `lft`, `slack` and `criticalPath` are empty, and `projectDuration = 0`.

### 5.3 DynamicOrder

1. `ord` is a permutation of `0 .. n-1` and `pos[ord[i]] == i`.
2. After every successful insertion, every edge of `g`, including the new one, goes from an earlier to a later position.
3. Vertices whose old position lies outside `[pos[v], pos[u]]` (old positions) do not move.
4. If `createdCycle` is `true`, `g` and `d` are exactly as before the call.

## 6. Equivalence contract for the differential test

Topological orders, critical paths and cycles are not unique, so equality is the wrong test for them. `resultsEquivalent` compares as follows.

| Item | How it is compared |
|---|---|
| `hasCycle` | equal |
| `projectDuration` | equal |
| `est`, `lst`, `slack` | equal (these are unique) |
| `order` | `validateOrder` on the structure's order; not compared with the oracle's order |
| `cycle` | `validateCycle` on the structure's cycle; not compared with the oracle's cycle |
| `criticalPath` | validated against invariant 7 of section 5.2; not compared for equality |
| A with `lexSmallest = true` | the order is unique, so it is compared exactly with the oracle's order |

After every insertion in the differential test, the harness runs `runOracle` on the same graph and compares it with the incrementally maintained result, and also calls `validateDynamic`.

## 7. Input formats

### 7.1 Makefile-like file (proposed; replaced by the instructor's format if one is published)

```
# a comment
<target> <duration> : <dependency> <dependency> ...
```

- One line per target. The dependencies may be empty.
- Names are non-empty strings without spaces. Ids are assigned in order of first appearance as a target.
- Every dependency must itself be declared as a target somewhere in the file. A dependency that is not declared is an error.
- A dependency `d` of target `t` gives the edge `d -> t`.
- Duration is a non-negative integer.

Example:

```
lib 3 :
util 2 :
app 5 : lib util
test 4 : app
```

This gives ids `lib=0, util=1, app=2, test=3` and edges `0->2, 1->2, 2->3`. `lib` finishes at 3 and `util` at 2, so `app` starts at 3 and finishes at 8, and `test` finishes at 12. The `projectDuration` is 12, the critical path is `lib, app, test`, and `util` has slack 1.

### 7.2 Plain format (generators and tests)

```
n m
d_0 d_1 ... d_{n-1}
u_1 v_1
...
u_m v_m
```

### 7.3 Interactive mode

The application reads one command per line: `add <target> <dependency>` inserts a dependency and prints the new order, or the cycle if one forms; `show` prints the current result; `quit` exits.

### 7.4 Evaluation format

The hidden tests call `./bin/run_ops --structure A|B < ops.txt > out.txt`. The exact input and output format will be copied here once the instructor publishes it for Project 1.

## 8. Error handling

| Situation | Behaviour |
|---|---|
| Malformed input file | `readGraph` returns `false` with a message naming the line |
| Undeclared dependency name | `readGraph` returns `false` |
| Negative duration | `readGraph` returns `false` |
| `addEdge` with an out-of-range vertex | throws `std::out_of_range` |
| `insertDependencyB` with an out-of-range vertex | precondition violation; the application checks the range before calling |
| Empty graph | valid; see section 1 |
| Cycle | not an error; reported in `Result` or `InsertResult` |

## 9. Library use

- Allowed in all code: `std::vector`, `std::string`, `std::pair`, `<iostream>` and similar basic containers and streams, and `<random>`.
- Banned for any structure under study: `std::priority_queue`, `std::map`, `std::set` and similar. Structure A's heap is written in `src/structA/`.
- Library containers are allowed in `tests/` and in data generation.
- To confirm: whether `std::queue` and `std::stack` are acceptable for Kahn's queue and the DFS stack. The Architect asks the instructor and records the answer here: **[pending]**.

## 10. Dependency rules

| Header | Structure A | Structure B | Oracle core | Differential harness | Adversary |
|---|---|---|---|---|---|
| `common.hpp` | yes | yes | yes | yes | yes |
| `graph.hpp` | yes | yes | yes | yes | yes |
| `result.hpp` | yes | yes | yes | yes | no |
| `counters.hpp` | yes | yes | no | no | no |
| `dynamic_order.hpp` | no | yes | yes | yes | no |
| `interface.hpp` | yes | yes | declaration only | yes | no |
| `schedule.hpp` | yes | yes | forbidden | no | no |
| `validate.hpp` | no | no | implements | yes | tests only |
| `io.hpp` | no | no | no | yes | optional |
| `generators.hpp` | no | no | no | yes | implements |

Rules:

1. A and B never call each other.
2. The oracle never calls `runStructureA`, `runStructureB`, `insertDependencyB` or `computeSchedule`. A CI step greps `oracle/` for these names.
3. The adversary's generators depend only on `Graph` and a seed. They never call a structure.
4. After c1, a change to any header goes through a pull request that every implementer reviews.

## 11. Implementation notes

- **Structure A owner:** when Kahn halts early, the remaining vertices include vertices downstream of the cycle. Build a local reverse adjacency for the remaining set and walk predecessors until a vertex repeats. A forward walk can dead-end in the chain.
- **Structure B owner:** use an explicit stack with a per-vertex edge index. Never modify `g` before knowing there is no cycle.
- **Oracle owner:** write the slow methods first and keep them simple. Their value is that they are obviously right.
- **Adversary:** each generator must come with a short argument, in derivation D5, that its output has the intended bad property by construction.
- **Analyst:** the `Counters` fields in section 3.4 are the variables of the predicted-cost model.