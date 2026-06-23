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
#include <stdlib.h>
#include <string.h>

#undef NDEBUG
#include <assert.h>

#include "zenoh-pico/collections/algorithms_template.h"

// ── Instantiate uint32_t -> uint32_t, 12 buckets, capacity 12 ────────────────

// Identity hash: bucket = key % capacity. Using the identity makes collisions
// fully predictable, so the collision-chain tests below can deliberately place
// several keys into the same bucket.
static inline size_t u32_hash(const uint32_t *k) { return (size_t)(*k); }
static inline bool u32_eq(const uint32_t *a, const uint32_t *b) { return *a == *b; }

#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE uint32_t
#define _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE uint32_t
#define _ZP_STATIC_HASHMAP_TEMPLATE_NAME u32map
#define _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY 12
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN u32_hash
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_EQ_FN u32_eq
#include "zenoh-pico/collections/static_hashmap_template.h"

// ── Instantiate int -> owned_str_t to exercise the value destroy callback ────

static inline size_t int_hash(const int *k) { return (size_t)(*k); }
static inline bool int_eq(const int *a, const int *b) { return *a == *b; }

// Wrap the owned pointer in a struct so the value type is not itself a bare
// pointer (keeps const-correctness clean for const_get/const_at accessors).
typedef struct {
    char *str;
} owned_str_t;

#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE int
#define _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE owned_str_t
#define _ZP_STATIC_HASHMAP_TEMPLATE_NAME strmap
#define _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY 8
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN int_hash
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_EQ_FN int_eq
#define _ZP_STATIC_HASHMAP_TEMPLATE_VAL_DESTROY_FN(p) free((p)->str)
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
    assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
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
    assert(u32map_insert(&m, &k, &v1) != u32map_end(&m));
    assert(u32map_insert(&m, &(uint32_t){5}, &v2) != u32map_end(&m));
    assert(u32map_size(&m) == 1);
    assert(*u32map_get(&m, &(uint32_t){5}) == 99);
    u32map_destroy(&m);
}

static void test_insert_null_value(void) {
    printf("Test: insert with NULL value leaves entry uninitialized for manual init\n");
    strmap_t m = strmap_new();

    // New key with NULL value: the entry is created but its value is left
    // uninitialized. Initialize it manually through the returned iterator.
    int k = 1;
    strmap_iter_t it = strmap_insert(&m, &k, NULL);
    assert(it != strmap_end(&m));
    assert(strmap_size(&m) == 1);
    strmap_elem_t *n = strmap_at(&m, it);
    n->val.str = (char *)malloc(16);
    snprintf(n->val.str, 16, "init");
    assert(strcmp(strmap_get(&m, &(int){1})->str, "init") == 0);

    // Update an existing key with NULL: the old value is destroyed (freed) and
    // the slot is left uninitialized (regression: this used to dereference the
    // NULL value pointer). Reinitialize via _at before teardown.
    int k_dup = 1;
    it = strmap_insert(&m, &k_dup, NULL);
    assert(it != strmap_end(&m));
    assert(strmap_size(&m) == 1);
    n = strmap_at(&m, it);
    n->val.str = (char *)malloc(16);
    snprintf(n->val.str, 16, "again");
    assert(strcmp(strmap_get(&m, &(int){1})->str, "again") == 0);

    // destroy frees the reinitialized value (run under valgrind to confirm).
    strmap_destroy(&m);
    assert(strmap_is_empty(&m));
}

static void test_remove_existing(void) {
    printf("Test: remove existing key moves value out\n");
    u32map_t m = u32map_new();
    uint32_t k = 7, v = 70;
    assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
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
    // bucket_count == 12, so 0 and 12 hash to the same bucket.
    uint32_t k0 = 0, v0 = 100;
    uint32_t k12 = 12, v12 = 800;
    assert(u32map_insert(&m, &k0, &v0) != u32map_end(&m));
    assert(u32map_insert(&m, &k12, &v12) != u32map_end(&m));
    assert(u32map_size(&m) == 2);
    assert(u32map_remove(&m, &(uint32_t){12}, NULL));
    assert(u32map_size(&m) == 1);
    assert(*u32map_get(&m, &(uint32_t){0}) == 100);
    assert(u32map_get(&m, &(uint32_t){12}) == NULL);
    u32map_destroy(&m);
}

