# Project 01: Build Ordering and Critical Paths
Team TeamMatrix — CS 253 / MA 253, Autumn 2026

## Team and roles
| Name                     | Roll no.      | GitHub               | Role              | Module folder                | Derivation |
|--------------------------|---------------|----------------------|-------------------|------------------------------|------------|
| P.Eswar Mukesh           | 250001053     | @Eswarmukesh14       | Structure A owner | `src/structA/`               | D1         |
| Boinapally Sricharan Rao | 250001018     | @sricharanrao01-web  | Structure B owner | `src/structB/`               | D2         |
| M.Varnith Reddy          | 250001045     | @7127Vzs             | Architect         | `include/, app/`             | D3         |
| MV.Karthik               | 250001046     | @karthikmvs008       | Oracle owner      | `oracle/, tests/`            | D4         |
| N.Dinakar Prashanth      | 250001051     | @dinakarprashanth19  | Adversary         | `adversary/`                 | D5         |
| Koya Reswanth            | 250001042     | @reswanthkoya205     | Analyst           | `bench/, results/, scripts/` | D6         |

## Summary

This project is a build-ordering tool that models tasks as vertices and dependencies as directed edges. Structure A uses Kahn's algorithm with a FIFO queue or a min-heap; Structure B uses iterative DFS and Pearce–Kelly-style incremental topological ordering. Both detect cycles and compute earliest starts, latest starts, slack and a critical path. In the supplied static benchmark CSV, DFS has lower mean running time than both Kahn variants at 5,000 vertices for each tested skew; the incremental benchmark has limitations described below.

## Requirements

- Linux (recommended), Windows with WSL2, or MSYS2 with GNU tools and Bash.
- Git, GNU Make, and a C++17-compatible g++ compiler. Verified with g++ 14.2.0 on Linux; REPORT.md records g++ 15.2.0 for the team's measurement environment.
- Python 3, with pandas, matplotlib and numpy for plotting only. The scripts invoke `python`, so that command must be available.
- Default compiler flags: `-std=c++17 -O3 -Wall -Wextra`.

On Ubuntu/WSL, install dependencies once:

```bash
sudo apt update
sudo apt install git build-essential python3 python3-venv
```

Clone the repository (requires repository access) and enter it:

```bash
git clone https://github.com/reswanthkoya205/cs253-TeamMatrix-p01.git
cd cs253-TeamMatrix-p01
```

For final evaluation, check out the `final` tag once the team has published it. Set up plotting dependencies:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install pandas matplotlib numpy
```

Run all remaining commands from the repository root in Bash. On native MSYS2 Windows, executables have the `.exe` suffix; the examples below use Linux/WSL.

## Build

```bash
make
```

This builds the application, all test executables and both benchmark executables in `bin/`. Intermediate files are stored in `build/`. If switching operating systems or reusing a directory containing old build outputs, run `make clean` first.

## Quick start

### 1. Understand the input

The included file `examples/small.txt` contains:

```text
parse   3 :
assets  4 :
compile 5 : parse
link    2 : compile assets
```

Each line is `target [duration] : dependencies`. For example, `compile 5 : parse` means compile takes 5 time units and must wait for parse to finish. The edge points from parse to compile. The default duration is 1 when omitted; explicit durations are integers from 0 to 1,000,000,000. Declare each target once, including dependencies; comments begin with `#`.

### 2. Run Structure A (FIFO)

```bash
./bin/app --input examples/small.txt --structure A
```

Verified output:

```text
ORDER: parse assets compile link
START: parse=0 assets=0 compile=3 link=8
LATEST: parse=0 assets=4 compile=3 link=8
SLACK: parse=0 assets=4 compile=0 link=0
MAKESPAN: 10
CRITICAL: parse -> compile -> link
```

`ORDER` is a valid execution order, not a requirement to execute every task serially. `START` is the earliest start, `LATEST` is the latest start without delaying completion, and `SLACK` is their difference. `MAKESPAN` is the minimum total completion time with unlimited workers. `CRITICAL` lists one longest-duration dependency path. In this example, 3 + 5 + 2 = 10; assets can start four units later without delaying completion.

### 3. Run the other structures

```bash
./bin/app --input examples/small.txt --structure A --heap
./bin/app --input examples/small.txt --structure B
```

The heap selects the smallest available vertex ID (input declaration order), not necessarily the alphabetically smallest target name. DFS can return a different valid topological order; the makespan and per-target schedule must agree.

### 4. Interactive mode

```bash
./bin/app --input examples/small.txt --structure B --interactive
```

Enter the following commands one at a time:

```text
order
schedule
critical
stats
add assets parse
show
add parse link
quit
```

| Command | Meaning |
|---|---|
| `order` | Display the current topological order. |
| `schedule` | Display earliest/latest starts and slack by name. |
| `critical` | Display makespan and critical path. |
| `stats` | Display operation counters and, for B after a successful insertion, affected region details. |
| `add TARGET DEPENDENCY` | Insert DEPENDENCY → TARGET. |
| `show` | Display the complete result, including numeric vertex IDs for schedule values. |
| `quit` or `exit` | End the session. |

Adding `assets parse` is valid; adding `parse link` creates a cycle and is rejected without changing the graph. Duplicate dependencies produce `DUPLICATE`; unknown targets produce `INVALID`. Structure A recomputes the order after insertion; B maintains the order incrementally, then recomputes the schedule. Use `--structure A` (optionally `--heap`) to try interactive recomputation instead.

## Tests

```bash
make test
```

This runs the unit tests, adversarial tests, incremental tests, oracle and invariant checks, large-scale stress test and differential test. The static differential suite uses base seed `20261109` for 1,000 random graphs; its incremental test uses `20261109 + 99999`. The large-scale suite uses base seed `20261110` for 1,000 independent 64-vertex DAGs with 1,000 edges each, comparing both structures against the oracle. It separately compares A and B on a deterministic 100,000-vertex DAG containing 1,099,934 edges.

