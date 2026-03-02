//
// Copyright (c) 2026 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#undef NDEBUG
#include <assert.h>

// ── Instantiate uint32_t -> uint32_t, 8 buckets, capacity 12 ─────────────────

static inline size_t u32_hash(const uint32_t *k) {
    uint32_t x = *k;
    x ^= x >> 16;
    x *= 0x45d9f3bU;
    x ^= x >> 16;
    return (size_t)x;
}
static inline bool u32_eq(const uint32_t *a, const uint32_t *b) { return *a == *b; }

#define _ZP_HASHMAP_TEMPLATE_KEY_TYPE uint32_t
#define _ZP_HASHMAP_TEMPLATE_VAL_TYPE uint32_t
#define _ZP_HASHMAP_TEMPLATE_NAME u32map
#define _ZP_HASHMAP_TEMPLATE_BUCKET_COUNT 8
#define _ZP_HASHMAP_TEMPLATE_CAPACITY 12
#define _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN_NAME u32_hash
#define _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN_NAME u32_eq
#include "zenoh-pico/collections/hashmap_template.h"

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_new_is_empty(void) {
    printf("Test: new map is empty\n");
    u32map_t m = u32map_new();
    assert(u32map_is_empty(&m));
    assert(u32map_size(&m) == 0);
    u32map_destroy(&m);
}

static void test_insert_and_get(void) {
    printf("Test: insert then get returns value\n");
    u32map_t m = u32map_new();
    uint32_t k = 1, v = 10;
    assert(u32map_insert(&m, &k, &v));
    assert(u32map_size(&m) == 1);
    uint32_t *got = u32map_get(&m, &(uint32_t){1});
    assert(got != NULL && *got == 10);
    u32map_destroy(&m);
}

static void test_get_missing_returns_null(void) {
    printf("Test: get on missing key returns NULL\n");
    u32map_t m = u32map_new();
    assert(u32map_get(&m, &(uint32_t){42}) == NULL);
    u32map_destroy(&m);
}

static void test_insert_updates_existing(void) {
    printf("Test: insert with duplicate key updates value, size stays same\n");
    u32map_t m = u32map_new();
    uint32_t k = 5, v1 = 50, v2 = 99;
    assert(u32map_insert(&m, &k, &v1));
    assert(u32map_insert(&m, &(uint32_t){5}, &v2));
    assert(u32map_size(&m) == 1);
    assert(*u32map_get(&m, &(uint32_t){5}) == 99);
    u32map_destroy(&m);
}

static void test_remove_existing(void) {
    printf("Test: remove existing key moves value out\n");
    u32map_t m = u32map_new();
    uint32_t k = 7, v = 70;
    assert(u32map_insert(&m, &k, &v));
    uint32_t out = 0;
    assert(u32map_remove(&m, &(uint32_t){7}, &out));
    assert(out == 70);
    assert(u32map_size(&m) == 0);
    assert(u32map_get(&m, &(uint32_t){7}) == NULL);
    u32map_destroy(&m);
}

static void test_remove_missing_returns_false(void) {
    printf("Test: remove on missing key returns false\n");
    u32map_t m = u32map_new();
    assert(!u32map_remove(&m, &(uint32_t){99}, NULL));
    u32map_destroy(&m);
}

static void test_remove_head_of_chain(void) {
    printf("Test: remove head of a collision chain; remaining entry still accessible\n");
    u32map_t m = u32map_new();
    // Keys 0 and 8 both hash to bucket 0 with BUCKET_COUNT=8 (hash%8 == 0 for both)
    uint32_t k0 = 0, v0 = 100;
    uint32_t k8 = 8, v8 = 800;
    assert(u32map_insert(&m, &k0, &v0));
    assert(u32map_insert(&m, &k8, &v8));
    assert(u32map_size(&m) == 2);
    // Remove whichever is the head (k8, inserted last → prepended)
    assert(u32map_remove(&m, &(uint32_t){8}, NULL));
    assert(u32map_size(&m) == 1);
    assert(*u32map_get(&m, &(uint32_t){0}) == 100);
    assert(u32map_get(&m, &(uint32_t){8}) == NULL);
    u32map_destroy(&m);
}

static void test_remove_tail_of_chain(void) {
    printf("Test: remove tail of a collision chain; head still accessible\n");
    u32map_t m = u32map_new();
    uint32_t k0 = 0, v0 = 100;
    uint32_t k8 = 8, v8 = 800;
    assert(u32map_insert(&m, &k0, &v0));
    assert(u32map_insert(&m, &k8, &v8));
    assert(u32map_remove(&m, &(uint32_t){0}, NULL));  // tail (inserted first)
    assert(u32map_size(&m) == 1);
    assert(*u32map_get(&m, &(uint32_t){8}) == 800);
    assert(u32map_get(&m, &(uint32_t){0}) == NULL);
    u32map_destroy(&m);
}

