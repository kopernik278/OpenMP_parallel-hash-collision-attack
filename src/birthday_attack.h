#ifndef BIRTHDAY_ATTACK_H
#define BIRTHDAY_ATTACK_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    int found;
    uint64_t nonce_a;
    uint64_t nonce_b;
    uint64_t trials_a;
    uint64_t trials_b;
    double seconds;
    int threads_used;
} attack_result_t;

attack_result_t birthday_attack(
    const unsigned char *file_a, size_t len_a,
    const unsigned char *file_b, size_t len_b,
    const char *student_id,
    int num_threads,
    uint64_t max_trials_per_side);

#endif
