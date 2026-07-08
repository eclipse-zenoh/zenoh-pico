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

// ── Instantiate a uint32_t set, 12 buckets, capacity 12 ──────────────────────
//
// Identity hash: bucket = key % capacity. Using the identity makes collisions
// fully predictable, so the collision-chain tests below can deliberately place
// several keys into the same bucket.
static inline size_t u32_hash(const uint32_t *k) { return (size_t)(*k); }
static inline bool u32_eq(const uint32_t *a, const uint32_t *b) { return *a == *b; }

#define _ZP_STATIC_HASHSET_TEMPLATE_KEY_TYPE uint32_t
#define _ZP_STATIC_HASHSET_TEMPLATE_NAME u32set
#define _ZP_STATIC_HASHSET_TEMPLATE_CAPACITY 12
#define _ZP_STATIC_HASHSET_TEMPLATE_KEY_HASH_FN u32_hash
#define _ZP_STATIC_HASHSET_TEMPLATE_KEY_EQ_FN u32_eq
#include "zenoh-pico/collections/static_hashset_template.h"

// ── Instantiate a set of owned_str_t to exercise the key destroy callback ────

// A key that owns a heap buffer, so we can verify that insert destroys the
// incoming duplicate key and that destroy/remove release the stored key.
typedef struct {
    char *str;
} owned_str_t;

static inline owned_str_t owned_str_make(const char *s) {
    owned_str_t o;
    o.str = malloc(strlen(s) + 1);
    assert(o.str != NULL);
    memcpy(o.str, s, strlen(s) + 1);
    return o;
}
static inline size_t owned_str_hash(const owned_str_t *k) {
    size_t h = 5381;
    for (const char *p = k->str; *p != '\0'; p++) {
        h = (h << 5) + (h ^ (size_t)(unsigned char)*p);
    }
    return h;
}
static inline bool owned_str_eq(const owned_str_t *a, const owned_str_t *b) { return strcmp(a->str, b->str) == 0; }

#define _ZP_STATIC_HASHSET_TEMPLATE_KEY_TYPE owned_str_t
#define _ZP_STATIC_HASHSET_TEMPLATE_NAME strset
#define _ZP_STATIC_HASHSET_TEMPLATE_CAPACITY 8
#define _ZP_STATIC_HASHSET_TEMPLATE_KEY_HASH_FN owned_str_hash
#define _ZP_STATIC_HASHSET_TEMPLATE_KEY_EQ_FN owned_str_eq
#define _ZP_STATIC_HASHSET_TEMPLATE_KEY_DESTROY_FN(p) free((p)->str)
#include "zenoh-pico/collections/static_hashset_template.h"

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_new_is_empty(void) {
    printf("Test: new set is empty\n");
    u32set_t s = u32set_new();
    assert(u32set_is_empty(&s));
    assert(u32set_size(&s) == 0);
    u32set_destroy(&s);
}

static void test_insert_and_contains(void) {
    printf("Test: insert then contains reports membership\n");
    u32set_t s = u32set_new();
    assert(!u32set_contains(&s, &(uint32_t){1}));
    uint32_t k = 1;
    assert(u32set_insert(&s, &k) != u32set_end(&s));
    assert(u32set_size(&s) == 1);
    assert(u32set_contains(&s, &(uint32_t){1}));
    u32set_destroy(&s);
}

static void test_node_is_key(void) {
    printf("Test: at() on a set returns a pointer to the key itself\n");
    u32set_t s = u32set_new();
    uint32_t k = 77;
    u32set_iter_t it = u32set_insert(&s, &k);
    assert(it != u32set_end(&s));
    // In set mode the element type is the key type; at() yields a key pointer.
    u32set_elem_t *elem = u32set_at(&s, it);
    assert(*elem == 77);
    const u32set_elem_t *celem = u32set_const_at(&s, it);
    assert(*celem == 77);
    u32set_destroy(&s);
}

static void test_get_iter(void) {
    printf("Test: get_iter locates the entry, invalid iterator when absent\n");
    u32set_t s = u32set_new();
    uint32_t k = 5;
    u32set_iter_t inserted = u32set_insert(&s, &k);
    assert(inserted != u32set_end(&s));
    u32set_iter_t found = u32set_get_iter(&s, &(uint32_t){5});
    assert(found == inserted);
    assert(u32set_get_iter(&s, &(uint32_t){6}) == u32set_end(&s));
    u32set_destroy(&s);
}

