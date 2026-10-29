# Project 01: Build Ordering and Critical Paths
Team TeamMatrix — CS 253 / MA 253, Autumn 2026

## Team and roles
| Name                     | Roll no.      | GitHub               | Role              | Module folder      | Derivation |
|--------------------------|---------------|----------------------|-------------------|--------------------|------------|
| P.Eswar Mukesh           | 250001053     | @Eswarmukesh14       | Structure A owner | `src/structA/`     | D1         |
| Boinapally Sricharan Rao | 250001018     | @sricharanrao01-web  | Structure B owner | `src/structB/`     | D2         |
| M.Varnith Reddy          | 250001045     | @7127Vzs             | Architect         | `include/, app/`   | D3         |
| MV.Karthik               | 250001046     | @karthikmvs008       | Oracle owner      | `oracle/, tests/`  | D4         |
| N.Dinakar Prashanth      | 250001051     | @dinakarprashanth19  | Adversary         | `adversary/`       | D5         |
| Koya Reswanth            | 250001042     | @reswanthkoya205     | Analyst           | `bench/, results/` | D6         |

## Summary
This project implements and compares two dynamic graph algorithms for build system ordering: Kahn's algorithm with a queue/min-heap (Structure A) and an iterative DFS finish-time ordering with Pearce-Kelly incremental maintenance (Structure B). It constructs valid build sequences, extracts dependency cycles, and computes earliest start times, latest start times, slack, and critical paths. Our experimental evaluation highlights the trade-offs between full recomputation and incremental reordering as dependency graphs grow under continuous edge insertions.

















