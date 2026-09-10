% CITS3402/CITS5507 Assignment 1: Parallel Hash Collision Attack
% Shaoming Wu, 24914408
% Semester 2, 2026

## Birthday-attack algorithm and collision-detection data structure

`toy_hash` produces a 48-bit output, so by the birthday bound a matching
pair between two independent families of nonce trials is expected after
roughly `sqrt(pi/2 * 2^48) ~ 2.1e7` combined trials — far fewer than 2^48
for a preimage search. Rather than fixing one file's nonce and searching
only the other, the program grows two trial pools in parallel, one per
file, stopping once a trial's hash already exists in the *other* file's
pool.

Both pools live in one shared open-addressing hash table keyed by the
48-bit hash. Each slot stores the hash, the nonce that produced it, which
file it came from, and a "ready" flag. Insertion mixes the hash with a
Fibonacci multiplier, masks it to the table size to pick a home slot, then
linear probes to the first free or matching slot. A match owned by the
*other* file is a genuine collision; one owned by the *same* file is a
redundant self-collision, discarded. Capacity is four times the trial
budget (`4 * 2^26` slots by default) so load factor stays at or below 0.5
and probe chains stay short. Nonces are generated sequentially via an
atomic counter per file rather than randomly — `toy_hash`'s
MurmurHash3-style finalising mix already scatters sequential inputs
uniformly, giving the same statistics as random sampling while wasting no
nonce on a repeat.

## Parallelisation and synchronisation with OpenMP

Threads split evenly into an "A-group" and a "B-group" (even/odd thread
id) inside one `#pragma omp parallel` region; each hashes trials only for
its file, drawing the next nonce with `atomic_fetch_add` on a per-file
counter — no two threads ever hash the same nonce, and none blocks.

The shared hash table is the only structure threads communicate through,
made safe without any `critical` section or `omp_lock_t`. Each slot's hash
field is a C11 `_Atomic uint64_t` written only via
`atomic_compare_exchange_strong` from `TABLE_EMPTY` to the trial's hash;
exactly one thread can win that CAS for a given slot, so that slot's
`nonce`/`owner` fields are subsequently written by that thread alone — no
two threads ever write the same memory. The remaining hazard is a reader
observing a slot as claimed before the winner finishes writing
`nonce`/`owner`; this is closed with a "published" flag — the winner
writes `nonce`/`owner`, then stores `ready = 1` with
`memory_order_release`, and a reader spins on `ready` with
`memory_order_acquire` first, so it never observes a torn write. That spin
window is only the few instructions between the CAS succeeding and the
stores after it, so it costs nothing in practice. A second atomic
(`found_flag`) is claimed with one final CAS so only the first thread to
find a collision records the winning pair; every other thread observes the
shared `stop_flag` and exits. Before writing any file, the program
independently recomputes `toy_hash` on the final bytes and aborts if they
disagree, so a concurrency bug can never silently produce an invalid
submission.

## Memory requirements and trade-offs

The dominant cost is the collision table: at the default
2^26-trials-per-side budget, capacity is 2^28 slots, each holding an
8-byte hash, 8-byte nonce, and two 1-byte tags — about 4.3 GB. This beats
a smaller, higher-load-factor table because linear probing degrades
sharply as load factor approaches 1, and Kaya nodes have far more memory
than this needs, so speed wins over economy. A second, much smaller cost
is per-thread: each thread keeps one private, mutable copy of whichever
file(s) it hashes (loaded once, header patched every trial), so only the
16-byte nonce is rewritten per trial rather than re-copying the whole file
— for the largest (~900 KB) pair at 96 threads this is under 90 MB,
negligible next to the table.

## Performance metrics and analysis

`scripts/scaling_job.slurm` measured `search_seconds` on Kaya (`cits3402`
partition, one 96-core node) across all six pairs and thread counts 1–96,
three repeats each, with a fixed per-pair trial budget (~8 s of
single-thread work) that isolates raw throughput from the attack's own
luck. Scaling is close to linear even for the hardest pair:

| threads | search_seconds (`6_exa`) | speedup |
|---:|---:|---:|
| 1 | 36.10 | 1.00x |
| 2 | 18.05 | 2.00x |
| 4 | 9.02 | 4.00x |
| 8 | 4.51 | 8.00x |
| 16 | 2.26 | 15.98x |
| 32 | 1.13 | 31.84x |
| 64 | 0.58 | 62.70x |
| 96 | 0.40 | 91.37x |

That is 95% parallel efficiency at 96 threads. Every other pair scales
just as well — 88.3% (`3_giga`) to 95.2% (`6_exa`) — and slightly
*improves* with file size, since a larger per-hash cost leaves threads
relatively less time touching the shared table (the one serialisation
point). This far exceeds a 10-core laptop, where efficiency fell to ~63%
by 10 threads: Kaya's server-class memory subsystem sustains many more
concurrent random accesses into the table before threads contend for
bandwidth.

Full end-to-end solves (student ID `24914408`, verified against
`check_toy_hash.py`) confirm correctness, not just throughput: on the
10-core laptop, `1_kilo` took 999.97 s (25.32M trials/side) and the larger
`2_mega` only 747.5 s (11.41M trials/side) — trial count is a random
variable around the ~2.1e7-combined mean, not a function of file size
alone, so wall-clock time must be measured per pair. At Kaya's measured
96-thread throughput, every pair — including `6_exa` — comfortably fits
the 900 s budget even on an above-average-trial-count run, which is why
harder pairs need the full node rather than fewer threads.