static void test_contains(void) {
    printf("Test: contains\n");
    u32map_t m = u32map_new();
    assert(!u32map_contains(&m, &(uint32_t){3}));
    uint32_t k = 3, v = 30;
    assert(u32map_insert(&m, &k, &v));
    assert(u32map_contains(&m, &(uint32_t){3}));
    assert(u32map_remove(&m, &(uint32_t){3}, NULL));
    assert(!u32map_contains(&m, &(uint32_t){3}));
    u32map_destroy(&m);
}

static void test_clear_and_reuse(void) {
    printf("Test: clear empties the map and frees pool for reuse\n");
    u32map_t m = u32map_new();
    for (uint32_t i = 0; i < 12; i++) {
        uint32_t k = i, v = i * 10;
        assert(u32map_insert(&m, &k, &v));
    }
    assert(u32map_size(&m) == 12);
    u32map_destroy(&m);
    assert(u32map_is_empty(&m));
    // Pool fully freed — all 12 slots available again
    for (uint32_t i = 0; i < 12; i++) {
        uint32_t k = i + 100, v = i;
        assert(u32map_insert(&m, &k, &v));
    }
    assert(u32map_size(&m) == 12);
    u32map_destroy(&m);
}

static void test_pool_exhaustion(void) {
    printf("Test: insert fails when pool is exhausted\n");
    u32map_t m = u32map_new();
    for (uint32_t i = 0; i < 12; i++) {
        uint32_t k = i, v = i;
        assert(u32map_insert(&m, &k, &v));
    }
    // One more distinct key must fail
    uint32_t k = 200, v = 200;
    assert(!u32map_insert(&m, &k, &v));
    // Updating an existing key must still succeed (no new pool slot needed)
    uint32_t ku = 0, vu = 255;
    assert(u32map_insert(&m, &ku, &vu));
    assert(*u32map_get(&m, &(uint32_t){0}) == 255);
    u32map_destroy(&m);
}

static void test_pool_slot_reused_after_remove(void) {
    printf("Test: pool slot freed by remove is reused by subsequent insert\n");
    u32map_t m = u32map_new();
    // Fill to capacity
    for (uint32_t i = 0; i < 12; i++) {
        uint32_t k = i, v = i;
        assert(u32map_insert(&m, &k, &v));
    }
    // Remove one entry to free a slot
    assert(u32map_remove(&m, &(uint32_t){0}, NULL));
    assert(u32map_size(&m) == 11);
    // Now a new key must succeed
    uint32_t k = 200, v = 200;
    assert(u32map_insert(&m, &k, &v));
    assert(u32map_size(&m) == 12);
    assert(*u32map_get(&m, &(uint32_t){200}) == 200);
    u32map_destroy(&m);
}

static void test_multiple_collisions(void) {
    printf("Test: many keys colliding into the same bucket\n");
    u32map_t m = u32map_new();
    // With BUCKET_COUNT=8, keys 0,8,16,24,32,40 all land in bucket 0
    uint32_t keys[] = {0, 8, 16, 24, 32, 40};
    for (size_t i = 0; i < 6; i++) {
        uint32_t k = keys[i], v = keys[i] * 10;
        assert(u32map_insert(&m, &k, &v));
    }
    assert(u32map_size(&m) == 6);
    for (size_t i = 0; i < 6; i++) {
        uint32_t *got = u32map_get(&m, &keys[i]);
        assert(got != NULL && *got == keys[i] * 10);
    }
    // Remove middle entries and verify survivors
    assert(u32map_remove(&m, &(uint32_t){16}, NULL));
    assert(u32map_remove(&m, &(uint32_t){32}, NULL));
    assert(u32map_size(&m) == 4);
    assert(u32map_get(&m, &(uint32_t){16}) == NULL);
    assert(u32map_get(&m, &(uint32_t){32}) == NULL);
    assert(*u32map_get(&m, &(uint32_t){0}) == 0);
    assert(*u32map_get(&m, &(uint32_t){8}) == 80);
    assert(*u32map_get(&m, &(uint32_t){24}) == 240);
    assert(*u32map_get(&m, &(uint32_t){40}) == 400);
    u32map_destroy(&m);
}

int main(void) {
    test_new_is_empty();
    test_insert_and_get();
    test_get_missing_returns_null();
    test_insert_updates_existing();
    test_remove_existing();
    test_remove_missing_returns_false();
    test_remove_head_of_chain();
    test_remove_tail_of_chain();
    test_contains();
    test_clear_and_reuse();
    test_pool_exhaustion();
    test_pool_slot_reused_after_remove();
    test_multiple_collisions();
    return 0;
}
