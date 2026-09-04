#include "hashtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint64_t table_next_pow2(uint64_t v)
{
    uint64_t p = 1;
    while (p < v) {
        p <<= 1;
    }
    return p;
}

collision_table_t table_create(uint64_t min_capacity)
{
    collision_table_t t;
    t.capacity = table_next_pow2(min_capacity);
    t.mask = t.capacity - 1;

    t.hash_slot = malloc(t.capacity * sizeof(*t.hash_slot));
    t.ready_slot = malloc(t.capacity * sizeof(*t.ready_slot));
    t.nonce_slot = malloc(t.capacity * sizeof(*t.nonce_slot));
    t.owner_slot = malloc(t.capacity * sizeof(*t.owner_slot));

    if (!t.hash_slot || !t.ready_slot || !t.nonce_slot || !t.owner_slot) {
        fprintf(stderr, "out of memory allocating collision table of %llu slots\n",
                (unsigned long long)t.capacity);
        exit(1);
    }

    memset(t.hash_slot, 0xff, t.capacity * sizeof(*t.hash_slot));
    memset(t.ready_slot, 0, t.capacity * sizeof(*t.ready_slot));

    return t;
}

void table_destroy(collision_table_t *t)
{
    free(t->hash_slot);
    free(t->ready_slot);
    free(t->nonce_slot);
    free(t->owner_slot);
    t->hash_slot = NULL;
    t->ready_slot = NULL;
    t->nonce_slot = NULL;
    t->owner_slot = NULL;
    t->capacity = 0;
    t->mask = 0;
}

static inline uint64_t table_index(const collision_table_t *t, uint64_t hash)
{
    uint64_t mixed = hash * 0x9e3779b97f4a7c15ULL;
    mixed ^= mixed >> 32;
    return mixed & t->mask;
}

insert_result_t table_insert(collision_table_t *t, uint64_t hash, uint64_t nonce, uint8_t owner)
{
    insert_result_t result = {0, 0};
    uint64_t idx = table_index(t, hash);

    for (uint64_t probes = 0; probes <= t->mask; ++probes) {
        uint64_t expected = TABLE_EMPTY_HASH;
        if (atomic_compare_exchange_strong_explicit(
                &t->hash_slot[idx], &expected, hash,
                memory_order_relaxed, memory_order_relaxed)) {
            t->nonce_slot[idx] = nonce;
            t->owner_slot[idx] = owner;
            atomic_store_explicit(&t->ready_slot[idx], 1, memory_order_release);
            return result;
        }

        if (expected == hash) {
            while (!atomic_load_explicit(&t->ready_slot[idx], memory_order_acquire)) {
            }
            if (t->owner_slot[idx] != owner) {
                result.collided = 1;
                result.other_nonce = t->nonce_slot[idx];
            }
            return result;
        }

        idx = (idx + 1) & t->mask;
    }

    fprintf(stderr, "collision table exhausted (capacity=%llu)\n", (unsigned long long)t->capacity);
    exit(1);
}
