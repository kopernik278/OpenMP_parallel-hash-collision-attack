#include "birthday_attack.h"

#include "hashtable.h"
#include "pdf_io.h"
#include "toy_hash.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _OPENMP
#include <omp.h>
#endif

static double monotonic_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

attack_result_t birthday_attack(
    const unsigned char *file_a, size_t len_a,
    const unsigned char *file_b, size_t len_b,
    const char *student_id,
    int num_threads,
    uint64_t max_trials_per_side)
{
    attack_result_t result;
    memset(&result, 0, sizeof(result));

    collision_table_t table = table_create(4 * max_trials_per_side);

    _Atomic uint64_t next_nonce[2];
    atomic_init(&next_nonce[0], (uint64_t)0);
    atomic_init(&next_nonce[1], (uint64_t)0);
    _Atomic int found_flag;
    atomic_init(&found_flag, 0);
    _Atomic int stop_flag;
    atomic_init(&stop_flag, 0);

    uint64_t out_nonce_a = 0;
    uint64_t out_nonce_b = 0;
    uint64_t trials_a = 0;
    uint64_t trials_b = 0;

#ifdef _OPENMP
    if (num_threads > 0) {
        omp_set_num_threads(num_threads);
    }
#else
    (void)num_threads;
#endif

    double t0 = monotonic_seconds();

    #pragma omp parallel reduction(+:trials_a, trials_b)
    {
#ifdef _OPENMP
        int tid = omp_get_thread_num();
        int nthreads = omp_get_num_threads();
#else
        int tid = 0;
        int nthreads = 1;
#endif
        int owner = (nthreads > 1) ? (tid % 2) : -1;

        unsigned char *scratch_a = NULL;
        unsigned char *scratch_b = NULL;

        if (owner != 1) {
            scratch_a = malloc(len_a);
            memcpy(scratch_a, file_a, len_a);
            pdf_set_student_id(scratch_a, student_id);
        }
        if (owner != 0) {
            scratch_b = malloc(len_b);
            memcpy(scratch_b, file_b, len_b);
            pdf_set_student_id(scratch_b, student_id);
        }

        while (!atomic_load_explicit(&stop_flag, memory_order_relaxed)) {
            int side = (owner != -1) ? owner : (trials_a <= trials_b ? 0 : 1);

            uint64_t nonce = atomic_fetch_add_explicit(&next_nonce[side], (uint64_t)1,
                                                         memory_order_relaxed);
            if (nonce >= max_trials_per_side) {
                atomic_store_explicit(&stop_flag, 1, memory_order_relaxed);
                break;
            }

            unsigned char *buf = (side == 0) ? scratch_a : scratch_b;
            size_t len = (side == 0) ? len_a : len_b;

            pdf_set_nonce(buf, nonce);
            uint64_t hash = toy_hash(buf, len);

            if (side == 0) {
                trials_a++;
            } else {
                trials_b++;
            }

            insert_result_t r = table_insert(&table, hash, nonce, (uint8_t)side);
            if (r.collided) {
                int expected = 0;
                if (atomic_compare_exchange_strong_explicit(
                        &found_flag, &expected, 1,
                        memory_order_acq_rel, memory_order_relaxed)) {
                    if (side == 0) {
                        out_nonce_a = nonce;
                        out_nonce_b = r.other_nonce;
                    } else {
                        out_nonce_b = nonce;
                        out_nonce_a = r.other_nonce;
                    }
                }
                atomic_store_explicit(&stop_flag, 1, memory_order_relaxed);
                break;
            }
        }

        free(scratch_a);
        free(scratch_b);
    }

    double t1 = monotonic_seconds();

    result.found = atomic_load_explicit(&found_flag, memory_order_relaxed);
    result.nonce_a = out_nonce_a;
    result.nonce_b = out_nonce_b;
    result.trials_a = trials_a;
    result.trials_b = trials_b;
    result.seconds = t1 - t0;
#ifdef _OPENMP
    result.threads_used = (num_threads > 0) ? num_threads : omp_get_max_threads();
#else
    result.threads_used = 1;
#endif

    table_destroy(&table);
    return result;
}
