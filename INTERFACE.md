# INTERFACE.md: Project 1, Build Ordering and Critical Paths

Team `<team-name>`. Written by the Architect: `<name, roll no., @github>`

This is the agreement the whole team codes against. It was written before any structure existed and is frozen at tag `c1`. If we want to change it later, we do it through a pull request that the Structure A, Structure B and Oracle owners all approve.

## 1. The short version

- Targets are numbered `0` to `V-1`. An edge `u -> v` means u has to be built before v.
- Every target has a duration, a whole number from 0 to 10^9.
- Structure A (Kahn) and Structure B (DFS with Pearce–Kelly) are used through one common class, `TopoStructure`. The rest of the code doesn't care which one it has.
- The main calls are `load`, `build`, `insert_edge` and `order`.
- When there's a cycle, we return an actual loop of targets. We don't return a chain that just leads into the loop.
- If an insert is rejected (cycle, duplicate or bad id), nothing changes.
- The critical path comes from one shared function, `compute_schedule`.
- Errors come back as status codes, not exceptions, and DFS never uses recursion.

## 2. Words we use

- **Order**: a list of all targets where `u` comes before `v` for every edge `u -> v`. `pos(x)` is where `x` sits in that list.
- **Cycle**: targets `c0 -> c1 -> ... -> ck-1 -> c0`, where every step is a real edge and no target repeats. A target with an edge to itself is a cycle of length 1.
- **I_i** (interval size): for an inserted edge `u -> v` with `pos(u) > pos(v)`, this is `pos(u) - pos(v) + 1`.
- **AR_i** (affected region): how many targets the search actually visited. For Structure B this is never more than `I_i`.

## 3. Types and the graph

```cpp
// include/types.hpp and include/graph.hpp
using VertexId = uint32_t;  using Position = uint32_t;  using Weight = int64_t;

enum class AddStatus { Added, Duplicate, InvalidVertex };

class Graph {
 public:
  explicit Graph(VertexId n = 0);
  VertexId n() const;   size_t m() const;
  Weight duration(VertexId v) const;
  bool set_duration(VertexId v, Weight d);          // false if out of range
  AddStatus add_edge(VertexId u, VertexId v);       // self-loops are stored on purpose
  void undo_last_add();                             // removes the last edge that was Added
  bool has_edge(VertexId u, VertexId v) const;
  const std::vector<VertexId>& out(VertexId u) const;   // in insertion order
  const std::vector<VertexId>& in(VertexId v) const;    // in insertion order
};
```

We keep both `out` and `in` lists because Pearce–Kelly has to search backward from `u`. Each structure holds its own copy of the graph, so nobody changes a graph they don't own.

## 4. What the calls return

```cpp
using Cycle = std::vector<VertexId>;     // any rotation of the same loop is fine

struct OrderResult {
  bool acyclic;                          // true: `order` is valid. false: `cycle` is valid.
  std::vector<VertexId> order;
  Cycle cycle;                           // always filled when acyclic is false
};

enum class InsertStatus { Inserted, Duplicate, WouldCycle, InvalidVertex, NotBuilt };

struct InsertResult {
  InsertStatus status;
  Cycle cycle;                           // only for WouldCycle: a loop through the new edge u->v
};

struct Schedule {
  std::vector<Weight> est, lst, slack;   // earliest start, latest start, slack = lst - est
  Weight makespan;                       // length of the critical path
  std::vector<VertexId> critical_path;
};
```

## 5. The common class

```cpp
struct Counters {
  uint64_t vertices_visited = 0, edges_relaxed = 0, heap_ops = 0;
  uint64_t affected_region_size = 0;   // AR_i of the last insert (0 if nothing was reordered)
  uint64_t interval_size = 0;          // I_i of the last insert (0 if nothing was reordered)
  uint64_t reordered = 0;              // how many targets moved in the last insert
};

class TopoStructure {
 public:
  virtual ~TopoStructure() = default;
  virtual void load(const Graph& g) = 0;                 // copies the graph, computes no order yet
  virtual OrderResult build() = 0;                       // full recompute from scratch
  virtual InsertResult insert_edge(VertexId u, VertexId v) = 0;
  virtual const std::vector<VertexId>& order() const = 0;    // order()[place] = target
  virtual Position position(VertexId v) const = 0;
  virtual const Graph& graph() const = 0;
  virtual const Counters& counters() const = 0;
  virtual void reset_counters() = 0;
  virtual const char* name() const = 0;                  // "structA-queue", "structA-heap", "structB"
  virtual std::string self_check() const = 0;            // "" if the internal arrays look fine
};

std::unique_ptr<TopoStructure> make_structA(bool min_heap_mode);
std::unique_ptr<TopoStructure> make_structB();
```

