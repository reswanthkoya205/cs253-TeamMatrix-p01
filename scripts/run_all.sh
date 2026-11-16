#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
trap 'echo "FAIL at line $LINENO" >&2' ERR
run() { local label=$1; shift; printf '\n===== %s =====\n' "$label"; "$@"; printf '%s : PASS\n' "$label"; }
run Clean make -f Makefile clean
run Compilation make -f Makefile -j4
run 'Automated tests' make -f Makefile test
exe=bin/app
[[ ! -f bin/app.exe ]] || exe=bin/app.exe
run 'A Queue' "$exe" --input examples/small.txt --structure A
run 'A Heap' "$exe" --input examples/small.txt --structure A --heap
run 'B DFS' "$exe" --input examples/small.txt --structure B
commands=$(cat <<'COMMANDS'
show
order
schedule
critical
stats
add assets parse
show
stats
add compile assets
show
critical
stats
add compile assets
add parse link
add parse parse
add xyz parse
add parse
hello
show
quit
COMMANDS
)
run 'A Queue interactive' "$exe" --input examples/small.txt --structure A --interactive <<< "$commands"
run 'A Heap interactive' "$exe" --input examples/small.txt --structure A --heap --interactive <<< "$commands"
run 'B DFS interactive' "$exe" --input examples/small.txt --structure B --interactive <<< "$commands"
run 'Python libraries' python -c 'import pandas, matplotlib, numpy'
csv=(results/raw/struct_comparison.csv results/raw/incremental_comparison.csv)
fig=(results/figures/fig1_structure_comparison.png results/figures/fig2_incremental_vs_recomp.png)
marker=$(mktemp)
trap 'rm -f -- "$marker"' EXIT
check_files() {
    local file
    for file in "$@"; do
        if [[ -s "$file" && "$file" -nt "$marker" ]]; then
            printf '%s : PASS\n' "$file"
        else
            printf '%s : FAIL (missing, empty or not updated)\n' "$file" >&2
            return 1
        fi
    done
}
touch "$marker"
run 'Benchmarks and figure generation' make -f Makefile bench
check_files "${csv[@]}" "${fig[@]}"
touch "$marker"
run 'Independent plotting' make -f Makefile plots
check_files "${fig[@]}"
ls -l "${csv[@]}" "${fig[@]}"
printf '\nALL EXECUTION CHECKS PASSED\nALL BENCHMARK CSV FILES UPDATED\nALL FIGURES UPDATED SUCCESSFULLY\n'