static void test_insert_duplicate_is_noop(void) {
    printf("Test: inserting a duplicate key leaves size unchanged\n");
    u32set_t s = u32set_new();
    uint32_t k = 5;
    u32set_iter_t first = u32set_insert(&s, &k);
    assert(first != u32set_end(&s));
    // Re-insert the same key: the entry stays, size does not grow, and the
    // returned iterator points at the existing slot.
    u32set_iter_t again = u32set_insert(&s, &(uint32_t){5});
    assert(again == first);
    assert(u32set_size(&s) == 1);
    u32set_destroy(&s);
}

static void test_remove_existing(void) {
    printf("Test: remove existing key returns true and drops the entry\n");
    u32set_t s = u32set_new();
    uint32_t k = 7;
    assert(u32set_insert(&s, &k) != u32set_end(&s));
    assert(u32set_remove(&s, &(uint32_t){7}));
    assert(u32set_size(&s) == 0);
    assert(!u32set_contains(&s, &(uint32_t){7}));
    u32set_destroy(&s);
}

static void test_remove_missing_returns_false(void) {
    printf("Test: remove on missing key returns false\n");
    u32set_t s = u32set_new();
    assert(!u32set_remove(&s, &(uint32_t){99}));
    u32set_destroy(&s);
}

static void test_remove_head_of_chain(void) {
    printf("Test: remove head of a collision chain; remaining entry still present\n");
    u32set_t s = u32set_new();
    // bucket_count == 12, so 0 and 12 hash to the same bucket.
    uint32_t k0 = 0, k12 = 12;
    assert(u32set_insert(&s, &k0) != u32set_end(&s));
    assert(u32set_insert(&s, &k12) != u32set_end(&s));
    assert(u32set_size(&s) == 2);
    assert(u32set_remove(&s, &(uint32_t){12}));
    assert(u32set_size(&s) == 1);
    assert(u32set_contains(&s, &(uint32_t){0}));
    assert(!u32set_contains(&s, &(uint32_t){12}));
    u32set_destroy(&s);
}

static void test_remove_tail_of_chain(void) {
    printf("Test: remove tail of a collision chain; head still present\n");
    u32set_t s = u32set_new();
    uint32_t k0 = 0, k12 = 12;
    assert(u32set_insert(&s, &k0) != u32set_end(&s));
    assert(u32set_insert(&s, &k12) != u32set_end(&s));
    assert(u32set_remove(&s, &(uint32_t){0}));
    assert(u32set_size(&s) == 1);
    assert(u32set_contains(&s, &(uint32_t){12}));
    assert(!u32set_contains(&s, &(uint32_t){0}));
    u32set_destroy(&s);
}

static void test_destroy_and_reuse(void) {
    printf("Test: destroy empties the set and frees the pool for reuse\n");
    u32set_t s = u32set_new();
    for (uint32_t i = 0; i < 12; i++) {
        uint32_t k = i;
        assert(u32set_insert(&s, &k) != u32set_end(&s));
    }
    assert(u32set_size(&s) == 12);
    u32set_destroy(&s);
    assert(u32set_is_empty(&s));
    for (uint32_t i = 0; i < 12; i++) {
        uint32_t k = i + 100;
        assert(u32set_insert(&s, &k) != u32set_end(&s));
    }
    assert(u32set_size(&s) == 12);
    u32set_destroy(&s);
}

static void test_pool_exhaustion(void) {
    printf("Test: insert fails when the pool is exhausted\n");
    u32set_t s = u32set_new();
    for (uint32_t i = 0; i < 12; i++) {
        uint32_t k = i;
        assert(u32set_insert(&s, &k) != u32set_end(&s));
    }
    uint32_t k = 200;
    assert(u32set_insert(&s, &k) == u32set_end(&s));
    // A duplicate of an existing key still succeeds even when the pool is full.
    uint32_t dup = 0;
    assert(u32set_insert(&s, &dup) != u32set_end(&s));
    assert(u32set_size(&s) == 12);
    u32set_destroy(&s);
}

static void test_pool_slot_reused_after_remove(void) {
    printf("Test: pool slot freed by remove is reused by a subsequent insert\n");
    u32set_t s = u32set_new();
    for (uint32_t i = 0; i < 12; i++) {
        uint32_t k = i;
        assert(u32set_insert(&s, &k) != u32set_end(&s));
    }
    assert(u32set_remove(&s, &(uint32_t){0}));
    assert(u32set_size(&s) == 11);
    uint32_t k = 200;
    assert(u32set_insert(&s, &k) != u32set_end(&s));
    assert(u32set_size(&s) == 12);
    assert(u32set_contains(&s, &(uint32_t){200}));
    u32set_destroy(&s);
}