`order()` can be trusted only after `build()` returned `acyclic == true`, or after an insert that returned `Inserted`. If you call `insert_edge` before a successful `build()`, you get `NotBuilt`.

Here is how the two structures fill in the calls:

| Call | Structure A (Kahn) | Structure B (DFS and Pearce–Kelly) |
|---|---|---|
| `build()` | Kahn with a queue, or with a hand-written min-heap to get the smallest order. If it stops early, we pull a cycle out of the leftover targets. | Iterative DFS, then reverse finish order. The cycle comes from the back edge. |
| `insert_edge(u,v)` | If `pos(u) < pos(v)`, just store the edge. Otherwise add it and run `build()`. If that finds a cycle, call `undo_last_add()` and return `WouldCycle`. | If `pos(u) < pos(v)`, just store the edge. Otherwise search forward from `v` and backward from `u` inside `[pos(v), pos(u)]` and reorder only those targets. If `v` can reach `u`, return `WouldCycle`. |

## 6. Critical path

```cpp
Schedule compute_schedule(const Graph&, const std::vector<VertexId>& order, Counters* = nullptr);
```

It is declared in `include/` and written in `src/structA/`. It works with any valid order: one forward pass, one backward pass, and each edge is relaxed once per pass. We don't run a shortest-path routine on negated weights, and we don't recurse over paths.

- `est[v]` is 0 if `v` has no incoming edges. Otherwise it is the largest `est[p] + dur[p]` over the incoming edges `p -> v`.
- `makespan` is the largest `est[v] + dur[v]` (0 for an empty graph).
- `lst[v]` is `makespan - dur[v]` if `v` has no outgoing edges. Otherwise it is the smallest `lst[s]` over the outgoing edges `v -> s`, minus `dur[v]`.
- `slack = lst - est`, and it is never negative.
- The critical path goes from a source to a sink. Every target on it has slack 0, and every step is tight, meaning `est[next] == est[v] + dur[v]`.

## 7. Invariants: things that must always be true

The Oracle owner writes the checkers in `tests/invariants.hpp`. Each one returns `""` when all is well, or one line saying what went wrong. Each structure also writes its own `self_check()` for its internal arrays.

| ID | Rule |
|---|---|
| G | The graph is consistent: `out` and `in` mirror each other, there are no duplicate edges, and `m()` matches the degree sums. |
| O1 | `order()` contains every target exactly once. |
| O2 | `position(order()[i]) == i`. |
| O3 | For every edge `u -> v`, `position(u) < position(v)`. |
| C1 | A returned cycle is simple, closed, and uses real edges. |
| C2 | The cycle from `WouldCycle` contains the new edge `u -> v`. |
| R1 | After `Duplicate`, `WouldCycle`, `InvalidVertex` or `NotBuilt`, the graph and the order are exactly what they were before the call. |
| R2 | **(B)** After a reordering insert, targets outside `[pos_old(v), pos_old(u)]` keep their positions. |
| R3 | **(B)** Targets outside the affected region keep their relative order. |
| R4 | When reordering, `I == pos_old(u) - pos_old(v) + 1`; otherwise `AR = I = 0`. **(B)** also has `AR <= I`. A recomputes everything, so its `AR` is `V`. |
| A1 | **(A, heap mode)** The order equals the oracle's order exactly. |
| S1 | `est`, `lst`, `slack` and `makespan` equal the oracle's values exactly. |
| S2 | `critical_path` runs source to sink, every step is tight, and its total duration equals `makespan`. |
| K1 | After a `build()` that returns `acyclic == true`, `edges_relaxed == m()` (A in queue mode, and B). A cyclic graph stops early, so we don't require it there. |

Two coding rules go with these. First, A must save the old order before the trial `build()` inside `insert_edge`, and restore it if the result is `WouldCycle` (that is what R1 needs). Second, nothing recurses, because a chain of 10^6 targets has to work. Cycles are cut at the first repeated target, so chain vertices never end up in them.

