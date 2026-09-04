# OpenMP Parallel Hash Collision Attack

CITS3402/CITS5507 Assignment 1 — a birthday-attack collision finder for the
intentionally weak 48-bit `toy_hash` function, with a single-threaded
baseline and an OpenMP-parallel implementation.

## Layout

```
src/
  toy_hash.c/.h         reference hash implementation (unmodified)
  pdf_io.c/.h            load/write PDFs, set nonce & student id fields
  hashtable.c/.h         lock-free open-addressing collision table
  birthday_attack.c/.h   shared birthday-attack search engine
  serial_main.c          single-threaded CLI (task 2)
  parallel_main.c        OpenMP CLI (task 3)
  config.h               STUDENT_ID (24914408)
scripts/
  solve_all.slurm         solves all six pairs on one Kaya node (96 cores)
  scaling_job.slurm        thread-count scaling benchmark on Kaya
report/
  report.md / report.pdf  1000-word report
*_a.pdf / *_b.pdf          the six provided file pairs, plus example_*.pdf
solved/                    output directory for solved pairs (created at runtime)
```

## Building

```
make
```

Produces `bin/birthday_serial` and `bin/birthday_parallel`. On Kaya, load a
compiler module first (e.g. `module load gcc`) so `cc`/`gcc` supports
`-fopenmp`. On macOS, `make` auto-detects Homebrew's `libomp`
(`brew install libomp` if you don't already have it).

`src/config.h` already contains the real student number (`24914408`) used
in every solved PDF and hash search, per the assignment's header
specification. If you fork this for a different submission, edit
`STUDENT_ID` there before building.

## Running

```
./bin/birthday_serial  <file_a> <file_b> <out_a> <out_b> [max_trials_per_side]
./bin/birthday_parallel <file_a> <file_b> <out_a> <out_b> [threads] [max_trials_per_side]
```

- `threads` defaults to the number of cores OpenMP reports available.
- `max_trials_per_side` defaults to 2^26 (~67 million), sized so a genuine
  collision is found with overwhelming probability (see report) — increase
  it if a run reports "no collision found".
- Both binaries print the winning nonces, the matching 48-bit hash, trial
  counts, thread count, and `search_seconds` — the search-only wall-clock
  time used for the report's timing figures (file I/O is excluded).
- Before writing the solved pair, the program recomputes `toy_hash` on the
  final modified bytes and aborts if the two hashes do not actually match.

Example:

```
mkdir -p solved
./bin/birthday_parallel 1_kilo_a.pdf 1_kilo_b.pdf solved/1_kilo_a.pdf solved/1_kilo_b.pdf 96
python3 check_toy_hash.py solved/1_kilo_a.pdf
python3 check_toy_hash.py solved/1_kilo_b.pdf
```

Both `check_toy_hash.py` calls should print the same 12-hex-digit hash.

## Solving all six pairs

```
make
mkdir -p solved
for pair in 1_kilo 2_mega 3_giga 4_tera 5_peta 6_exa; do
    ./bin/birthday_parallel "${pair}_a.pdf" "${pair}_b.pdf" \
        "solved/${pair}_a.pdf" "solved/${pair}_b.pdf" 96
done
```

or, on Kaya, submit `scripts/solve_all.slurm` (see below).

## Running on Kaya

You should do this yourself from a Kaya login node; the steps are:

1. **Copy the project to Kaya** (from your own machine, not this sandbox):
   ```
   git clone git@github.com:kopernik278/OpenMP_parallel-hash-collision-attack.git
   scp -r OpenMP_parallel-hash-collision-attack <username>@kaya.hpc.uwa.edu.au:~/
   ```
   or `git clone` directly on Kaya if it has outbound network access.

2. **Log in**:
   ```
   ssh <username>@kaya.hpc.uwa.edu.au
   cd OpenMP_parallel-hash-collision-attack
   ```

3. **Check `src/config.h`** — `STUDENT_ID` is already set to `24914408`.

4. **Check available compiler modules** (name may vary by cluster image):
   ```
   module avail gcc
   module load gcc
   ```
   Adjust the `module load gcc` line in `scripts/solve_all.slurm` and
   `scripts/scaling_job.slurm` to match whatever module Kaya offers.

5. **Submit the job to solve all six pairs** (one node, 96 cores, ≤15 min per
   pair, 2-hour overall budget):
   ```
   mkdir -p logs solved
   sbatch scripts/solve_all.slurm
   ```

6. **Check the queue / job status**:
   ```
   squeue -u $USER
   ```

7. **Once it finishes**, inspect the output and error logs:
   ```
   cat logs/birthday-solve_<jobid>.out
   cat logs/birthday-solve_<jobid>.err
   ```
   Confirm every pair printed `collision found` and that `search_seconds`
   for each pair is comfortably under 900 seconds (15 minutes).

8. **Run the thread-scaling benchmark** for the report (varies thread count
   1…96 across all six pairs, three repeats each, ~10 minutes total):
   ```
   sbatch scripts/scaling_job.slurm
   ```
   This writes `results/scaling_<jobid>.csv` with columns
   `pair,threads,repeat,search_seconds,trials_a,trials_b`. Use this CSV to
   fill in the performance section of `report/report.md`, then re-render
   `report/report.pdf` (e.g. `pandoc report/report.md -o report/report.pdf`).

9. **Copy the solved PDFs and results back** to your own machine for
   submission:
   ```
   scp -r <username>@kaya.hpc.uwa.edu.au:~/OpenMP_parallel-hash-collision-attack/solved .
   scp -r <username>@kaya.hpc.uwa.edu.au:~/OpenMP_parallel-hash-collision-attack/results .
   ```

10. Verify every solved pair one more time with `check_toy_hash.py` before
    zipping up the LMS submission.

## Submission checklist

`scripts/package_submission.sh` builds `submission_24914408.zip` containing
everything the assignment's Submission section requires: `src/`,
`Makefile`, the two Slurm scripts, `README.md`, `report/report.pdf`, and
`solved/` (the solved pairs). Run it any time from the repo root:

```
bash scripts/package_submission.sh
```

It warns (but still packages) if any of the six solved pairs are missing.
As of this repo's last commit, `solved/` contains `example`, `1_kilo`, and
`2_mega` (all verified against `check_toy_hash.py`), solved on a local
10-core machine — `3_giga`, `4_tera`, `5_peta`, and `6_exa` still need to be
produced **by you, on Kaya**, following the "Running on Kaya" steps above.
After `scripts/solve_all.slurm` finishes and you `scp` `solved/` back:

1. Copy the four missing pairs (and optionally re-solved `1_kilo`/`2_mega`
   for genuine Kaya timings) into this repo's `solved/` directory.
2. Fill the placeholder table in `report/report.md` with the real
   `results/scaling_<jobid>.csv` numbers, then re-render:
   `pandoc report/report.md -o report/report.pdf`.
3. Fill in your full name at the top of `report/report.md` (currently a
   `[Your Name]` placeholder — only the student number is filled in).
4. Re-run `bash scripts/package_submission.sh` to produce the final,
   complete `submission_24914408.zip` for LMS.

## Design summary

See `report/report.md` for the full description. In brief: a single shared,
lock-free, open-addressing hash table (48-bit `toy_hash` value → nonce,
owner file) is filled concurrently by threads generating nonce trials for
*both* files at once (threads are split into an A-group and a B-group).
Slots are claimed with a C11 atomic compare-and-swap; a per-slot `ready`
flag (release/acquire) publishes the nonce and owner only after the CAS
winner has written them, which is what makes concurrent readers of a
just-claimed slot safe without a lock. The search stops as soon as any
thread's insert finds an already-occupied slot from the *other* file with
the same hash — a genuine collision — which is then re-verified by
recomputing `toy_hash` on the final files before anything is written to
disk.
