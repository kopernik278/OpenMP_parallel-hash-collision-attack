#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdatomic.h>
#include <stdint.h>

#define TABLE_EMPTY_HASH UINT64_MAX

typedef struct {
    _Atomic uint64_t *hash_slot;
    _Atomic uint8_t *ready_slot;
    uint64_t *nonce_slot;
    uint8_t *owner_slot;
    uint64_t capacity;
    uint64_t mask;
} collision_table_t;

typedef struct {
    int collided;
    uint64_t other_nonce;
} insert_result_t;

uint64_t table_next_pow2(uint64_t v);
collision_table_t table_create(uint64_t min_capacity);
void table_destroy(collision_table_t *t);
insert_result_t table_insert(collision_table_t *t, uint64_t hash, uint64_t nonce, uint8_t owner);

#endif