static void test_multiple_collisions(void) {
    printf("Test: many keys colliding into the same bucket\n");
    u32set_t s = u32set_new();
    // bucket_count == 12: every multiple of 12 hashes to bucket 0.
    uint32_t keys[] = {0, 12, 24, 36, 48, 60};
    for (size_t i = 0; i < 6; i++) {
        assert(u32set_insert(&s, &keys[i]) != u32set_end(&s));
    }
    assert(u32set_size(&s) == 6);
    for (size_t i = 0; i < 6; i++) {
        assert(u32set_contains(&s, &keys[i]));
    }
    assert(u32set_remove(&s, &(uint32_t){24}));
    assert(u32set_remove(&s, &(uint32_t){48}));
    assert(u32set_size(&s) == 4);
    assert(!u32set_contains(&s, &(uint32_t){24}));
    assert(!u32set_contains(&s, &(uint32_t){48}));
    assert(u32set_contains(&s, &(uint32_t){0}));
    assert(u32set_contains(&s, &(uint32_t){12}));
    assert(u32set_contains(&s, &(uint32_t){36}));
    assert(u32set_contains(&s, &(uint32_t){60}));
    u32set_destroy(&s);
}

// ── Tests: iteration ──────────────────────────────────────────────────────────

static void test_empty_iteration(void) {
    printf("Test: iteration over empty set yields no elements\n");
    u32set_t s = u32set_new();
    assert(u32set_begin(&s) == u32set_end(&s));
    u32set_destroy(&s);
}

static void test_iteration_visits_all(void) {
    printf("Test: forward iteration visits every key exactly once\n");
    u32set_t s = u32set_new();
    const uint32_t N = 10;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i;
        assert(u32set_insert(&s, &k) != u32set_end(&s));
    }
    bool seen[10] = {false};
    size_t count = 0;
    for (u32set_iter_t it = u32set_begin(&s); it != u32set_end(&s); it = u32set_iter_next(&s, it)) {
        u32set_elem_t *key = u32set_at(&s, it);
        assert(*key < N && !seen[*key]);
        seen[*key] = true;
        count++;
    }
    assert(count == N);
    for (uint32_t i = 0; i < N; i++) {
        assert(seen[i]);
    }
    u32set_destroy(&s);
}

static void test_iteration_visits_all_with_collisions(void) {
    printf("Test: iteration visits all keys including collision chains\n");
    u32set_t s = u32set_new();
    // bucket_count == 12: {0,12}, {1,13} and {2,14} each share a bucket.
    uint32_t keys[] = {0, 12, 1, 13, 2, 14};
    const uint32_t N = 6;
    for (uint32_t i = 0; i < N; i++) {
        assert(u32set_insert(&s, &keys[i]) != u32set_end(&s));
    }
    bool seen[15] = {false};
    size_t count = 0;
    for (u32set_iter_t it = u32set_begin(&s); it != u32set_end(&s); it = u32set_iter_next(&s, it)) {
        u32set_elem_t *key = u32set_at(&s, it);
        assert(!seen[*key]);
        seen[*key] = true;
        count++;
    }
    assert(count == N);
    for (uint32_t i = 0; i < N; i++) {
        assert(seen[keys[i]]);
    }
    u32set_destroy(&s);
}

static void test_iteration_removal_pattern(void) {
    printf("Test: safe removal of even keys during iteration\n");
    u32set_t s = u32set_new();
    const uint32_t N = 10;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i;
        assert(u32set_insert(&s, &k) != u32set_end(&s));
    }
    for (u32set_iter_t it = u32set_begin(&s); it != u32set_end(&s);) {
        u32set_elem_t *key = u32set_at(&s, it);
        if (*key % 2 == 0) {
            u32set_remove_at(&s, it, NULL, &it);
        } else {
            it = u32set_iter_next(&s, it);
        }
    }
    assert(u32set_size(&s) == N / 2);
    for (uint32_t i = 0; i < N; i++) {
        assert(u32set_contains(&s, &i) == (i % 2 != 0));
    }
    u32set_destroy(&s);
}

static void test_remove_at_moves_key_out(void) {
    printf("Test: remove_at with out_node moves the key out correctly\n");
    u32set_t s = u32set_new();
    uint32_t k = 55;
    assert(u32set_insert(&s, &k) != u32set_end(&s));
    u32set_iter_t it = u32set_begin(&s);
    assert(it != u32set_end(&s));
    u32set_elem_t out = 0;
    u32set_remove_at(&s, it, &out, NULL);
    assert(out == 55);
    assert(u32set_is_empty(&s));
    u32set_destroy(&s);
}

