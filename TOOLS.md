# TOOLS.md — Declaration of AI and External Tools

## Team Details
* **Team Name:** TeamMatrix
* **Project:** Project 01 — Build Ordering and Critical Paths
* **Course:** CS 253 / MA 253 Data Structures and Algorithms Laboratory
* **Institute:** Indian Institute of Technology Indore (IIT Indore)

---

## 1. Tool Declarations

### Gemini (Google Large Language Model)
* **Model / Version:** Gemini (Advanced / Conversational Interface)
* **Purpose:**
  * Formatting Markdown tables and scaffolding template headers for `README.md` and `REPORT.md`.
  * Formatting LaTeX equations and mathematical derivations (e.g., Kahn's topological sort complexity bounds, Pearce-Kelly incremental sort amortized cost models, and critical path slack formulations).
  * Checking standard C++ `#ifndef` header guard naming conventions and modular layout patterns.
  * Assisting with Git workflow troubleshooting and Makefile dependency setups.

* **Representative Prompts:**
  * *"Draft the formal structural invariants for Kahn's topological sort and DFS finish-time ordering in Markdown format."*
  * *"Provide a clean Markdown template for Section 9 predicted-cost models in our lab project report."*
  * *"Formulate the asymptotic time complexity equations for incremental topological order maintenance (Pearce-Kelly) in Markdown LaTeX."*

* **Files / Modules Containing Tool-Assisted Content:**
  * `include/graph.hpp`: Header guard syntax, Doxygen documentation comment formatting, and interface definitions.
  * `INTERFACE.md`: Formatting of Markdown math tables and invariant definitions.
  * `REPORT.md`: Section outline structure and LaTeX formatting of Section 9 (Predicted-Cost Model).
  * `src/structA/` and `src/structB/`: Structural design scaffolding and syntax checks.

* **Human Verification:**
  * All algorithmic logic, core method signatures, structural invariants, cost models, and C++ source code implementations were written, reviewed, tested against oracles, and fully verified by team members before committing. No raw AI-generated code was pushed without manual testing.

---

## 2. Other Third-Party Libraries and Tools
* **Development Environment & Version Control:** 
  * Visual Studio Code (VS Code) on Windows with Git Bash (MinGW64) for local compilation and Git version control.
* **Build & Test Tools:** 
  * GNU Make and `g++` (supporting C++17 or higher) with `-O2` optimization flags for compilation and benchmarking.
* **External Libraries:** 
  * **None.** No external data structure, graph, or container libraries (such as Boost, pre-built graph algorithms, or standard library substitutes for the structures under study) were used. All graph representations, queues, heaps, DFS stacks, and incremental update handlers are implemented completely from first principles using standard C++ raw arrays/pointers and STL containers strictly for testing/generation wrappers as permitted by guidelines.