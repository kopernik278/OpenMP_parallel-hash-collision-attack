#!/bin/bash --login
#SBATCH --job-name=birthday-solve
#SBATCH --partition=work
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=96
#SBATCH --time=02:00:00
#SBATCH --output=logs/%x_%j.out
#SBATCH --error=logs/%x_%j.err

module load gcc

cd "$SLURM_SUBMIT_DIR"
mkdir -p solved logs

make clean
make

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export OMP_PROC_BIND=close
export OMP_PLACES=cores

PAIRS=(1_kilo 2_mega 3_giga 4_tera 5_peta 6_exa)

for pair in "${PAIRS[@]}"; do
    echo "=== ${pair} : ${SLURM_CPUS_PER_TASK} threads ==="
    /usr/bin/time -v ./bin/birthday_parallel \
        "${pair}_a.pdf" "${pair}_b.pdf" \
        "solved/${pair}_a.pdf" "solved/${pair}_b.pdf" \
        "$SLURM_CPUS_PER_TASK"
done