## 8. Edge cases

| Case | What must happen |
|---|---|
| `V = 0` | Acyclic, empty order, makespan 0. |
| Isolated or disconnected targets | Allowed. The order covers everything, and the makespan is the longest component. |
| Duplicate edge | `Duplicate`, nothing changes. |
| Self-loop in the loaded graph | `build()` gives the cycle `[u]`. |
| Self-loop inserted | `WouldCycle`, cycle `[u]`, nothing changes. |
| Target id `>= n()` | `InvalidVertex`, nothing changes. |
| Loaded graph already has a cycle | `build()` gives a valid cycle, and a later `insert_edge` gives `NotBuilt`. |
| Cycle behind a long chain | The reported cycle contains only cycle targets. |
| Insert with `pos(u) < pos(v)` | `Inserted`, order unchanged, `AR = I = 0`. |
| Insert with `pos(u) > pos(v)` | Reorder if there's no cycle, otherwise `WouldCycle`. |
| Zero durations | Allowed. The tie rule in §9 picks one critical path. |

## 9. Tie rules

Many different orders can be valid, so most tests only check that the result is valid. We compare for equality only where there is exactly one right answer.

- **Kahn, queue:** start with the sources in increasing id, and scan edges in insertion order.
- **Kahn, heap:** always take the smallest id. This gives the one lexicographically smallest order.
- **DFS:** try roots in increasing id, scan edges in insertion order, and take the reverse of finish order.
- **Pearce–Kelly:** inside the affected region, the group that reaches `u` goes first, then the group reachable from `v`. Each group keeps its discovery order, and together they reuse the same set of positions.
- **Critical path:** start at the slack-0 source with the smallest id. At each step go to the smallest-id successor `s` with slack 0 and `est[s] == est[v] + dur[v]`, and stop at a sink.

## 10. Counters and the cost model

| Counter | Meaning | Symbol in REPORT §9 |
|---|---|---|
| `vertices_visited` | targets entered | `V` for a build, `AR` for an insert |
| `edges_relaxed` | edges examined | `E` |
| `heap_ops` | heap pushes and pops | the `V log V` term |
| `affected_region_size` | `AR_i` | `AR_i` |
| `interval_size` | `I_i` | `I_i` |

Counters record real work, never estimates, and only the benchmark measures time. The benchmark writes one CSV row per insert: `i, E_i, pos_u, pos_v, interval_size, affected_region_size, status, time_ns_inc, time_ns_full`.

## 11. The application

**Input file.** One target per line: `<target> [<duration>] : <dep> <dep> ...`. The default duration is 1, `#` starts a comment, and `a : b c` makes the edges `b -> a` and `c -> a`.

```
parse   3 :
assets  4 :
compile 5 : parse
link    2 : compile assets
```

**Run it:** `./bin/app --input FILE [--structure A|B] [--heap] [--interactive]`

**Output:** `ORDER:`, `START:`, `LATEST:`, `SLACK:`, `MAKESPAN: 10` and `CRITICAL: parse -> compile -> link`. A cycle prints `CYCLE: a -> b -> c -> a` and exits with status 2. Other errors go to stderr with status 1.

**Interactive commands:** `add u v` (answers `OK`, `DUPLICATE`, `INVALID` or `CYCLE: ...`), `order`, `schedule`, `critical`, `stats` and `quit`.

**Hidden-test program:** `./bin/run_ops --structure A|B < ops.txt > out.txt`. The instructor's published format takes priority. Until it comes out, we use a provisional one: a first line `V E0`, a line of durations, `E0` edge lines, then `Q`, then the operations `BUILD`, `INSERT u v`, `ORDER` and `SCHEDULE`. It is parsed in one file, `app/ops_io.cpp`, so switching formats is easy.

## 12. Rules every module follows

- The oracle is written independently of A and B. It never calls their code, so it can catch their mistakes.
- Structures A and B only compute. They don't read files, print, or recurse. Reading input and printing belong to `app/`.
- In `src/`, `app/` and `oracle/` we only use `std::vector`, `std::string`, `<cstdint>`, and `std::swap`/`std::reverse`. These are not allowed: `std::priority_queue`, `map`, `set`, `unordered_*`, `queue`, `deque`, and `std::sort` on data under study. Tests and data generation can use anything.
