#!/bin/bash --login
#SBATCH --job-name=birthday-scaling
#SBATCH --partition=cits3402
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=96
#SBATCH --time=01:30:00
#SBATCH --output=logs/%x_%j.out
#SBATCH --error=logs/%x_%j.err

module load gcc

cd "$SLURM_SUBMIT_DIR"
mkdir -p logs results

make

export OMP_PROC_BIND=close
export OMP_PLACES=cores

PAIRS=(1_kilo 2_mega 3_giga 4_tera 5_peta 6_exa)
THREAD_COUNTS=(1 2 4 8 16 24 32 48 64 96)
BYTES_PER_SECOND_PER_CORE=1000000000
TARGET_SECONDS=8
CSV=results/scaling_$SLURM_JOB_ID.csv

echo "pair,threads,repeat,search_seconds,trials_a,trials_b" > "$CSV"

for pair in "${PAIRS[@]}"; do
    file_size=$(stat -c%s "${pair}_a.pdf")
    max_trials=$(( BYTES_PER_SECOND_PER_CORE * TARGET_SECONDS / file_size / 2 ))
    if [ "$max_trials" -lt 20000 ]; then
        max_trials=20000
    fi

    for threads in "${THREAD_COUNTS[@]}"; do
        for repeat in 1 2 3; do
            OMP_NUM_THREADS=$threads \
            ./bin/birthday_parallel \
                "${pair}_a.pdf" "${pair}_b.pdf" \
                "/tmp/${pair}_a_scaling.pdf" "/tmp/${pair}_b_scaling.pdf" \
                "$threads" "$max_trials" > /tmp/scaling_out.txt

            seconds=$(grep search_seconds /tmp/scaling_out.txt | cut -d= -f2)
            trials_a=$(grep trials_a /tmp/scaling_out.txt | cut -d= -f2)
            trials_b=$(grep trials_b /tmp/scaling_out.txt | cut -d= -f2)

            echo "${pair},${threads},${repeat},${seconds},${trials_a},${trials_b}" >> "$CSV"
            echo "${pair} threads=${threads} repeat=${repeat} seconds=${seconds} max_trials=${max_trials}"
        done
    done
done
