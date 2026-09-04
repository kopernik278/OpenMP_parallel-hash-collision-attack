#include "birthday_attack.h"
#include "config.h"
#include "pdf_io.h"
#include "toy_hash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc < 5) {
        fprintf(stderr, "usage: %s <file_a> <file_b> <out_a> <out_b> [max_trials_per_side]\n", argv[0]);
        return 1;
    }

    const char *path_a = argv[1];
    const char *path_b = argv[2];
    const char *out_a = argv[3];
    const char *out_b = argv[4];
    uint64_t max_trials = (argc > 5) ? strtoull(argv[5], NULL, 10) : ((uint64_t)1 << 26);

    pdf_buffer_t a = pdf_load(path_a);
    pdf_buffer_t b = pdf_load(path_b);

    attack_result_t r = birthday_attack(a.data, a.size, b.data, b.size, STUDENT_ID, 1, max_trials);

    if (!r.found) {
        fprintf(stderr, "no collision found within %llu trials per side\n", (unsigned long long)max_trials);
        return 2;
    }

    unsigned char *solved_a = malloc(a.size);
    memcpy(solved_a, a.data, a.size);
    pdf_set_student_id(solved_a, STUDENT_ID);
    pdf_set_nonce(solved_a, r.nonce_a);

    unsigned char *solved_b = malloc(b.size);
    memcpy(solved_b, b.data, b.size);
    pdf_set_student_id(solved_b, STUDENT_ID);
    pdf_set_nonce(solved_b, r.nonce_b);

    uint64_t hash_a = toy_hash(solved_a, a.size);
    uint64_t hash_b = toy_hash(solved_b, b.size);

    if (hash_a != hash_b) {
        fprintf(stderr, "verification failed: %012llx != %012llx\n",
                (unsigned long long)hash_a, (unsigned long long)hash_b);
        return 3;
    }

    pdf_write(out_a, solved_a, a.size);
    pdf_write(out_b, solved_b, b.size);

    printf("collision found\n");
    printf("nonce_a=%016llx\n", (unsigned long long)r.nonce_a);
    printf("nonce_b=%016llx\n", (unsigned long long)r.nonce_b);
    printf("hash=%012llx\n", (unsigned long long)hash_a);
    printf("trials_a=%llu\n", (unsigned long long)r.trials_a);
    printf("trials_b=%llu\n", (unsigned long long)r.trials_b);
    printf("threads=1\n");
    printf("search_seconds=%.6f\n", r.seconds);

    free(solved_a);
    free(solved_b);
    pdf_free(&a);
    pdf_free(&b);
    return 0;
}
