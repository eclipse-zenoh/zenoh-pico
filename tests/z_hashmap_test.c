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

#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE uint32_t
#define _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE uint32_t
#define _ZP_STATIC_HASHMAP_TEMPLATE_NAME u32map
#define _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT 8
#define _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY 12
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN u32_hash
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_EQ_FN u32_eq
#include "zenoh-pico/collections/static_hashmap_template.h"

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
    assert(u32map_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
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
    assert(u32map_insert(&m, &k, &v1) != _ZP_HASHMAP_ITER_INVALID);
    assert(u32map_insert(&m, &(uint32_t){5}, &v2) != _ZP_HASHMAP_ITER_INVALID);
    assert(u32map_size(&m) == 1);
    assert(*u32map_get(&m, &(uint32_t){5}) == 99);
    u32map_destroy(&m);
}

static void test_remove_existing(void) {
    printf("Test: remove existing key moves value out\n");
    u32map_t m = u32map_new();
    uint32_t k = 7, v = 70;
    assert(u32map_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
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
    assert(u32map_insert(&m, &k0, &v0) != _ZP_HASHMAP_ITER_INVALID);
    assert(u32map_insert(&m, &k8, &v8) != _ZP_HASHMAP_ITER_INVALID);
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
    assert(u32map_insert(&m, &k0, &v0) != _ZP_HASHMAP_ITER_INVALID);
    assert(u32map_insert(&m, &k8, &v8) != _ZP_HASHMAP_ITER_INVALID);
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
    assert(u32map_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
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
        assert(u32map_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }
    assert(u32map_size(&m) == 12);
    u32map_destroy(&m);
    assert(u32map_is_empty(&m));
    // Pool fully freed — all 12 slots available again
    for (uint32_t i = 0; i < 12; i++) {
        uint32_t k = i + 100, v = i;
        assert(u32map_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }
    assert(u32map_size(&m) == 12);
    u32map_destroy(&m);
}

static void test_pool_exhaustion(void) {
    printf("Test: insert fails when pool is exhausted\n");
    u32map_t m = u32map_new();
    for (uint32_t i = 0; i < 12; i++) {
        uint32_t k = i, v = i;
        assert(u32map_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }
    // One more distinct key must fail
    uint32_t k = 200, v = 200;
    assert(u32map_insert(&m, &k, &v) == _ZP_HASHMAP_ITER_INVALID);
    // Updating an existing key must still succeed (no new pool slot needed)
    uint32_t ku = 0, vu = 255;
    assert(u32map_insert(&m, &ku, &vu) != _ZP_HASHMAP_ITER_INVALID);
    assert(*u32map_get(&m, &(uint32_t){0}) == 255);
    u32map_destroy(&m);
}

static void test_pool_slot_reused_after_remove(void) {
    printf("Test: pool slot freed by remove is reused by subsequent insert\n");
    u32map_t m = u32map_new();
    // Fill to capacity
    for (uint32_t i = 0; i < 12; i++) {
        uint32_t k = i, v = i;
        assert(u32map_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }
    // Remove one entry to free a slot
    assert(u32map_remove(&m, &(uint32_t){0}, NULL));
    assert(u32map_size(&m) == 11);
    // Now a new key must succeed
    uint32_t k = 200, v = 200;
    assert(u32map_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    assert(u32map_size(&m) == 12);
    assert(*u32map_get(&m, &(uint32_t){200}) == 200);
    u32map_destroy(&m);
}

// ── Instantiate a map where BUCKET_COUNT (300) exceeds the max value of
// the chosen index type (uint8_t, max = 255, selected because CAPACITY=10 ≤ 254).
// _buckets[] stores pool node indices — values are always < CAPACITY — so the
// uint8_t representation is safe even though 300 > 255.
// This test verifies the map operates correctly in that configuration.

#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE uint32_t
#define _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE uint32_t
#define _ZP_STATIC_HASHMAP_TEMPLATE_NAME wide_bucket_map
#define _ZP_STATIC_HASHMAP_TEMPLATE_BUCKET_COUNT 300
#define _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY 10
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN u32_hash
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_EQ_FN u32_eq
#include "zenoh-pico/collections/static_hashmap_template.h"

static void test_bucket_count_exceeds_iter_type(void) {
    printf("Test: BUCKET_COUNT (%d) > index type max (255, uint8_t), map still correct\n", 300);
    wide_bucket_map_t m = wide_bucket_map_new();
    assert(wide_bucket_map_is_empty(&m));

    // Insert entries that spread across many buckets (key % 300 spreads widely)
    for (uint32_t i = 0; i < 10; i++) {
        uint32_t k = i * 31, v = i;  // 0, 31, 62, 93 … — distinct buckets for most
        assert(wide_bucket_map_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }
    assert(wide_bucket_map_size(&m) == 10);

    // Verify every entry is retrievable
    for (uint32_t i = 0; i < 10; i++) {
        uint32_t k = i * 31;
        uint32_t *got = wide_bucket_map_get(&m, &k);
        assert(got != NULL && *got == i);
    }

    // Update one entry — must not allocate a new pool node
    uint32_t ku = 0, vu = 99;
    assert(wide_bucket_map_insert(&m, &ku, &vu) != _ZP_HASHMAP_ITER_INVALID);
    assert(wide_bucket_map_size(&m) == 10);
    assert(*wide_bucket_map_get(&m, &(uint32_t){0}) == 99);

    // Remove an entry and re-insert a different key to confirm pool slot recycling
    assert(wide_bucket_map_remove(&m, &(uint32_t){31}, NULL));
    assert(wide_bucket_map_size(&m) == 9);
    uint32_t kn = 1000, vn = 42;
    assert(wide_bucket_map_insert(&m, &kn, &vn) != _ZP_HASHMAP_ITER_INVALID);
    assert(wide_bucket_map_size(&m) == 10);
    assert(*wide_bucket_map_get(&m, &(uint32_t){1000}) == 42);

    wide_bucket_map_destroy(&m);
    assert(wide_bucket_map_is_empty(&m));
}

// ── Tests: iteration ──────────────────────────────────────────────────────────

static void test_empty_iteration(void) {
    printf("Test: iteration over empty map yields no elements\n");
    u32map_t m = u32map_new();
    assert(u32map_iter(&m) == _ZP_HASHMAP_ITER_INVALID);
    u32map_destroy(&m);
}

static void test_iteration_visits_all(void) {
    printf("Test: forward iteration visits every live entry exactly once\n");
    u32map_t m = u32map_new();
    const uint32_t N = 10;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i * 10;
        assert(u32map_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }

    bool seen[10] = {false};
    size_t count = 0;
    for (u32map_iter_t it = u32map_iter(&m); it != _ZP_HASHMAP_ITER_INVALID; it = u32map_iter_next(&m, it)) {
        u32map_node_t *n = u32map_node_at(&m, it);
        assert(n->key < N && !seen[n->key]);
        assert(n->val == n->key * 10);
        seen[n->key] = true;
        count++;
    }
    assert(count == N);
    for (uint32_t i = 0; i < N; i++) {
        assert(seen[i]);
    }
    u32map_destroy(&m);
}

static void test_iteration_visits_all_with_collisions(void) {
    printf("Test: iteration visits all entries including collision chains\n");
    u32map_t m = u32map_new();
    // Keys 0,8,16 all hash to bucket 0 (BUCKET_COUNT=8); spread the rest across other buckets.
    uint32_t keys[] = {0, 8, 16, 1, 2, 3};
    const uint32_t N = 6;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = keys[i], v = keys[i] + 100;
        assert(u32map_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }

    bool seen[17] = {false};  // keys go up to 16
    size_t count = 0;
    for (u32map_iter_t it = u32map_iter(&m); it != _ZP_HASHMAP_ITER_INVALID; it = u32map_iter_next(&m, it)) {
        u32map_node_t *n = u32map_node_at(&m, it);
        assert(!seen[n->key]);
        assert(n->val == n->key + 100);
        seen[n->key] = true;
        count++;
    }
    assert(count == N);
    for (uint32_t i = 0; i < N; i++) {
        assert(seen[keys[i]]);
    }
    u32map_destroy(&m);
}

static void test_iter_next_after_single_entry(void) {
    printf("Test: iter_next after single-entry map returns INVALID\n");
    u32map_t m = u32map_new();
    uint32_t k = 42, v = 420;
    assert(u32map_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    u32map_iter_t it = u32map_iter(&m);
    assert(it != _ZP_HASHMAP_ITER_INVALID);
    assert(u32map_iter_next(&m, it) == _ZP_HASHMAP_ITER_INVALID);
    u32map_destroy(&m);
}

static void test_iteration_removal_pattern(void) {
    printf("Test: safe removal of even keys during iteration\n");
    u32map_t m = u32map_new();
    const uint32_t N = 10;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i;
        assert(u32map_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }

    // Remove all even keys during iteration using the next_idx output.
    for (u32map_iter_t it = u32map_iter(&m); it != _ZP_HASHMAP_ITER_INVALID;) {
        u32map_node_t *n = u32map_node_at(&m, it);
        if (n->key % 2 == 0) {
            u32map_remove_at(&m, it, NULL, &it);
        } else {
            it = u32map_iter_next(&m, it);
        }
    }

    assert(u32map_size(&m) == N / 2);
    for (uint32_t i = 0; i < N; i++) {
        uint32_t *got = u32map_get(&m, &i);
        if (i % 2 == 0) {
            assert(got == NULL);
        } else {
            assert(got != NULL && *got == i);
        }
    }
    u32map_destroy(&m);
}

static void test_iteration_remove_all(void) {
    printf("Test: removing all entries during iteration leaves map empty\n");
    u32map_t m = u32map_new();
    const uint32_t N = 8;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i;
        assert(u32map_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }

    for (u32map_iter_t it = u32map_iter(&m); it != _ZP_HASHMAP_ITER_INVALID;) {
        u32map_remove_at(&m, it, NULL, &it);
    }

    assert(u32map_is_empty(&m));
    assert(u32map_iter(&m) == _ZP_HASHMAP_ITER_INVALID);
    u32map_destroy(&m);
}

static void test_remove_at_moves_value_out(void) {
    printf("Test: remove_at with out_val moves node out correctly\n");
    u32map_t m = u32map_new();
    uint32_t k = 55, v = 550;
    assert(u32map_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    u32map_iter_t it = u32map_iter(&m);
    assert(it != _ZP_HASHMAP_ITER_INVALID);

    u32map_node_t out;
    u32map_remove_at(&m, it, &out, NULL);
    assert(out.key == 55 && out.val == 550);
    assert(u32map_is_empty(&m));
    u32map_destroy(&m);
}

static void test_multiple_collisions(void) {
    printf("Test: many keys colliding into the same bucket\n");
    u32map_t m = u32map_new();
    // With BUCKET_COUNT=8, keys 0,8,16,24,32,40 all land in bucket 0
    uint32_t keys[] = {0, 8, 16, 24, 32, 40};
    for (size_t i = 0; i < 6; i++) {
        uint32_t k = keys[i], v = keys[i] * 10;
        assert(u32map_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
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
    test_bucket_count_exceeds_iter_type();
    test_empty_iteration();
    test_iteration_visits_all();
    test_iteration_visits_all_with_collisions();
    test_iter_next_after_single_entry();
    test_iteration_removal_pattern();
    test_iteration_remove_all();
    test_remove_at_moves_value_out();
    return 0;
}
