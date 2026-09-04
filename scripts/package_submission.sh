#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

STUDENT_ID=24914408
OUT="submission_${STUDENT_ID}.zip"

REQUIRED_PAIRS=(1_kilo 2_mega 3_giga 4_tera 5_peta 6_exa)
missing=0
for pair in "${REQUIRED_PAIRS[@]}"; do
    if [ ! -f "solved/${pair}_a.pdf" ] || [ ! -f "solved/${pair}_b.pdf" ]; then
        echo "warning: solved/${pair}_a.pdf / _b.pdf missing" >&2
        missing=1
    fi
done
if [ "$missing" -eq 1 ]; then
    echo "warning: packaging an incomplete submission (see missing pairs above)" >&2
fi

rm -f "$OUT"
zip -r "$OUT" \
    src \
    Makefile \
    scripts/solve_all.slurm \
    scripts/scaling_job.slurm \
    README.md \
    report/report.pdf \
    solved \
    check_toy_hash.py \
    -x '*.DS_Store' '*.gitkeep'

echo "wrote ${OUT}"
