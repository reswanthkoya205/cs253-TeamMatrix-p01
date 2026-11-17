Executive Summary
TeamMatrix implemented a build-ordering and critical-path analysis system for directed dependency graphs. A vertex represents a build target, and an edge u → v means that u must be completed before v. The system computes a valid topological order, detects and returns a concrete directed cycle when one exists, and—when the graph is acyclic—computes earliest and latest schedule times, slack, project duration, and one critical path.
The project compares two static ordering strategies. Structure A uses Kahn’s indegree-elimination algorithm, with either a FIFO queue or a custom binary min-heap. Structure B uses iterative depth-first search (DFS) and reverse finishing order. Structure B additionally maintains its order under edge insertions using a Pearce–Kelly-style two-way search and local reorder operation.
The implementation is supported by an independent, deliberately slow oracle, structural validators, differential tests, adversarial graph generators, and benchmark programs. The uploaded repository’s test suite was rebuilt and executed during report preparation: all seven test executables and the differential test passed, including the large-scale test that reports a 100,000-vertex graph and 1,099,934 dependency operations.
The report also records important evidence limitations. The static benchmark CSV contains only V = 1,000, 3,000, and 5,000 with E = 2V; it does not establish performance at the 10^5-vertex scale proposed in the original cost-model assumptions. More importantly, the checked-in worst-insertion benchmark data reports an affected-region size of zero for all 950 insertions. Inspection explains why: the current DFS initialization places the disconnected free vertices before the chain in reverse finish order, so the generated edge sequence does not invalidate the initial order. The figure must therefore not be interpreted as a measurement of expensive Pearce–Kelly reorders.
The central design conclusion is that Pearce–Kelly was selected for Structure B because its two-way, affected-set discovery and local reassignment are straightforward to map to the project’s DynamicOrder representation and are reported by the authors to perform particularly well on sparse digraphs in practice. This is not a claim that Pearce–Kelly has the strongest asymptotic bound: the original paper explicitly acknowledges inferior time complexity to some alternatives, and MNR has an O(V) amortized insertion bound from an initially empty graph.
·       Structure A: Kahn’s algorithm; O(V + E) with FIFO queue and O(V log V + E) with the custom min-heap.
·       Structure B initial order: iterative DFS; O(V + E), with an explicit stack to avoid recursion-depth failure.
·       Structure B updates: Pearce–Kelly-style bounded forward and backward searches, cycle check, and local reorder.
·       Schedule computation: two topological passes; O(V + E).
·       Validation: independent oracle, invariant checkers, and differential tests.
1. Team and Contributions
The repository identifies the project as “Project 01: Build Ordering and Critical Paths,” TeamMatrix, CS 253 / MA 253, Autumn 2026, Indian Institute of Technology Indore. The role allocation below is taken from README.md. Each member’s derivation is a separate PDF under docs/derivations/ and is not counted toward the main report length, consistent with the submission instructions.
The roles describe ownership areas rather than an assertion that a member worked only on one file. The Architect defined the shared interface and integrated the application; the Oracle owner supplied independent correctness checks; the Adversary supplied targeted graph families; and the Analyst prepared the cost model and measurement harness.
Member
Roll number
Role / ownership
Derivation
P. Eswar Mukesh
250001053
Structure A — src/structA/
D1
Boinapally Sricharan Rao
250001018
Structure B — src/structB/
D2
M. Varnith Reddy
250001045
Architect — include/, app/
D3
MV. Karthik
250001046
Oracle owner — oracle/, tests/
D4
N. Dinakar Prashanth
250001051
Adversary — adversary/
D5
Koya Reswanth
250001042
Analyst — bench/, results/, scripts/
D6

 
2. Problem
The application models a build system as a directed graph G = (V, E). Each vertex v is a target with a non-negative build duration d(v). A directed edge (u, v) means that u is a prerequisite of v: u must finish before v may start. The adjacency-list convention is therefore adj[u] = the list of targets that depend on u. For example, the build rule “app: lib util” produces edges lib → app and util → app.
For an acyclic graph, the system must return a topological ordering: every prerequisite appears before the target that depends on it. It must also calculate a schedule assuming unlimited parallel workers and no resource contention. The project duration is the minimum possible completion time under those precedence constraints. A critical path is a dependency path whose task durations sum to the project duration.
For a cyclic graph, no valid topological ordering exists. The system must report one actual simple directed cycle, not merely the set of unprocessed vertices. This distinction matters because vertices downstream of a cycle can remain unprocessed even though they are not themselves part of a cycle.
The project also supports incremental dependency insertion. Given a valid current order and a proposed edge u → v, Structure B must either (i) insert the edge and update the order while preserving validity, or (ii) report a cycle and leave both the graph and maintained order unchanged.
·       Static task: find an order, detect a cycle, and calculate schedule fields.
·       Dynamic task: insert dependencies one at a time without recomputing the entire order when a local repair is sufficient.
·       Scale exercised by the test suite: a reported large-graph test with 100,000 vertices and 1,099,934 dependency operations.
·       Graph assumptions: C++17, adjacency lists, non-negative integer durations, duplicate edges allowed, self-loops treated as cycles, and an empty graph treated as a valid empty DAG.
3. Interface and Invariants
3.1 Common Data Representation and Interface
The shared contract is specified in INTERFACE.md and reflected in include/. Graph stores n, adj, and duration. Result stores hasCycle, order, cycle, est, eft, lst, lft, slack, criticalPath, and projectDuration. DynamicOrder stores ord and pos, where ord[i] is the vertex at position i and pos[v] is the inverse mapping. InsertResult records whether an insertion creates a cycle, the cycle itself, the old affected interval, the affected count, and the number of vertices whose positions changed.
3.2 DynamicOrder Representation
The core dynamic-order invariant is pos[ord[i]] = i for every valid position i, with ord a permutation of all vertices. For every graph edge u → v, a valid order must satisfy pos[u] < pos[v]. A successful insertion must preserve both properties. If an insertion creates a cycle, the operation must be transactional: the graph and DynamicOrder remain exactly as they were before the call.
3.3 Structural Invariants
Acyclic Result invariants include a permutation order that respects every edge; earliest and latest schedule vectors of length n; eft[v] = est[v] + duration[v]; slack[v] = lst[v] − est[v] ≥ 0; and a critical path consisting of connected zero-slack vertices whose durations sum to projectDuration. Cyclic results must contain a closed simple cycle following edge directions, while order and schedule vectors are empty and projectDuration is zero.
3.4 Invariant Checkers
The validator layer checks these properties directly. validateOrder checks permutation and edge precedence. validateCycle checks closure, simplicity, endpoint range, and edge existence. validateResult checks schedule equations and critical-path structure. validateDynamic checks the inverse position map and all edge constraints. These checks are independent of the particular valid topological order chosen.
Header / module
Purpose
common.hpp
Shared integer aliases and sentinels.
graph.hpp / result.hpp
Graph and result data structures.
dynamic_order.hpp
DynamicOrder and InsertResult.
interface.hpp
Public entry points for A, B, incremental insertion, and oracle.
schedule.hpp
Critical-path schedule computation.
validate.hpp
Order, cycle, result, and dynamic-state invariants.
io.hpp
Input parsing, serialization, and result printing.
generators.hpp
Deterministic graph and insertion-sequence generators.

 
4. Structure A
4.1 Representation and Initialization
Structure A is implemented in src/structA/struct_a.cpp. It first computes the indegree of every vertex by scanning all adjacency lists. Every vertex whose indegree is zero is eligible to be placed next. When a vertex u is removed from the worklist, it is appended to the output order; each outgoing edge u → v is then processed by decrementing indegree[v]. When indegree[v] becomes zero, v is inserted into the worklist.
4.2 Kahn Operations and Variants
The FIFO variant returns one valid topological order. The lexicographic variant uses the project’s own MinHeap implementation to always choose the smallest available vertex id. This gives the lexicographically smallest topological ordering. The custom heap is implemented in src/structA/min_heap.hpp rather than delegating the studied data structure to std::priority_queue. The FIFO queue is implemented in src/structA/fifo_queue.hpp.
4.3 Cycle Detection and Reconstruction
If fewer than n vertices are removed, the residual graph contains a directed cycle. Structure A marks vertices with positive remaining indegree, constructs reverse adjacency restricted to those residual vertices, follows residual predecessors until a vertex repeats, and reverses the repeated segment to restore the original edge direction. This yields a concrete simple cycle rather than returning every residual vertex.
4.4 Time and Space Complexity
The queue algorithm performs O(V + E) work: indegrees are initialized in O(V + E), each vertex is enqueued/dequeued at most once, and each edge is scanned once during elimination. The heap variant performs O(V log V) heap work plus O(E) edge processing, giving O(V log V + E). The heap variant is useful when deterministic lexicographic minimality is part of the output contract; it is not the default fastest option.
4.5 Correctness Derivation (D1)
D1, authored by P. Eswar Mukesh, proves two claims: every produced order is topological, and if the algorithm stops with vertices remaining, those vertices contain a cycle.
4.6 Indegree Invariant
The key invariant is that indegree[v] equals the number of incoming edges to v whose source has not yet been removed. Initially this is true by direct indegree counting. When u is removed, the algorithm processes each outgoing edge u → v and decrements indegree[v] exactly once, so the invariant remains true.
4.7 Topological-Order Correctness
For validity, suppose v is removed. It could only have entered the queue or heap when its current indegree became zero. By the invariant, every predecessor of v has already been removed. Thus every edge u → v has u earlier than v in the output. This is exactly the topological-order condition.
4.8 Residual-Graph Cycle Argument
For cycle detection, let R be the nonempty set of vertices not removed when the worklist becomes empty. Every v in R has positive remaining indegree; otherwise it would have been eligible for removal. Hence every v in R has an incoming edge from another vertex in R. Starting at any residual vertex and repeatedly following an incoming edge must eventually repeat a vertex because R is finite. The repeated segment gives a directed cycle (when read in the original edge direction).
4.9 Design Decisions
The proof justifies both the order and the residual-cycle test. The implementation performs a separate reconstruction step because the proof that a cycle exists does not itself specify a convenient output representation. The code’s predecessor walk supplies the concrete cycle required by the interface.
·       FIFO: O(V + E) time and O(V) auxiliary storage.
·       Min-heap: O(V log V + E) time and O(V) auxiliary storage.
·       Cycle output: a closed simple cycle; no schedule is produced for cyclic input.
·       Schedule computation is called only after all vertices have been successfully ordered.
·       Proof 1: zero remaining indegree implies all predecessors were already output.
·       Proof 2: every residual vertex has a residual predecessor; finiteness forces repetition.
·       Implementation mapping: Kahn loops and the r.order.size() != n branch in src/structA/struct_a.cpp.
5. Structure B
5.1 Initial Representation and DFS State
Structure B’s initial static ordering is implemented in src/structB/struct_b.cpp. It performs a full DFS forest using an explicit vector as a stack. state[v] distinguishes unvisited (0), active (1), and finished (2). next[v] records the next outgoing adjacency entry to examine, allowing the algorithm to simulate recursive DFS without recursion.
5.2 Iterative DFS Operations
When the top vertex u has another outgoing edge u → v, the algorithm advances next[u]. If v is unvisited, it marks v active and pushes it. If v is active, the edge is a back edge to an ancestor on the current DFS stack, so a directed cycle has been found; the stack segment from v to u followed by v is returned as a closed cycle. If all outgoing edges of u have been examined, u is marked finished and appended to finish.
5.3 Producing the Initial Topological Order
For an acyclic graph, reversing finish gives a topological order. The implementation optionally fills DynamicOrder: ord receives the computed order and pos is constructed as its inverse. The algorithm then calls computeSchedule. Its static complexity is O(V + E), because each vertex is pushed and finished once and each adjacency entry is examined once.
5.4 Complexity and Design Decisions
The explicit stack is an important engineering decision. A long dependency chain can have depth proportional to V. Recursive DFS would consume call-stack space proportional to that depth and may overflow for large inputs. The iterative implementation stores the traversal state in vectors under explicit control.
For incremental insertion, the source currently rebuilds revAdj by scanning every edge before the backward search. Therefore, even though the two searches are restricted by positions to [lo, hi], an invalidating insertion includes an O(V + E) reverse-adjacency construction cost. The implementation as checked in should consequently be described as O(V + E) per invalidating insertion in the worst case, plus the bounded searches and interval rewrite—not as a purely affected-set-only update.
5.5 Dynamic-Order Representation and Update Interval
Let the current DAG have a valid order and inverse positions pos. Consider insertion of edge e = (u, v). If pos[u] < pos[v], the new edge already agrees with the order: append it and do not reorder. If pos[u] > pos[v], the order is invalidated. Define lo = pos[v] and hi = pos[u]. Any repair can be confined to the interval [lo, hi]; vertices outside it already lie strictly before both endpoints or strictly after both endpoints and need not be moved.
5.6 Forward and Backward Affected Sets
Within the interval, define F as the vertices reachable forward from v and B as the vertices that can reach u. The forward search is restricted to vertices whose old positions are at most hi; the backward search is restricted to positions at least lo. In the current implementation the backward search uses a reverse-adjacency structure. The key cycle condition is v ⇝ u: if that path exists, adding u → v closes a directed cycle. If no such path exists, the two reachable sets are disjoint and the order can be repaired by placing B before F while preserving each set’s internal order.
5.7 Reordering and Position-Map Maintenance
The code uses DynamicOrder.ord and DynamicOrder.pos to enforce the position interval and maintain the inverse mapping. It constructs reverse adjacency, performs the forward and backward searches, checks whether u is reachable from v, and—if no cycle is formed—rebuilds the interval as B vertices, then neutral interval vertices, then F vertices. It updates pos for every vertex placed in the interval and finally appends u → v to the graph.
5.8 Cycle Handling and Transactional Invariant
If a cycle is found, the code reconstructs the path v ⇝ u using the forward-search parent array, appends v to close the cycle, and returns without modifying g or d. This is consistent with the transactional cycle invariant. A self-loop is handled directly as [u, u].
·       state = 0: undiscovered; state = 1: active; state = 2: finished.
·       next[u] prevents rescanning already examined outgoing edges.
·       Reverse finishing order is topological when no back edge exists.
·       Cycle extraction uses the active stack segment and closes it by repeating the ancestor.
·       Fast path: pos[u] < pos[v] ⇒ append edge; no search or reorder.
·       Invalidating edge: search only within the old interval [pos[v], pos[u]].
·       Cycle condition: forward reachability from v reaches u.
·       Successful repair: place B before neutral vertices before F, preserving the internal order of each group.
·       Update both ord and pos; append the edge only after the cycle check succeeds.
6. The Requirement Beyond the Lecture
6.1 MNR and Pearce–Kelly: Algorithmic Difference
The choice is practical and implementation-oriented, not a claim that Pearce–Kelly dominates MNR in every theoretical measure. MNR (Marchetti-Spaccamela, Nanni, and Rohnert, 1996) is a valid incremental topological-order algorithm with an O(V) amortized time bound per edge insertion over a sequence of insertions starting from an empty graph, and O(V) additional space beyond the graph representation. The MNR method identifies the forward-reachable set from v and shifts it toward the end of the affected interval; in the Pearce–Kelly paper’s description, every vertex in the affected interval receives a new position during this shift.
6.2 Affected-Set Reordering in Pearce–Kelly
Pearce–Kelly (PK) discovers both sides of the invalidated edge: a forward search from v identifies F, and a backward search from u identifies B. It then reassigns only the positions occupied by F ∪ B, putting B before F and leaving all other vertices unaffected. Its per-insertion work is expressed in terms of the affected sets and their incident search edges, plus sorting/reassignment. The original paper gives Θ(δ log δ + |δ|) for the presented implementation, where δ is the union of the forward- and backward-reachable affected vertices.
6.3 Why Pearce–Kelly Was Chosen
Pearce and Kelly explicitly state that their algorithm has inferior time complexity to the best previously known result, but that its simplicity yields better practical performance. Their experiments report that PK is the best of the compared methods on sparse digraphs and only a constant factor slower than the best on dense digraphs. They also explain that MNR’s one-way search can cause work to be proportional to the entire affected interval even when the truly relevant forward set is smaller; MNR’s amortized result does not express this per-update locality in the same way.
For this project, PK is a natural match because the Structure B owner’s derivation D2 is built around the two-sided sets F and B, the bounded interval [pos[v], pos[u]], and a simple B-before-F reassignment. The existing DynamicOrder representation already stores both the order and inverse positions, making interval-restricted traversal and local order updates direct to implement. This gives a clear correctness proof, explicit cycle detection, and an affected-region quantity that can be instrumented.
The trade-off must be stated accurately. MNR has a stronger amortized guarantee in its stated setting; PK’s practical advantage is supported by the PK paper’s experiments, especially for sparse graphs, and by the simpler proof-to-code mapping used here. The repository does not contain a direct MNR implementation or a controlled PK-versus-MNR benchmark. Therefore this report justifies the design choice using the literature and implementation fit, not an experimental claim that this team measured PK faster than MNR.
6.4 Critical-Path Schedule Representation
The schedule computation is implemented in app/schedule.cpp and is shared by Structures A and B. It assumes a valid topological order and non-negative task durations. The model assumes unlimited workers: independent tasks may run simultaneously, and only dependency edges constrain start times.
6.5 Earliest-Time Recurrence
For each vertex v, earliest start is ES(v) = 0 when v has no predecessor; otherwise ES(v) = max over predecessors u of [ES(u) + d(u)]. Earliest finish is EF(v) = ES(v) + d(v). Processing vertices in topological order makes every predecessor’s earliest finish known before v is processed. The forward pass therefore relaxes each outgoing edge once, keeping the maximum candidate.
6.6 Latest-Time Recurrence and Slack
The project duration is T = max_v EF(v). In reverse topological order, latest finish is LF(v) = T for a vertex with no successor; otherwise LF(v) = min over successors w of LS(w). Latest start is LS(v) = LF(v) − d(v). Slack is S(v) = LS(v) − ES(v). The backward pass scans each outgoing edge once and takes the tightest successor deadline.
6.7 Derivation D3: Correctness of the Schedule
D3 proves the forward recurrence by induction on topological order. Every path ending at v is either the single-vertex path or a path ending at a predecessor u followed by u → v. Therefore the maximum path duration before v is the maximum of zero and ES(u) + d(u) over all predecessors. Since all predecessors are processed first, the algorithm has already propagated every candidate when v is processed. The resulting ES(v) is the earliest feasible start.
D3 also proves LS(v) = T − B(v), where B(v) is the maximum duration of a path starting at v and including d(v). Thus S(v) = T − A(v) − B(v), with A(v) the maximum prefix duration before v. A vertex has zero slack exactly when it lies on at least one critical path. However, two adjacent zero-slack vertices form a critical-path edge only when the edge is tight: ES(w) = EF(v). The implementation uses this tight-edge condition when extracting a deterministic critical path.
6.8 Scheduling Complexity and Boundary Cases
Each forward/backward pass is O(V + E). Critical-path extraction scans candidate outgoing edges along the selected path. The empty graph is handled explicitly: projectDuration is zero and schedule arrays and criticalPath are empty.
6.9 Beyond-Lecture Algorithms in This Project
The beyond-lecture material in this project comprises Kahn’s algorithm for the Structure A ordering implementation and Pearce–Kelly’s dynamic topological ordering method for Structure B. Structure B’s initial order is produced by iterative DFS; Pearce–Kelly is used for subsequent dependency insertions. The distinction is important: DFS builds an initial order, while Pearce–Kelly maintains an existing order as edges are added.
Aspect
MNR (1996)
Pearce–Kelly (2006)
Search
Forward search from v
Forward from v and backward from u
Repair
Shift forward-reachable vertices toward the end of the affected interval
Pool affected positions and place B before F
Theoretical statement
O(V) amortized per insertion from an initially empty graph
Per-update bound in terms of affected set δ; Θ(δ log δ + |δ|) in the paper’s implementation
Locality
Reassignment scans the affected interval
Reassignment uses positions occupied by F ∪ B
Paper’s empirical conclusion
PK significantly more efficient on sparse graphs; only constant-factor slower on dense graphs
Better practical performance despite inferior asymptotic bound to the best prior result
Project fit
Would require a different update strategy and proof mapping
Direct match to D2 and the current two-sided affected-set design

 
7. Correctness
7.1 DFS Cycle Detection and Reverse-Finish Proof (D4)
D4, authored by MV. Karthik, establishes that a full DFS detects a back edge if and only if the directed graph contains a cycle, and that reverse finish order is topological for a DAG.
If DFS encounters an edge u → v where v is active, v is an ancestor of u in the current DFS tree. The tree edges form a directed path v ⇝ u. Adding u → v closes a directed cycle. Thus every reported back edge certifies a cycle.
Conversely, suppose a directed cycle exists and consider the first vertex x on that cycle discovered by a full DFS. The DFS explores outgoing edges before finishing x. Following the cycle from x eventually reaches an already active vertex on the current DFS path; otherwise the cycle vertices would have to finish in a strictly cyclic order, which is impossible. Therefore a full DFS of every component encounters a back edge when a cycle exists.
For an edge u → v in a DAG, v cannot be active when the edge is examined, because that would be a back edge and hence a cycle. If v is unvisited, DFS explores and finishes v before u can finish. If v is already finished, it has already received a smaller finish time. Therefore f(u) > f(v) for every edge u → v. Sorting vertices by decreasing finish time places u before v for every edge, which is a topological order.
The derivation’s assigned statement concerns the DFS theorem used by Structure B. The project’s oracle uses a different, indegree-based method; the two algorithms are not conflated. The oracle’s correctness is established independently and is used as a reference for tests.
7.2 Pearce–Kelly Reordering Derivation (D2)
D2 is authored by Boinapally Sricharan Rao (Roll No. 250001018), Structure B owner. It proves the local-reordering statement used by insertDependencyB(): when u lies after v, only vertices in the interval between them need consideration; vertices reaching u are placed before vertices reachable from v; and if v reaches u, the inserted edge closes a cycle.
Notation: lb = ord(v), ub = ord(u), R = {w : lb ≤ ord(w) ≤ ub}, F = {w : v ⇝ w and ord(w) ≤ ub}, and B = {w : w ⇝ u and ord(w) ≥ lb}. Reachability includes a zero-length path, so v ∈ F and u ∈ B. Along every existing directed path, positions strictly increase because the old order is topological.
Closure lemma. If w ∈ F, then v ⇝ w implies ord(w) ≥ lb, and the definition gives ord(w) ≤ ub; hence w ∈ R. If w ∈ B, then w ⇝ u implies ord(w) ≤ ub, and the definition gives ord(w) ≥ lb; hence B ⊆ R. Also, F is closed under outgoing edges that remain within the upper bound, and B is closed under incoming edges that remain within the lower bound.
Cycle lemma. The new graph G + (u, v) is cyclic exactly when v ⇝ u in the old graph. One direction follows by adding the new edge to the path v ⇝ u. Conversely, any new cycle must use the newly inserted edge because the old graph was acyclic; removing that edge leaves a path v ⇝ u. Equivalently, v ⇝ u exactly when F ∩ B is nonempty: if v reaches u, u belongs to both; if w belongs to both, concatenate v ⇝ w and w ⇝ u.
Assume no cycle is formed, so F ∩ B = ∅. Let P be the sorted set of old positions occupied by F ∪ B. Sort B and F separately by their old positions, concatenate B followed by F, and assign this list the positions in P in increasing order. Every vertex outside F ∪ B keeps its old position. This assignment is injective because it permutes exactly the positions originally occupied by the distinct vertices in F ∪ B.
Monotone-drift lemma. Every x ∈ B moves weakly left: ord′(x) ≤ ord(x). Every y ∈ F moves weakly right: ord′(y) ≥ ord(y). This follows from order statistics: the k-th smallest pooled position cannot exceed the old position of the k-th B vertex, while the corresponding upper order statistic cannot be smaller than the old position of the relevant F vertex.
Validity follows by considering edge categories. The new edge u → v is respected because u ∈ B and v ∈ F, and all B vertices are assigned before all F vertices. Edges internal to B or F remain valid because each set preserves its old relative order. An edge from F to B is impossible: it would create v ⇝ x → y ⇝ u, contradicting the no-cycle assumption. For edges crossing moved and unmoved vertices, closure of F and B plus the monotone-drift inequalities ensure the original strict order remains strict. Edges with both endpoints unmoved are unchanged.
Therefore the reassignment produces an injective ordering that respects every edge of G + (u, v). This is the core correctness argument behind Pearce–Kelly’s affected-set reordering. The submitted implementation uses the same two reachability sets and B-before-F principle, but its concrete interval rebuild retains neutral vertices in the middle; this implementation detail is documented in Section 13 rather than silently equated with the exact pooled-position formulation in D2.
7.3 Independent Oracle
The oracle is implemented in oracle/oracle.cpp and is intentionally independent of Structures A and B. It uses a simple linear scan to choose the smallest remaining zero-indegree vertex, making its topological order lexicographically smallest. The repeated linear scan makes the ordering phase O(V² + E), which is acceptable for a reference implementation on small test graphs but unsuitable as a production algorithm for very large graphs.
The oracle does not call runStructureA, runStructureB, insertDependencyB, or computeSchedule. Its schedule is computed independently by repeated forward and backward relaxation until no value changes. This independence reduces the chance that the same implementation bug appears in both the algorithm under test and its reference.
7.4 Differential and Incremental Testing
The differential harness compares structural properties rather than requiring identical orders, cycles, or critical paths. Multiple topological orders and multiple valid cycles/critical paths may exist. It compares hasCycle, projectDuration, est, lst, and slack exactly where these are unique, and validates the returned order, cycle, and critical path against their invariants. The lexicographically smallest Structure A mode is compared exactly against the oracle’s order.
Incremental tests preserve snapshots before each insertion. If an insertion creates a cycle, the test checks that the reported cycle is valid and that graph and DynamicOrder are unchanged. Otherwise it validates the updated dynamic order, verifies that vertices outside the old interval did not move, and compares a fresh Structure B result with the oracle.
7.5 Executed Test Evidence
Verification performed while preparing this report: `make test` compiled and ran test_adversarial, test_edge_cases, test_incremental, test_large_scale, test_oracle, test_structures, test_validators, and differential_test. All passed. The large-scale test printed `large_graph_vertices=100000` and `large_graph_dependency_operations=1099934`. The differential test reported 1,000 static random-graph tests and incremental insertion tests passed. These are observed test outputs from the uploaded repository at report-generation time.
The critical-path recurrences and their correctness argument are presented in Section 6. The present section focuses on ordering, cycle detection, incremental-update correctness, independent validation, and test evidence.
·       Static differential batch: 1,000 seeded random directed graphs.
·       Incremental insertion checks: verify dynamic-order invariants after each successful insertion.
·       Adversarial checks: layered DAGs, chain-family DAGs, antichains, hidden cycles, and worst-insertion sequences.
·       Large-scale test: 100,000 vertices and 1,099,934 dependency operations, as printed by the test executable.
·       Observed result: all test executables passed under the repository Makefile.
8. Adversarial Inputs
8.1 Worst-Case Insertion Target
D5, authored by N. Dinakar Prashanth, contains two constructions. The first aims to force Θ(V) vertices to be reordered at every insertion. The second constructs a DAG with exponentially many valid topological orders.
The intended worst-insertion construction partitions n vertices into a chain C of length K = floor(n/2) and t = n − K isolated vertices W. Initially C is c0 → c1 → … → c(K−1). The intended initial order is [c0, …, c(K−1), w0, …, w(t−1)]. Insert edges wj → c0 one at a time. Each insertion is acyclic because wj has no path back from the chain, but invalidates the current order. The affected forward set contains the K chain vertices and the backward set contains wj. Reordering therefore involves K + 1 vertices = Θ(V), and t such insertions yield Θ(V²) total moved/affected work.
8.2 Exponentially Many Topological Orders
The second construction divides vertices into consecutive layers of constant width w ≥ 2 and adds all edges from each layer to the next. The layer index strictly increases along every edge, so the graph is acyclic. Every layer may be internally permuted, while all vertices of an earlier layer must precede all vertices of the next layer. The number of valid orders is the product of the factorials of the layer sizes. For even n and width two, this is 2^(n/2), which is exponential in n.
8.3 Generator Implementation
The generators are in adversary/generators.cpp. genExponentialDAG supports `layers`, `chains`, and `antichain` modes. genHiddenCycleBehindChain constructs a head chain leading to a directed cycle and then a tail chain. genWorstInsertions constructs a chain and returns edges from the remaining vertices to the chain head.
8.4 Benchmark Construction Mismatch
A reproducibility issue was found in the current worst-insertion benchmark. The derivation assumes the initial order places the chain before the isolated vertices. However, runStructureB starts DFS roots in increasing vertex id and then reverses the complete finish list. For the disconnected chain-plus-isolates graph, this places the later isolated roots before the chain in the final order. Consequently the generated insertion (w_j, c_0) is already forward in the computed order and takes the fast path. The current incremental_comparison.csv consequently records AffectedRegionSize = 0 for all 950 insertions. The current figure therefore measures mostly no-reorder insertions, not the Θ(V) adversarial reorder claimed by D5.
8.5 Interpretation and Required Correction
This is a mismatch between the intended D5 initial ordering and the actual initial ordering produced by the implementation. The theoretical construction remains valid when initialized with the stated order, but the generator/benchmark must be changed or the initial order explicitly constructed before the experiment can validate it. The report preserves this distinction and does not claim the current benchmark verifies the worst-case theorem.
·       Required correction for a valid worst-case benchmark: construct or enforce the chain-first initial order before generating the (wj, c0) insertions, then assert pos[wj] > pos[c0] before every insertion.
·       Record both `affected` and `moved`; the current source sets affected to interval width on successful reorder and moved to the number of changed positions.
·       Retain the existing oracle and validateDynamic checks after each insertion.
9. Predicted Cost Model
9.1 Variables and Assumptions
D6, authored by Koya Reswanth, develops cost models for static sorting, schedule computation, incremental maintenance, and the crossover between incremental updates and full recomputation. Let V = |V|, E = |E|, and d = E/V. For insertion i, let E_i = E_0 + i and d_i = E_i/V. When an insertion invalidates the order, let AR_i be the number of vertices in the affected search region, and let p be the fraction of insertions requiring reordering.
9.2 Static Algorithm Models
The model for Structure A with FIFO queue is T_A = a(V + E). For its min-heap variant, T_A,heap = a(V + E) + a_h V log₂V. The model for iterative DFS Structure B is T_B = b(V + E). The critical-path pass is T_crit = q(V + E). These expressions reflect one or a constant number of passes over vertices and adjacency entries, plus heap operations where applicable.
9.3 Incremental Insertion Model
For an insertion that already respects the order, the model assigns a small constant cost h₀. For an invalidating insertion, the model is T_inc(i) = h₀ + g AR_i(1 + d_i) + s AR_i log₂(AR_i). The g term approximates discovery work over affected vertices and their incident edges; the s term represents sorting/reassignment. Averaging over an insertion stream gives T_inc(avg) = h₀ + p[g ARbar(1 + d) + s ARbar log₂(ARbar)].
9.4 Single-Update Crossover
Ignoring lower-order constants and the sort term for a first-order comparison, full recomputation costs bV(1 + d_i), while one incremental reorder costs g AR_i(1 + d_i). Recompute is cheaper for an individual update only when AR_i ≥ (b/g)V. Since AR_i ≤ V, this can occur only when g ≥ b and the affected region is sufficiently large.
9.5 Batch Crossover
If the order is required only after a batch of m insertions, one recomputation costs bV(1 + d), while incremental maintenance over the batch costs approximately mp g ARbar(1 + d). Equating them gives m* = bV/(p g ARbar). Small affected regions increase the crossover batch size; adversarial updates with ARbar approaching V can make recomputation competitive much sooner.
9.6 Calibration and Model Limitations
The model’s constants are machine-dependent and are to be fitted on calibration runs and frozen before large-run measurements. The original REPORT.md records a commit history for the model preceding the raw CSV files. However, several predicted quantities and setup fields remain marked TODO in that file, and the uploaded benchmark data do not provide a valid worst-case incremental measurement. The model should therefore be treated as a pre-measurement analytical model, not as a fully calibrated and validated predictor.
10. Experimental Setup
10.1 Hardware and Software Environment
The repository report records an Intel Core i7-1355U laptop, 16 GB RAM, Windows 11 Home with WSL2 Ubuntu, g++ 15.2.0, and Valgrind/Cachegrind 3.26.0. The benchmark source uses C++ steady/high-resolution clock timing and five repetitions for each static structure point. The static generator uses seed 20260928, graph sizes V = 1,000, 3,000, and 5,000, E = 2V, and skew values 0, 1, and 2.
10.2 Static Benchmark Workload
The static benchmark source uses C++ high-resolution timing and five repetitions for each point. The graph generator uses seed 20260928, vertex counts V = 1,000, 3,000, and 5,000, E = 2V, and skew values 0, 1, and 2.
10.3 Incremental and Profiling Workload
The incremental benchmark uses n = 1,000, seed 20260928, and 950 insertions. The profiling artifacts are Cachegrind instruction-reference summaries. The exact command lines, compiler flags, and complete repetition protocol should be retained with the submitted benchmark scripts.
11. Results
11.1 Static Structure Comparison
Static structure results. Averaging the recorded mean times across skew values gives the following measurements. Structure B’s DFS is fastest in these recorded runs, the FIFO version of Structure A is next, and the heap version is slowest. This is consistent with the expected extra heap work, although the sample is small and standard deviations are material relative to some differences.
11.2 Incremental Benchmark Result
The incremental benchmark data contain 950 insertions for n = 1,000 and seed 20260928. Every row has AffectedRegionSize = 0. This is explained by the initial-order mismatch described in Section 8: the generated insertion is already forward in the computed order and takes the fast path. The current figure must therefore not be interpreted as a measurement of expensive Pearce–Kelly reorders.
11.3 Cachegrind Result and Interpretation
The Cachegrind summary reports 1,996,764,465 total instruction references across both benchmark invocations, with 1,697,362,578 (85.0%) attributed to genRandomDAG. Graph generation dominates the combined profiler total; this aggregate count cannot be attributed to the sorting algorithms alone. The profiler files record instruction references (Ir), not direct wall-clock timing or hardware cache-miss measurements.
V
A FIFO (ms)
A heap (ms)
B DFS (ms)
1,000
0.0939
0.1168
0.0441
3,000
0.2544
0.3314
0.1707
5,000
0.3910
0.5155
0.2610

 
V
A FIFO (ms)
A heap (ms)
B DFS (ms)
1,000
0.0939
0.1168
0.0441
3,000
0.2544
0.3314
0.1707
5,000
0.3910
0.5155
0.2610

 

