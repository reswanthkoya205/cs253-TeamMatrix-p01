# TOOLS.md: Declaration of AI and Other Tools

**Project:** Project 01 — Build Ordering and Critical Paths  
**Team:** TeamMatrix  
**Course:** CS 253 / MA 253 Data Structures and Algorithms Laboratory  
**Institute:** IIT Indore  

All AI use is declared below. Everything an AI tool produced was a draft or a formatting aid; the team wrote, ran and can explain the final code, derivations and measurements.

# Tools used

| Tool and version | Used by | Used for | Files affected | What we changed or verified |
| :--- | :--- | :--- | :--- | :--- |
| Gemini (Advanced, conversational interface, Nov 2026) | M. Varnith Reddy (Architect) | Header-guard naming and Doxygen comment layout; Markdown formatting of the invariant tables; Makefile dependency syntax | `include/graph.hpp`, `INTERFACE.md`, `Makefile` | Formatting and scaffolding only. We designed the interface and invariants ourselves and tested the code against the oracle. |
| Gemini (Advanced, conversational interface, Nov 2026) | P. Eswar Mukesh (Structure A owner) | Skeleton and syntax checks for the Kahn queue and min-heap code | `src/structA/` | We wrote the algorithm logic, our own FIFO queue and min-heap, and verified them with the oracle and invariant checkers. |
| CHATGPT | Boinapally Sricharan Rao (Structure B owner) | Skeleton and syntax checks for the iterative DFS and incremental-order code | `src/structB/` | We wrote the DFS and Pearce–Kelly reordering logic and verified them with the oracle and invariant checkers. |
| ChatGPT (OpenAI, GPT-6) | MV Karthik(250001046,Oracle Owner) | Understanding algorithms, code assistance, reviewing validators and tests, debugging builds | oracle/oracle.cpp, oracle/validators.cpp, tests/*.cpp, Makefile | Reviewed topological sorting, cycle detection, critical paths, validation, differential/edge/adversarial/incremental/large-scale tests; checked build errors and test execution. |
| Visual Studio Code (version to verify) | Oracle Owner | Editing and running project code | C++ source/header files, Makefile | Edited files and inspected compiler/test output. |
| g++ (version to verify; C++17) and GNU Make (version to verify) | Oracle Owner | Compiling and running automated tests | C++ source/header files, Makefile | Built modules and ran test targets; addressed compilation and linking issues. |
| Git and GitHub (Git version to verify) | Oracle Owner | Version control and team integration | Project repository | Managed oracle branch, commits, pushes, pulls and merges. |
| Windows PowerShell (version to verify) | Oracle Owner | Running development commands | Build and test workflow | Executed build/test commands and investigated Windows shell issues. |
| *Google Gemini* (Web Assistant, 2026) | NADIPILLI DINAKAR PRASHANTH (250001051, Adversary) | Implementing deterministic C++17 adversarial graph generators (genExponentialDAG, genHiddenCycleBehindChain, genWorstInsertions, genRandomDAG, genRandomGraph), drafting Derivation D5 proofs, Git PR workflow guidance, and generating presentation slides. | include/generators.hpp, adversary/generators.cpp, docs/derivations/D5_250001051.pdf | Verified strict compliance with INTERFACE.md signatures, deterministic 64-bit mt19937_64 seeding across platforms, O(1) 64-bit edge deduplication, inductive proof steps in D5, and clean compilation with zero warnings. |
| Gemini (Advanced, conversational interface, Nov 2026) | Koya Reswanth (Analyst) | Markdown and LaTeX formatting of the Section 9 cost-model equations; Git workflow troubleshooting | `REPORT.md` (Section 9) | The cost model and its constants are our own; equations checked by hand against our notation. |
| Claude (Anthropic, Claude Sonnet 5.5, Nov 2026) | Koya Reswanth (Analyst) | Draft and arithmetic check of the Section 9 cost model (break-even $m^*$, heap-overhead ratio); draft structure of `INTERFACE.md` for team discussion; update of this `TOOLS.md` | `REPORT.md` (Section 9 wording and checks), `TOOLS.md` | Re-derived each formula by hand; the committed `REPORT.md` and `INTERFACE.md` were reviewed and edited by the team. Constants come from our own calibration runs. |
| GNU `g++` (C++17), flags `-std=c++17 -O3 -Wall -Wextra` | All members | Compiling application, tests, oracle and benchmarks | `Makefile` | Builds cleanly with warnings enabled. |
| GNU Make (Linux and MSYS2 on Windows) | M. Varnith Reddy, Koya Reswanth | Build, test, bench, plots and clean targets | `Makefile` | Verified per-target dependencies; `make test` stops at the first failing test. |
| Bash script `scripts/run_all.sh` | Koya Reswanth, M. Varnith Reddy | One command: clean, build, test, run the app, benchmark, regenerate figures | `scripts/run_all.sh` | Checks that every CSV and figure was actually updated by the run. |
| Valgrind 3.26.0 (Cachegrind, `cg_annotate`) | Koya Reswanth (Analyst) | Instruction-count profiling of both benchmark programs | `results/cachegrind_*.txt` | Used the counts to separate random-graph generation cost from Structure A and B cost. |
| Python 3 (`pandas`, `matplotlib`, `numpy`) | Koya Reswanth (Analyst) | Reading the raw CSVs and drawing the figures | `scripts/generate_figures.py`, `results/figures/` | Offline plotting only; no Python code implements a structure under study. |
| `std::chrono` (C++ standard library) | Koya Reswanth (Analyst) | Benchmark timing | `bench/*.cpp` | The same timer is used for every structure compared. |
| Git and GitHub | All members | Version control; branches and pull requests | whole repository | Each role was developed on its own branch and merged by pull request. |
| Visual Studio Code | All members | Editing source files | none | None. |
| ReportLab, Google Docs, LaTeX (pdfTeX), Microsoft Word 2021 with `python-docx` | Derivation authors | Producing the derivation PDFs: D1 ReportLab, D2/D5/D6 Google Docs, D3 LaTeX, D4 Word | `docs/derivations/` | Content of each derivation written and checked by its author. |


# Representative prompts

## Gemini
* **2026-11-07** "Draft the formal structural invariants for Kahn's topological sort and DFS finish-time ordering in Markdown format."
* **2026-11-01** "Provide a clean Markdown template for Section 9 predicted-cost models in our lab project report."
* **2026-11-10** "Formulate the asymptotic time complexity equations for incremental topological order maintenance (Pearce–Kelly) in Markdown LaTeX."

# Library policy
No library container stands in for a structure under study. Structure A uses our own FIFO queue (`src/structA/fifo_queue.hpp`) and binary min-heap (`src/structA/min_heap.hpp`); `std::queue`, `std::stack` and `std::priority_queue` are not used in `src/` or `include/`. The standard library is used for `std::vector` storage, `<algorithm>`, `<random>` and `std::unordered_set` (adversary generator), `<chrono>` (benchmarks) and stream/string utilities. Python packages are used only for offline plotting.