static void test_remove_tail_of_chain(void) {
    printf("Test: remove tail of a collision chain; head still accessible\n");
    u32map_t m = u32map_new();
    // bucket_count == 12, so 0 and 12 hash to the same bucket.
    uint32_t k0 = 0, v0 = 100;
    uint32_t k12 = 12, v12 = 800;
    assert(u32map_insert(&m, &k0, &v0) != u32map_end(&m));
    assert(u32map_insert(&m, &k12, &v12) != u32map_end(&m));
    assert(u32map_remove(&m, &(uint32_t){0}, NULL));
    assert(u32map_size(&m) == 1);
    assert(*u32map_get(&m, &(uint32_t){12}) == 800);
    assert(u32map_get(&m, &(uint32_t){0}) == NULL);
    u32map_destroy(&m);
}

static void test_contains(void) {
    printf("Test: contains\n");
    u32map_t m = u32map_new();
    assert(!u32map_contains(&m, &(uint32_t){3}));
    uint32_t k = 3, v = 30;
    assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
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
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    assert(u32map_size(&m) == 12);
    u32map_destroy(&m);
    assert(u32map_is_empty(&m));
    for (uint32_t i = 0; i < 12; i++) {
        uint32_t k = i + 100, v = i;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    assert(u32map_size(&m) == 12);
    u32map_destroy(&m);
}

static void test_pool_exhaustion(void) {
    printf("Test: insert fails when pool is exhausted\n");
    u32map_t m = u32map_new();
    for (uint32_t i = 0; i < 12; i++) {
        uint32_t k = i, v = i;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    uint32_t k = 200, v = 200;
    assert(u32map_insert(&m, &k, &v) == u32map_end(&m));
    uint32_t ku = 0, vu = 255;
    assert(u32map_insert(&m, &ku, &vu) != u32map_end(&m));
    assert(*u32map_get(&m, &(uint32_t){0}) == 255);
    u32map_destroy(&m);
}

static void test_pool_slot_reused_after_remove(void) {
    printf("Test: pool slot freed by remove is reused by subsequent insert\n");
    u32map_t m = u32map_new();
    for (uint32_t i = 0; i < 12; i++) {
        uint32_t k = i, v = i;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    assert(u32map_remove(&m, &(uint32_t){0}, NULL));
    assert(u32map_size(&m) == 11);
    uint32_t k = 200, v = 200;
    assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    assert(u32map_size(&m) == 12);
    assert(*u32map_get(&m, &(uint32_t){200}) == 200);
    u32map_destroy(&m);
}

#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE uint32_t
#define _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE uint32_t
#define _ZP_STATIC_HASHMAP_TEMPLATE_NAME wide_bucket_map
#define _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY 10
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN u32_hash
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_EQ_FN u32_eq
#include "zenoh-pico/collections/static_hashmap_template.h"

static void test_bucket_count_exceeds_iter_type(void) {
    printf("Test: entries spread across many buckets, map still correct\n");
    wide_bucket_map_t m = wide_bucket_map_new();
    assert(wide_bucket_map_is_empty(&m));
    for (uint32_t i = 0; i < 10; i++) {
        uint32_t k = i * 31, v = i;
        assert(wide_bucket_map_insert(&m, &k, &v) != wide_bucket_map_end(&m));
    }
    assert(wide_bucket_map_size(&m) == 10);
    for (uint32_t i = 0; i < 10; i++) {
        uint32_t k = i * 31;
        uint32_t *got = wide_bucket_map_get(&m, &k);
        assert(got != NULL && *got == i);
    }
    uint32_t ku = 0, vu = 99;
    assert(wide_bucket_map_insert(&m, &ku, &vu) != wide_bucket_map_end(&m));
    assert(wide_bucket_map_size(&m) == 10);
    assert(*wide_bucket_map_get(&m, &(uint32_t){0}) == 99);
    assert(wide_bucket_map_remove(&m, &(uint32_t){31}, NULL));
    assert(wide_bucket_map_size(&m) == 9);
    uint32_t kn = 1000, vn = 42;
    assert(wide_bucket_map_insert(&m, &kn, &vn) != wide_bucket_map_end(&m));
    assert(wide_bucket_map_size(&m) == 10);
    assert(*wide_bucket_map_get(&m, &(uint32_t){1000}) == 42);
    wide_bucket_map_destroy(&m);
    assert(wide_bucket_map_is_empty(&m));
}

// ── Tests: iteration ──────────────────────────────────────────────────────────

static void test_empty_iteration(void) {
    printf("Test: iteration over empty map yields no elements\n");
    u32map_t m = u32map_new();
    assert(u32map_begin(&m) == u32map_end(&m));
    u32map_destroy(&m);
}

static void test_iteration_visits_all(void) {
    printf("Test: forward iteration visits every live entry exactly once\n");
    u32map_t m = u32map_new();
    const uint32_t N = 10;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i * 10;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    bool seen[10] = {false};
    size_t count = 0;
    for (u32map_iter_t it = u32map_begin(&m); it != u32map_end(&m); it = u32map_iter_next(&m, it)) {
        u32map_elem_t *n = u32map_at(&m, it);
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
    // bucket_count == 12: {0,12}, {1,13} and {2,14} each share a bucket,
    // forming three collision chains of length two.
    uint32_t keys[] = {0, 12, 1, 13, 2, 14};
    const uint32_t N = 6;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = keys[i], v = keys[i] + 100;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    bool seen[15] = {false};
    size_t count = 0;
    for (u32map_iter_t it = u32map_begin(&m); it != u32map_end(&m); it = u32map_iter_next(&m, it)) {
        u32map_elem_t *n = u32map_at(&m, it);
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
    printf("Test: iter_next after single-entry map returns end\n");
    u32map_t m = u32map_new();
    uint32_t k = 42, v = 420;
    assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    u32map_iter_t it = u32map_begin(&m);
    assert(it != u32map_end(&m));
    assert(u32map_iter_next(&m, it) == u32map_end(&m));
    u32map_destroy(&m);
}

static void test_iteration_removal_pattern(void) {
    printf("Test: safe removal of even keys during iteration\n");
    u32map_t m = u32map_new();
    const uint32_t N = 10;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    for (u32map_iter_t it = u32map_begin(&m); it != u32map_end(&m);) {
        u32map_elem_t *n = u32map_at(&m, it);
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
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    for (u32map_iter_t it = u32map_begin(&m); it != u32map_end(&m);) {
        u32map_remove_at(&m, it, NULL, &it);
    }
    assert(u32map_is_empty(&m));
    assert(u32map_begin(&m) == u32map_end(&m));
    u32map_destroy(&m);
}

static void test_remove_at_moves_value_out(void) {
    printf("Test: remove_at with out_val moves node out correctly\n");
    u32map_t m = u32map_new();
    uint32_t k = 55, v = 550;
    assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    u32map_iter_t it = u32map_begin(&m);
    assert(it != u32map_end(&m));
    u32map_elem_t out;
    u32map_remove_at(&m, it, &out, NULL);
    assert(out.key == 55 && out.val == 550);
    assert(u32map_is_empty(&m));
    u32map_destroy(&m);
}

static void test_multiple_collisions(void) {
    printf("Test: many keys colliding into the same bucket\n");
    u32map_t m = u32map_new();
    // bucket_count == 12: every multiple of 12 hashes to bucket 0.
    uint32_t keys[] = {0, 12, 24, 36, 48, 60};
    for (size_t i = 0; i < 6; i++) {
        uint32_t k = keys[i], v = keys[i] * 10;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    assert(u32map_size(&m) == 6);
    for (size_t i = 0; i < 6; i++) {
        uint32_t *got = u32map_get(&m, &keys[i]);
        assert(got != NULL && *got == keys[i] * 10);
    }
    assert(u32map_remove(&m, &(uint32_t){24}, NULL));
    assert(u32map_remove(&m, &(uint32_t){48}, NULL));
    assert(u32map_size(&m) == 4);
    assert(u32map_get(&m, &(uint32_t){24}) == NULL);
    assert(u32map_get(&m, &(uint32_t){48}) == NULL);
    assert(*u32map_get(&m, &(uint32_t){0}) == 0);
    assert(*u32map_get(&m, &(uint32_t){12}) == 120);
    assert(*u32map_get(&m, &(uint32_t){36}) == 360);
    assert(*u32map_get(&m, &(uint32_t){60}) == 600);
    u32map_destroy(&m);
}

// ── Tests: algorithms_template.h macros ───────────────────────────────────────

static void test_algorithms_foreach(void) {
    printf("Test: _ZP_FOREACH visits every entry exactly once\n");
    u32map_t m = u32map_new();
    const uint32_t N = 8;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i * 5;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    bool seen[8] = {false};
    size_t count = 0;
    u32map_elem_t *node = NULL;
    _ZP_FOREACH (u32map, &m, node) {
        assert(node->key < N && !seen[node->key]);
        assert(node->val == node->key * 5);
        seen[node->key] = true;
        count++;
    }
    assert(count == N);
    u32map_destroy(&m);
}

static void test_algorithms_cforeach(void) {
    printf("Test: _ZP_CONST_FOREACH visits every entry via const pointer\n");
    u32map_t m = u32map_new();
    const uint32_t N = 6;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i + 100;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    bool seen[6] = {false};
    const u32map_elem_t *node = NULL;
    _ZP_CONST_FOREACH (u32map, &m, node) {
        assert(node->key < N && !seen[node->key]);
        assert(node->val == node->key + 100);
        seen[node->key] = true;
    }
    for (uint32_t i = 0; i < N; i++) {
        assert(seen[i]);
    }
    u32map_destroy(&m);
}

static void test_algorithms_foreach_val(void) {
    printf("Test: _ZP_FOREACH_VAL iterates values of entries\n");
    u32map_t m = u32map_new();
    uint32_t sum = 0;
    for (uint32_t i = 1; i <= 5; i++) {
        uint32_t k = i, v = i;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
        sum += i;
    }
    uint32_t *val = NULL;
    uint32_t got_sum = 0;
    _ZP_FOREACH_VAL (u32map, &m, val) {
        got_sum += *val;
    }
    assert(got_sum == sum);
    u32map_destroy(&m);
}

static void test_algorithms_find(void) {
    printf("Test: _ZP_CONST_FIND locates first matching entry, returns NULL when absent\n");
    u32map_t m = u32map_new();
    for (uint32_t i = 0; i < 10; i++) {
        uint32_t k = i, v = i * 3;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    const u32map_elem_t *found = NULL;
    _ZP_CONST_FIND(u32map, &m, found, _->val == 15);  // key 5 -> val 15
    assert(found != NULL && found->key == 5 && found->val == 15);
    _ZP_CONST_FIND(u32map, &m, found, _->val == 999);
    assert(found == NULL);
    u32map_destroy(&m);
}

static void test_algorithms_find_val(void) {
    printf("Test: _ZP_CONST_FIND_VAL locates first matching value\n");
    u32map_t m = u32map_new();
    for (uint32_t i = 0; i < 8; i++) {
        uint32_t k = i, v = i * 2;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    const uint32_t *found_val = NULL;
    _ZP_CONST_FIND_VAL(u32map, &m, found_val, *_ == 10);  // key 5 -> val 10
    assert(found_val != NULL && *found_val == 10);
    _ZP_CONST_FIND_VAL(u32map, &m, found_val, *_ == 99);
    assert(found_val == NULL);
    u32map_destroy(&m);
}

static void test_algorithms_itfind(void) {
    printf("Test: _ZP_IT_FIND positions iterator on first matching entry\n");
    u32map_t m = u32map_new();
    const uint32_t N = 10;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i * 5;  // unique keys and values
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }

    // Found over the full range.
    u32map_iter_t it = u32map_begin(&m);
    u32map_iter_t end_it = u32map_end(&m);
    _ZP_IT_FIND(u32map, &m, it, end_it, _->key == 6);
    assert(it != end_it);
    u32map_iter_t pos = it;
    assert(u32map_at(&m, it)->key == 6 && u32map_at(&m, it)->val == 30);

    // begin bound: searching strictly after the match finds nothing (unique key).
    it = u32map_iter_next(&m, pos);
    end_it = u32map_end(&m);
    _ZP_IT_FIND(u32map, &m, it, end_it, _->key == 6);
    assert(it == end_it);

    // end bound: excluding the matching slot finds nothing.
    it = u32map_begin(&m);
    end_it = pos;
    _ZP_IT_FIND(u32map, &m, it, end_it, _->key == 6);
    assert(it == pos);

    // No match advances the iterator to end.
    it = u32map_begin(&m);
    end_it = u32map_end(&m);
    _ZP_IT_FIND(u32map, &m, it, end_it, _->key == 999);
    assert(it == end_it);

    // Empty range finds nothing.
    it = u32map_end(&m);
    end_it = u32map_end(&m);
    _ZP_IT_FIND(u32map, &m, it, end_it, true);
    assert(it == end_it);

    u32map_destroy(&m);
}

static void test_algorithms_citfind(void) {
    printf("Test: _ZP_CONST_IT_FIND positions iterator on first matching entry (const)\n");
    u32map_t m = u32map_new();
    const uint32_t N = 9;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i + 100;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }

    // Found over the full range; node accessed via const_at().
    u32map_iter_t it = u32map_begin(&m);
    u32map_iter_t end_it = u32map_end(&m);
    _ZP_CONST_IT_FIND(u32map, &m, it, end_it, _->key == 4);
    assert(it != end_it);
    u32map_iter_t pos = it;
    assert(u32map_const_at(&m, it)->key == 4 && u32map_const_at(&m, it)->val == 104);

    // begin bound: searching strictly after the match finds nothing.
    it = u32map_iter_next(&m, pos);
    end_it = u32map_end(&m);
    _ZP_CONST_IT_FIND(u32map, &m, it, end_it, _->key == 4);
    assert(it == end_it);

    // end bound: excluding the matching slot finds nothing.
    it = u32map_begin(&m);
    end_it = pos;
    _ZP_CONST_IT_FIND(u32map, &m, it, end_it, _->key == 4);
    assert(it == pos);

    // No match advances to end.
    it = u32map_begin(&m);
    end_it = u32map_end(&m);
    _ZP_CONST_IT_FIND(u32map, &m, it, end_it, _->key == 999);
    assert(it == end_it);

    u32map_destroy(&m);
}

static void test_algorithms_itfind_val(void) {
    printf("Test: _ZP_IT_FIND_VAL positions iterator on first matching value\n");
    u32map_t m = u32map_new();
    const uint32_t N = 9;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i * 2;  // unique values
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }

    // Found by value.
    u32map_iter_t it = u32map_begin(&m);
    u32map_iter_t end_it = u32map_end(&m);
    _ZP_IT_FIND(u32map, &m, it, end_it, _->val == 10);  // key 5 -> val 10
    assert(it != end_it);
    u32map_iter_t pos = it;
    assert(u32map_at(&m, it)->key == 5 && u32map_at(&m, it)->val == 10);

    // begin bound: searching strictly after the match finds nothing.
    it = u32map_iter_next(&m, pos);
    end_it = u32map_end(&m);
    _ZP_IT_FIND(u32map, &m, it, end_it, _->val == 10);
    assert(it == end_it);

    // end bound: excluding the matching slot finds nothing.
    it = u32map_begin(&m);
    end_it = pos;
    _ZP_IT_FIND(u32map, &m, it, end_it, _->val == 10);
    assert(it == pos);

    // No match advances to end.
    it = u32map_begin(&m);
    end_it = u32map_end(&m);
    _ZP_IT_FIND(u32map, &m, it, end_it, _->val == 9999);
    assert(it == end_it);

    u32map_destroy(&m);
}

static void test_algorithms_citfind_val(void) {
    printf("Test: _ZP_CONST_IT_FIND_VAL positions iterator on first matching value (const)\n");
    u32map_t m = u32map_new();
    const uint32_t N = 8;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i * 3;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }

    // Found by value via const_at().
    u32map_iter_t it = u32map_begin(&m);
    u32map_iter_t end_it = u32map_end(&m);
    _ZP_CONST_IT_FIND(u32map, &m, it, end_it, _->val == 12);  // key 4 -> val 12
    assert(it != end_it);
    u32map_iter_t pos = it;
    assert(u32map_const_at(&m, it)->key == 4 && u32map_const_at(&m, it)->val == 12);

    // begin bound: searching strictly after the match finds nothing.
    it = u32map_iter_next(&m, pos);
    end_it = u32map_end(&m);
    _ZP_CONST_IT_FIND(u32map, &m, it, end_it, _->val == 12);
    assert(it == end_it);

    // end bound: excluding the matching slot finds nothing.
    it = u32map_begin(&m);
    end_it = pos;
    _ZP_CONST_IT_FIND(u32map, &m, it, end_it, _->val == 12);
    assert(it == pos);

    // No match advances to end.
    it = u32map_begin(&m);
    end_it = u32map_end(&m);
    _ZP_CONST_IT_FIND(u32map, &m, it, end_it, _->val == 9999);
    assert(it == end_it);

    u32map_destroy(&m);
}

static void test_algorithms_remove(void) {
    printf("Test: _ZP_REMOVE removes all matching entries\n");
    u32map_t m = u32map_new();
    const uint32_t N = 12;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    _ZP_REMOVE(u32map, &m, _->key % 2 != 0);

    assert(u32map_size(&m) == N / 2);
    for (uint32_t i = 0; i < N; i++) {
        uint32_t *got = u32map_get(&m, &i);
        if (i % 2 != 0) {
            assert(got == NULL);
        } else {
            assert(got != NULL && *got == i);
        }
    }
    u32map_destroy(&m);
}

static void test_algorithms_remove_all(void) {
    printf("Test: _ZP_REMOVE with always-true predicate empties the map\n");
    u32map_t m = u32map_new();
    for (uint32_t i = 0; i < 8; i++) {
        uint32_t k = i, v = i;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    _ZP_REMOVE(u32map, &m, true);

    assert(u32map_is_empty(&m));
    assert(u32map_begin(&m) == u32map_end(&m));
    u32map_destroy(&m);
}

int main(void) {
    test_new_is_empty();
    test_insert_and_get();
    test_get_missing_returns_null();
    test_insert_updates_existing();
    test_insert_null_value();
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
    test_algorithms_foreach();
    test_algorithms_cforeach();
    test_algorithms_foreach_val();
    test_algorithms_find();
    test_algorithms_find_val();
    test_algorithms_itfind();
    test_algorithms_citfind();
    test_algorithms_itfind_val();
    test_algorithms_citfind_val();
    test_algorithms_remove();
    test_algorithms_remove_all();
    printf("All static_hashmap tests passed\n");
    return 0;
}