Figure 1. Static structure comparison from the repository figure (V = 1,000; logarithmic y-axis).
12. Limitations
12.1 Benchmark Coverage and Methodology
The most important limitations are methodological rather than theoretical. First, the static benchmark sizes stop at 5,000 vertices, so the timing results do not establish behavior at V = 100,000 and E = 1,000,000. Second, the current worst-insertion benchmark does not create invalidating insertions because the initial DFS order differs from the order assumed by D5. Third, the recomputation benchmark does not replay all earlier insertions between samples, so the recomputed graph is not the same graph state as the incrementally maintained graph at each sampled point.
12.2 Implementation-Level Complexity
There is also a code-level complexity issue in insertDependencyB(): it rebuilds the entire reverse adjacency list by scanning all graph edges before the backward search. This costs O(V + E) per invalidating insertion, even though the conceptual Pearce–Kelly search is intended to restrict exploration to the affected region. As written, the implementation’s total update cost is therefore not purely proportional to the affected region. To realize the locality assumed in D2/D6, maintain reverse adjacency incrementally or use a representation that supports incoming-edge traversal without rebuilding it for every update.
12.3 Difference Between Derivation and Implementation
The current interval-rebuild code constructs B, then neutral interval vertices, then F, and writes all interval positions back. D2’s mathematical pooled-position formulation instead assigns only the positions originally occupied by F ∪ B, leaving all other vertices untouched. The code’s approach is a related interval-local stable partition, but the report should not claim it is literally the same assignment. The current tests verify order validity and no movement outside the old interval; they do not assert the stronger D2 property that every vertex outside F ∪ B keeps its exact position.
12.4 Other Limitations
Other limitations include the incomplete experimental fields marked TODO in REPORT.md, a small static benchmark sample, one fixed random seed, and no direct benchmark comparison against MNR. The cost model assumes simple proportionality to touched vertices and edges and omits cache, allocation, and reverse-adjacency reconstruction effects. The oracle is deliberately quadratic in V and is therefore used only on small graphs; large tests instead compare structures and validators.
12.5 Recommended Next Measurements
Recommended next steps are: (1) correct the initial ordering for genWorstInsertions and assert every generated insertion violates the current order; (2) fix the benchmark to apply every insertion to both incremental and recomputation graph states, sampling recomputation only after the full prefix has been applied; (3) avoid rebuilding reverse adjacency for each insertion; (4) add counters for forward-search vertices, backward-search vertices, interval width, affected-set size, moved vertices, and edges scanned; (5) rerun multiple seeds and graph sizes including 10^5 vertices and 10^6 edges; and (6) add a true MNR implementation only if an empirical PK-versus-MNR comparison is required.
12.6 Overall Conclusion
The TeamMatrix project provides a modular implementation of build ordering, cycle reporting, critical-path scheduling, and incremental dependency insertion. Structure A demonstrates Kahn’s algorithm with queue and heap worklists; Structure B demonstrates iterative DFS and a Pearce–Kelly-style dynamic reorder. D1–D6 provide the associated correctness, scheduling, adversarial, and cost-model derivations.
The strongest verified result in the uploaded repository is correctness under the supplied tests: the full Makefile test target passed, including the large-scale test and differential testing. The recorded small static benchmarks favor Structure B’s DFS over the two Structure A variants, with the heap variant paying additional selection cost. The incremental timing evidence is not yet valid for the intended worst-case experiment, and the current implementation’s reverse-adjacency rebuild must be accounted for in any complexity claim.
Pearce–Kelly is a defensible design choice for this implementation because its two-sided affected-set formulation is simple to reason about, maps cleanly to DynamicOrder, and has favorable published practical performance on sparse digraphs. MNR remains a theoretically strong alternative with an O(V) amortized insertion guarantee in its original setting. A direct local benchmark and a corrected incremental implementation would be required to make a project-specific performance comparison.
Overall, the repository demonstrates a modular implementation of build ordering, cycle reporting, critical-path scheduling, and incremental dependency insertion. The supplied test suite passed, including large-scale and differential tests. The static measurements favor Structure B’s DFS in the tested range. The incremental timing evidence and reverse-adjacency implementation need correction before claiming that measured performance validates the affected-region cost model.
·       Correct the adversarial generator/order mismatch.
·       Correct the recomputation benchmark state before comparing timings.
·       Make reverse adjacency persistent or otherwise avoid O(V + E) reconstruction per insertion.
·       Run repeated, multi-seed benchmarks at the intended scale and report confidence/spread.
·       Complete all TODO fields in the experimental protocol and cost-model calibration.
13. References
[1] A. B. Kahn, “Topological sorting of large networks,” Communications of the ACM, vol. 5, no. 11, pp. 558–562, 1962.
[2] D. J. Pearce and P. H. J. Kelly, “A dynamic topological sort algorithm for directed acyclic graphs,” ACM Journal of Experimental Algorithms, vol. 11, Article 1.7, 2006. DOI: 10.1145/1187436.1210590.
[3] A. Marchetti-Spaccamela, U. Nanni, and H. Rohnert, “Maintaining a topological order under edge insertions,” Information Processing Letters, vol. 59, no. 1, pp. 53–58, 1996. DOI: 10.1016/0020-0190(96)00075-0.
[4] T. H. Cormen, C. E. Leiserson, R. L. Rivest, and C. Stein, Introduction to Algorithms, 3rd ed., MIT Press, 2009.