// ── Tests: owned-key set (destroy callback) ───────────────────────────────────

static void test_owned_key_insert_destroys_duplicate(void) {
    printf("Test: duplicate insert destroys the incoming owned key\n");
    strset_t s = strset_new();
    owned_str_t a = owned_str_make("hello");
    assert(strset_insert(&s, &a) != strset_end(&s));
    // Duplicate: the incoming key is destroyed (freed) by insert, the existing
    // entry is kept. Run under valgrind to confirm no leak/double-free.
    owned_str_t b = owned_str_make("hello");
    assert(strset_insert(&s, &b) != strset_end(&s));
    assert(strset_size(&s) == 1);
    owned_str_t probe = owned_str_make("hello");
    assert(strset_contains(&s, &probe));
    free(probe.str);
    strset_destroy(&s);  // frees the one stored key
    assert(strset_is_empty(&s));
}

static void test_owned_key_remove_destroys(void) {
    printf("Test: remove destroys the owned key\n");
    strset_t s = strset_new();
    owned_str_t a = owned_str_make("alpha");
    owned_str_t b = owned_str_make("beta");
    assert(strset_insert(&s, &a) != strset_end(&s));
    assert(strset_insert(&s, &b) != strset_end(&s));
    owned_str_t key_a = owned_str_make("alpha");
    assert(strset_remove(&s, &key_a));  // frees the stored "alpha"
    free(key_a.str);
    assert(strset_size(&s) == 1);
    strset_destroy(&s);  // frees the stored "beta"
}

// ── Tests: algorithms_template.h macros ───────────────────────────────────────

static void test_algorithms_foreach(void) {
    printf("Test: _ZP_FOREACH visits every key exactly once\n");
    u32set_t s = u32set_new();
    const uint32_t N = 8;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i;
        assert(u32set_insert(&s, &k) != u32set_end(&s));
    }
    bool seen[8] = {false};
    size_t count = 0;
    u32set_elem_t *key = NULL;
    _ZP_FOREACH (u32set, &s, key) {
        assert(*key < N && !seen[*key]);
        seen[*key] = true;
        count++;
    }
    assert(count == N);
    u32set_destroy(&s);
}

static void test_algorithms_find(void) {
    printf("Test: _ZP_CONST_FIND locates first matching key, NULL when absent\n");
    u32set_t s = u32set_new();
    for (uint32_t i = 0; i < 10; i++) {
        uint32_t k = i;
        assert(u32set_insert(&s, &k) != u32set_end(&s));
    }
    const u32set_elem_t *found = NULL;
    _ZP_CONST_FIND(u32set, &s, found, *_ == 5);
    assert(found != NULL && *found == 5);
    _ZP_CONST_FIND(u32set, &s, found, *_ == 999);
    assert(found == NULL);
    u32set_destroy(&s);
}

static void test_algorithms_remove(void) {
    printf("Test: _ZP_REMOVE removes all matching keys\n");
    u32set_t s = u32set_new();
    const uint32_t N = 12;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i;
        assert(u32set_insert(&s, &k) != u32set_end(&s));
    }
    _ZP_REMOVE(u32set, &s, *_ % 2 != 0);
    assert(u32set_size(&s) == N / 2);
    for (uint32_t i = 0; i < N; i++) {
        assert(u32set_contains(&s, &i) == (i % 2 == 0));
    }
    u32set_destroy(&s);
}

int main(void) {
    test_new_is_empty();
    test_insert_and_contains();
    test_node_is_key();
    test_get_iter();
    test_insert_duplicate_is_noop();
    test_remove_existing();
    test_remove_missing_returns_false();
    test_remove_head_of_chain();
    test_remove_tail_of_chain();
    test_destroy_and_reuse();
    test_pool_exhaustion();
    test_pool_slot_reused_after_remove();
    test_multiple_collisions();
    test_empty_iteration();
    test_iteration_visits_all();
    test_iteration_visits_all_with_collisions();
    test_iteration_removal_pattern();
    test_remove_at_moves_key_out();
    test_owned_key_insert_destroys_duplicate();
    test_owned_key_remove_destroys();
    test_algorithms_foreach();
    test_algorithms_find();
    test_algorithms_remove();
    printf("All static_hashset tests passed\n");
    return 0;
}
