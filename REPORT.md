## 9. Predicted-cost model (written before any measurement)


### 9.1 Notation
- V = vertices, E = edges, d = E/V (average out-degree).
- i = index of the inserted edge u -> v; E_i = edges after i insertions,
  d_i = E_i / V.
- I_i = interval size = pos(u) - pos(v) + 1 (positions between v and u in the
  current order; only defined when pos(u) > pos(v)).
- AR_i = affected-region size = number of vertices actually visited by the
  forward search from v and the backward search from u (AR_i <= I_i).
  This is what the counter `affected_region_size` records; I_i is
  recorded in a second column.
- p = fraction of insertions with pos(u) > pos(v). The others need no
  reordering and cost only h0.
- ARbar = mean AR_i over the insertions that need reordering.
- Constants (seconds per unit of work, machine-dependent): a, a_h, q, b, g,
  s, h0. Their meaning is given where they are used.

### 9.2 Assumptions
1. Graphs are DAGs in adjacency-list form; V >= 10^5, E >= 10^6.
2. Cost is proportional to the number of vertices and edges touched; one
   constant per kind of operation. Cache and memory effects are ignored in
   this model (the graph is about 5 MB at E = 10^6 and may not fit in one
   core's L2 cache on the machine in section 10). Any gap is analysed in
   section 11 with profiler output.
3. Constants are fitted by least squares from small runs (V = 10^3 to 10^4)
   and then frozen. Large-run data is not used to adjust the model.
4. Prior guess, to be tested: g/b is between 1 and 3, because incremental
   maintenance does two searches plus a reorder per visited vertex, while a
   DFS pass does one.

### 9.3 Full algorithms
Structure A (Kahn, queue): each vertex is dequeued once and each edge
decrements one in-degree once.

    T_A = a * (V + E)

Structure A (min-heap version, lexicographically smallest order): every vertex
is pushed and popped once, each heap operation costs about log2 V, and only
vertices (not edges) enter the heap.

    T_A,heap = a * (V + E) + a_h * V * log2(V)

Critical path (forward pass for earliest start, backward pass for latest
start and slack, each relaxing every edge once):

    T_crit = q * (V + E)

Structure B (iterative DFS):

    T_B = b * (V + E)

Full recomputation after an insertion is B's DFS run from scratch, so

    T_full(i) = b * (V + E_i)

Cycle extraction happens once and costs at most O(V + E).

### 9.4 Incremental maintenance (Pearce-Kelly)

    pos(u) < pos(v):   T_inc(i) = h0
    pos(u) > pos(v):   T_inc(i) = h0 + g * AR_i * (1 + d_i) + s * AR_i * log2(AR_i)

The g-term is the scan of the visited vertices and their edges; the s-term is
sorting the visited positions before reassigning them. Averaged over all
insertions:

    T_inc(avg) = h0 + p * ( g * ARbar * (1 + d) + s * ARbar * log2(ARbar) )

### 9.5 Break-even between incremental and recomputation
Ignore h0 and the sort term (checked in section 11). Using V + E = V * (1 + d),
the factor (1 + d) cancels in both comparisons below.

**(a) The order must be valid after every insertion.** Recomputation is
cheaper for insertion i when

    g * AR_i * (1 + d_i)  >=  b * V * (1 + d_i)     i.e.     AR_i >= (b / g) * V

Since AR_i <= V, this can happen only if g >= b.

**(b) The order is needed only after m insertions.** Recomputing once costs
b * V * (1 + d). Maintaining incrementally costs m * p * g * ARbar * (1 + d).
Recomputation becomes cheaper after

    m*  =  b * V / ( p * g * ARbar )

m* is also the factor by which incremental maintenance is cheaper per
insertion. Illustration (not a prediction): V = 10^5, p = 0.5, g/b = 2,
ARbar = 100 gives m* = 1000 insertions.

**Predictions**
- **P1 (random insertions):** ARbar is much smaller than V, so m* is large and
  incremental is cheaper than recomputation at every insertion.
- **P2 (adversarial insertions, section 8):** AR_i is close to V, so
  T_inc(i) is about g*V*(1+d) + s*V*log2(V). If g >= b, recomputation is
  cheaper from the very first insertion (k* = 1). If g < b, incremental still
  wins on the g-term and the sort term decides.
- **P3 (growing graph):** ARbar(k) grows with the number of insertions k as
  the graph gets denser, so m* shrinks over a run.
  TODO: fit ARbar as a function of E_i from the small runs and state the
  predicted m* and k* for V = 10^5, E_0 = 10^6.

### 9.6 Predictions for A vs B (V = 10^5, E = 10^6)
- **P4:** Both are linear in V + E, so the slope on a log-log plot is about 1.
  The counters are exact: edges_relaxed = E for both A and B, whatever the
  in-degree distribution. Predicted time: TODO ms for A (queue), TODO ms for B.
- **P5:** The heap version is slower than the queue version by the factor
  1 + (a_h / a) * log2(V) / (1 + d). With log2(V) about 17, d = 10 and a_h / a
  between 1 and 2, this is about 2.5 to 4.
- **P6:** A and B differ by a roughly constant factor a/b. Any change of that
  gap with in-degree skew is outside this model and is attributed to cache
  behaviour, which requires profiler output (section 11).

### 9.7 How constants are estimated
- a, b, q: least squares of run time against V + E.
- a_h: least squares of the heap excess time against V * log2(V).
- g, s, h0: least squares of per-insertion time against AR_i * (1 + d_i),
  AR_i * log2(AR_i) and the no-reorder insertions.
Predicted curves in the figures use these frozen constants.

### 9.8 What could make the model wrong
1. Cache effects: A, B and incremental walk memory differently, so cost is not
   exactly proportional to items touched.
2. The sort term and h0, which are ignored in 9.5.
3. The prior g/b in [1, 3] (9.2) may be off; then P2 changes.
4. p and ARbar depend on how the generator picks edges; P1 holds only for the
   generator named in section 10.


### 9.9 Commit record
The model above was first committed in 0b516e1 (draft, 2026-10-29) and
completed in 4ea981e (sections 9.1 to 9.8, 2026-10-30), before any file
existed in results/raw/.

## 10. Experimental setup

All measurements in this report were taken on one machine, described below.

### 10.1 Hardware

| Item | Value |
|---|---|
| Processor | Intel Core i7-1355U (13th gen, hybrid) |
| Cores / threads | 10 cores (2 performance + 8 efficiency), 12 logical processors |
| Base clock | 1.70 GHz (turbo frequency varies during runs) |
| L1 cache (total) | 928 KB |
| L2 cache (total) | 6.5 MB |
| L3 cache | 12 MB, shared by all cores |
| RAM | 16 GB |

### 10.2 Software

| Item | Value |
|---|---|
| Host OS | Windows 11 Home, build 26300 |
| Measurement environment | WSL2, Ubuntu 26.04.1 LTS |
| Kernel | 6.18.54.1-microsoft-standard-WSL2 |
| Compiler | g++ 15.2.0 (Ubuntu 15.2.0-16ubuntu1) |
| Flags | TODO: agreed with the team |
| Profiler | valgrind 3.26.0, --tool=cachegrind |
| Python | 3.14.4 (matplotlib, pandas; figures only) |

### 10.3 Measurement protocol

- Timing uses std::chrono::steady_clock.
- Every timing is the mean of at least 5 runs; the spread (standard
  deviation) is reported as error bars. TODO: state the final number of runs.
- The random seed of every run is recorded in the CSV in results/raw/.
- The laptop was plugged in, set to the "Best performance" power mode, with
  other applications closed. TODO: confirm before the runs.
- Runs are not pinned to specific cores. WSL2 exposes the 12 logical
  processors as 6 identical cores, so performance and efficiency cores cannot
  be told apart from inside it. The operating system may therefore move a run
  between core types; this variation is part of the reported spread.
- Edge generator for the incremental experiments: TODO (name the Adversary's
  generators used; section 9.8 item 4 refers to this).
- Graph sizes: V = TODO and E = TODO for the main experiments (at least 10^5
  vertices and 10^6 edges); calibration runs for fitting the model constants
  used V = 10^3 to 10^4.

### 10.4 Limitations of the setup

- The machine is a hybrid laptop CPU whose clock frequency changes with
  temperature and power, so absolute times are not comparable with other
  machines. Ratios and scaling are the meaningful results.
- WSL2 runs inside a virtual machine. Hardware cache counters are usually
  unavailable there, so cache claims use the simulated counts from
  valgrind --tool=cachegrind.
- Under WSL2, the processor topology shown by lscpu (6 cores x 2 threads) is a
  simplification of the real hybrid layout (2 performance + 8 efficiency
  cores).