A successful run includes:

```text
oracle_differential_seed=20261110 oracle_differential_operations=1000000
large_graph_vertices=100000 large_graph_dependency_operations=1099934
ALL DIFFERENTIAL TESTS PASSED
All tests passed.
```

The million-edge workload is batched static oracle testing, not one million sequential incremental insertions. The quadratic oracle is not run on the full-size graph.

## Evaluation interface (hidden tests)

The implemented CLI accepts a Makefile-like input file or standard input:

```bash
./bin/app --input - --structure A < examples/small.txt > out.txt
./bin/app --input - --structure B < examples/small.txt > out.txt
```

The input and normal output are shown in Quick start. A cyclic input prints `CYCLE:` followed by a closed cycle and exits with status 2; malformed input or options exit with status 1; successful execution exits with status 0.

**Pending confirmation:** INTERFACE.md mentions a separate `bin/run_ops` evaluator, but the supplied Makefile does not build it and the instructor's final operation format is not included in the supplied materials. The commands above describe the actual implemented CLI, not a verified replacement for the official hidden-test interface. Confirm the required format before submission.

## Benchmarks and figures

### Run everything with one command

```bash
bash scripts/run_all.sh
```

This cleans and builds the project, runs the automated tests, demonstrates all three application modes (including interactive commands), checks Python dependencies, executes both benchmarks, regenerates both plots and checks that the output files were updated. An independent plotting step verifies regeneration again. A successful run ends with:

```text
ALL EXECUTION CHECKS PASSED
ALL BENCHMARK CSV FILES UPDATED
ALL FIGURES UPDATED SUCCESSFULLY
```

The complete script took about 22 seconds in the Linux verification environment; runtime varies by machine. It overwrites benchmark CSVs and figures, so preserve any previous measurements you need.

### Run individual stages

```bash
make bench       # Run both benchmarks and generate plots
make plots       # Generate plots from existing CSVs only
```

| Experiment | Configuration | Raw output | Figure |
|---|---|---|---|
| Static structure comparison | 1,000 / 3,000 / 5,000 vertices; E = 2V; skew 0, 1, 2; five repeats | `results/raw/struct_comparison.csv` | `results/figures/fig1_structure_comparison.png` |
| Incremental vs recomputation | 1,000 vertices; worst-insertion generator; sampled recomputation | `results/raw/incremental_comparison.csv` | `results/figures/fig2_incremental_vs_recomp.png` |

Both benchmarks use seed `20260928`. The plotting script is `scripts/generate_figures.py`.

## Repository map

| Path | Purpose |
|---|---|
| `include/` | Graph representation, shared interfaces, results and counters. |
| `src/structA/` | Kahn FIFO and heap ordering. |
| `src/structB/` | Iterative DFS and incremental order maintenance. |
| `app/` | CLI, input/output and scheduling. |
| `oracle/` | Reference implementation, differential test and validators. |
| `adversary/` | Random and worst-case graph generators. |
| `bench/` | Timing and operation-count experiments. |
| `tests/` | Unit, invariant, adversarial and scale tests. |
| `examples/` | Small sample dependency file. |
| `scripts/` | Full execution workflow and plotting. |
| `results/` | Raw CSV logs and generated figures. |
| `docs/` | Derivations and presentation materials. |
| `build/`, `bin/` | Generated objects and executables; not submission source files. |

See [INTERFACE.md](INTERFACE.md) for the common API and invariants, [REPORT.md](REPORT.md) for the cost model and analysis, and [TOOLS.md](TOOLS.md) for tool declarations.

## Results at a glance

The following timings are taken from the original uploaded `results/raw/struct_comparison.csv`, before rerunning the benchmark, for V = 5,000, E = 10,000 and skew = 0:

| Method | Mean time (ms) | Standard deviation (ms) |
|---|---:|---:|
| Kahn FIFO | 1.0270 | 0.452507 |
| Kahn heap | 1.7092 | 1.100220 |
| Iterative DFS | 0.5708 | 0.252770 |

DFS was fastest for this measured case; these values are not universal performance guarantees. All automated tests passed in the verification run. See [REPORT.md](REPORT.md) for experimental context and the cost model. Refresh this table if replacing the checked-in measurements.

## Known limitations

- The official hidden-test command and input format still need confirmation as described above.
- The scale test reaches 100,000 vertices and over one million edges, but the current timing benchmarks use smaller graphs. Full-scale timing and a full-scale CLI demo are not established by the small-example script.
- The incremental benchmark adds edges to the recomputation graph only at sampled steps, writes zero for unmeasured times, and does not perform five independent repeats per point. Consequently the two strategies are not consistently timed on identical graph states; do not use that CSV to claim a reliable crossover point or speedup until corrected.
- Incremental topological ordering does not imply incremental schedule calculation: the CLI recalculates the schedule after successful insertions.
- REPORT.md contains unresolved experimental-setup TODOs. Complete them using the team's actual measurement records.

## Tools and references

See [TOOLS.md](TOOLS.md) for AI and other tool use. Principal references:

- A. B. Kahn (1962), Topological sorting of large networks.
- R. E. Tarjan (1976), Edge-disjoint spanning trees and depth-first search.
- D. J. Pearce and P. H. J. Kelly (2006), A dynamic topological sort algorithm for directed acyclic graphs. DOI: 10.1145/1187436.1210590.
- J. E. Kelley Jr. and M. R. Walker (1959), Critical-path planning and scheduling.
- T. H. Cormen et al., Introduction to Algorithms, 3rd ed., sections 22.3–22.4 and 24.2.
