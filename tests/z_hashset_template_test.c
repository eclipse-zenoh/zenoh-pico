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

// ── Instantiate a uint32_t set, small initial capacity to force growth ───────
//
// Identity hash: bucket = key % capacity. Using the identity makes collisions
// fully predictable, so the collision-chain tests below can deliberately place
// several keys into the same bucket.
static inline size_t u32_hash(const uint32_t *k) { return (size_t)(*k); }
static inline bool u32_eq(const uint32_t *a, const uint32_t *b) { return *a == *b; }

#define _ZP_HASHSET_TEMPLATE_KEY_TYPE uint32_t
#define _ZP_HASHSET_TEMPLATE_NAME u32set
#define _ZP_HASHSET_TEMPLATE_INITIAL_CAPACITY 4
#define _ZP_HASHSET_TEMPLATE_KEY_HASH_FN u32_hash
#define _ZP_HASHSET_TEMPLATE_KEY_EQ_FN u32_eq
#include "zenoh-pico/collections/hashset_template.h"

// ── Instantiate a uint8_t-indexed set to exercise the capacity limit ─────────

#define _ZP_HASHSET_TEMPLATE_KEY_TYPE uint32_t
#define _ZP_HASHSET_TEMPLATE_NAME u8idxset
#define _ZP_HASHSET_TEMPLATE_INDEX_TYPE uint8_t
#define _ZP_HASHSET_TEMPLATE_INITIAL_CAPACITY 2
#define _ZP_HASHSET_TEMPLATE_KEY_HASH_FN u32_hash
#define _ZP_HASHSET_TEMPLATE_KEY_EQ_FN u32_eq
#include "zenoh-pico/collections/hashset_template.h"

// ── Instantiate with a custom REALLOC_FN to exercise the in-place grow path ──

#define _ZP_HASHSET_TEMPLATE_KEY_TYPE uint32_t
#define _ZP_HASHSET_TEMPLATE_NAME reallocset
#define _ZP_HASHSET_TEMPLATE_INITIAL_CAPACITY 2
#define _ZP_HASHSET_TEMPLATE_KEY_HASH_FN u32_hash
#define _ZP_HASHSET_TEMPLATE_KEY_EQ_FN u32_eq
#define _ZP_HASHSET_TEMPLATE_REALLOC_FN(ptr, bytes) realloc(ptr, bytes)
#include "zenoh-pico/collections/hashset_template.h"

// ── Instantiate a set of owned_str_t to exercise the key destroy/move path ───

// A key that owns a heap buffer, so we can verify that insert destroys the
// incoming duplicate key, that growth relocates keys via KEY_MOVE_FN, and that
// destroy/remove release the stored key.
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
static inline void owned_str_move(owned_str_t *dst, owned_str_t *src) {
    *dst = *src;
    src->str = NULL;
}

#define _ZP_HASHSET_TEMPLATE_KEY_TYPE owned_str_t
#define _ZP_HASHSET_TEMPLATE_NAME strset
#define _ZP_HASHSET_TEMPLATE_INITIAL_CAPACITY 2
#define _ZP_HASHSET_TEMPLATE_KEY_HASH_FN owned_str_hash
#define _ZP_HASHSET_TEMPLATE_KEY_EQ_FN owned_str_eq
#define _ZP_HASHSET_TEMPLATE_KEY_MOVE_FN(d, s) owned_str_move(d, s)
#define _ZP_HASHSET_TEMPLATE_KEY_DESTROY_FN(p) free((p)->str)
#include "zenoh-pico/collections/hashset_template.h"

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_new_is_empty(void) {
    printf("Test: new set is empty (no allocation until first insert)\n");
    u32set_t s = u32set_new();
    assert(u32set_is_empty(&s));
    assert(u32set_size(&s) == 0);
    assert(u32set_capacity(&s) == 0);
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
    // get_iter on an unallocated set must be safe and miss.
    assert(u32set_get_iter(&s, &(uint32_t){5}) == u32set_end(&s));
    uint32_t k = 5;
    u32set_iter_t inserted = u32set_insert(&s, &k);
    assert(inserted != u32set_end(&s));
    assert(u32set_get_iter(&s, &(uint32_t){5}) == inserted);
    assert(u32set_get_iter(&s, &(uint32_t){6}) == u32set_end(&s));
    u32set_destroy(&s);
}

static void test_insert_duplicate_is_noop(void) {
    printf("Test: inserting a duplicate key leaves size unchanged\n");
    u32set_t s = u32set_new();
    uint32_t k = 5;
    u32set_iter_t first = u32set_insert(&s, &k);
    assert(first != u32set_end(&s));
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
    assert(!u32set_remove(&s, &(uint32_t){99}));  // unallocated set
    uint32_t k = 1;
    assert(u32set_insert(&s, &k) != u32set_end(&s));
    assert(!u32set_remove(&s, &(uint32_t){99}));
    u32set_destroy(&s);
}

static void test_growth_preserves_entries(void) {
    printf("Test: set grows beyond initial capacity, all entries retained\n");
    u32set_t s = u32set_new();
    const uint32_t N = 100;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i;
        assert(u32set_insert(&s, &k) != u32set_end(&s));
    }
    assert(u32set_size(&s) == N);
    assert(u32set_capacity(&s) >= N);
    for (uint32_t i = 0; i < N; i++) {
        assert(u32set_contains(&s, &i));
    }
    u32set_destroy(&s);
}

static void test_iterator_stable_across_growth(void) {
    printf("Test: iterators (indices) preserved across rehashing/growth\n");
    u32set_t s = u32set_new();
    uint32_t k0 = 0;
    u32set_iter_t it0 = u32set_insert(&s, &k0);
    // Force several growths; the entry for key 0 must keep its index and value.
    for (uint32_t i = 1; i < 100; i++) {
        uint32_t k = i;
        assert(u32set_insert(&s, &k) != u32set_end(&s));
    }
    assert(u32set_get_iter(&s, &(uint32_t){0}) == it0);
    assert(*u32set_at(&s, it0) == 0);
    u32set_destroy(&s);
}

static void test_reserve(void) {
    printf("Test: reserve pre-allocates capacity\n");
    u32set_t s = u32set_new();
    assert(u32set_reserve(&s, 64));
    assert(u32set_capacity(&s) >= 64);
    size_t cap = u32set_capacity(&s);
    // Inserting within the reserved capacity must not grow further.
    for (uint32_t i = 0; i < 64; i++) {
        uint32_t k = i;
        assert(u32set_insert(&s, &k) != u32set_end(&s));
    }
    assert(u32set_capacity(&s) == cap);
    u32set_destroy(&s);
}

static void test_clear_and_reuse(void) {
    printf("Test: clear empties the set but keeps the backing store\n");
    u32set_t s = u32set_new();
    for (uint32_t i = 0; i < 20; i++) {
        uint32_t k = i;
        assert(u32set_insert(&s, &k) != u32set_end(&s));
    }
    size_t cap = u32set_capacity(&s);
    u32set_clear(&s);
    assert(u32set_is_empty(&s));
    assert(u32set_capacity(&s) == cap);  // backing store retained
    for (uint32_t i = 0; i < 20; i++) {
        uint32_t k = i + 100;
        assert(u32set_insert(&s, &k) != u32set_end(&s));
    }
    assert(u32set_size(&s) == 20);
    u32set_destroy(&s);
}

static void test_remove_head_and_tail_of_chain(void) {
    printf("Test: remove head/tail of a collision chain; other entry retained\n");
    u32set_t s = u32set_new();
    assert(u32set_reserve(&s, 8));
    size_t cap = u32set_capacity(&s);
    // Two keys that share a bucket at the current capacity.
    uint32_t k0 = 0, k1 = (uint32_t)cap;
    assert(u32set_insert(&s, &k0) != u32set_end(&s));
    assert(u32set_insert(&s, &k1) != u32set_end(&s));
    assert(u32set_size(&s) == 2);
    assert(u32set_remove(&s, &k1));  // tail (inserted last => head of chain)
    assert(u32set_contains(&s, &k0));
    assert(!u32set_contains(&s, &k1));
    assert(u32set_remove(&s, &k0));
    assert(u32set_is_empty(&s));
    u32set_destroy(&s);
}

static void test_small_index_type_capacity_limit(void) {
    printf("Test: uint8_t index type refuses to grow past max addressable capacity\n");
    u8idxset_t s = u8idxset_new();
    // Max addressable capacity for uint8_t is 255 (sentinel reserved).
    bool exhausted = false;
    for (uint32_t i = 0; i < 300; i++) {
        uint32_t k = i;
        if (u8idxset_insert(&s, &k) == u8idxset_end(&s)) {
            exhausted = true;
            break;
        }
    }
    assert(exhausted);
    assert(u8idxset_size(&s) <= 255);
    u8idxset_destroy(&s);
}

static void test_realloc_grow_path(void) {
    printf("Test: in-place realloc grow path preserves entries and iterators\n");
    reallocset_t s = reallocset_new();
    uint32_t k0 = 0;
    reallocset_iter_t it0 = reallocset_insert(&s, &k0);
    const uint32_t N = 100;
    for (uint32_t i = 1; i < N; i++) {
        uint32_t k = i;
        assert(reallocset_insert(&s, &k) != reallocset_end(&s));
    }
    assert(reallocset_size(&s) == N);
    // Index stability holds regardless of the relocation strategy.
    assert(reallocset_get_iter(&s, &(uint32_t){0}) == it0);
    for (uint32_t i = 0; i < N; i++) {
        assert(reallocset_contains(&s, &i));
    }
    reallocset_destroy(&s);
}

// ── Tests: iteration ──────────────────────────────────────────────────────────

static void test_empty_iteration(void) {
    printf("Test: iteration over empty set yields no elements\n");
    u32set_t s = u32set_new();
    assert(u32set_begin(&s) == u32set_end(&s));  // unallocated
    uint32_t k = 1;
    assert(u32set_insert(&s, &k) != u32set_end(&s));
    assert(u32set_remove(&s, &k));
    assert(u32set_begin(&s) == u32set_end(&s));  // allocated but empty
    u32set_destroy(&s);
}

static void test_iteration_visits_all(void) {
    printf("Test: forward iteration visits every key exactly once\n");
    u32set_t s = u32set_new();
    const uint32_t N = 50;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i;
        assert(u32set_insert(&s, &k) != u32set_end(&s));
    }
    bool seen[50] = {false};
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

static void test_iteration_removal_pattern(void) {
    printf("Test: safe removal of even keys during iteration\n");
    u32set_t s = u32set_new();
    const uint32_t N = 20;
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

// ── Tests: owned-key set (destroy/move callbacks) ─────────────────────────────

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

static void test_owned_key_growth_relocates(void) {
    printf("Test: growth relocates owned keys via KEY_MOVE_FN without leaking\n");
    strset_t s = strset_new();
    char buf[32];
    const int N = 50;
    for (int i = 0; i < N; i++) {
        snprintf(buf, sizeof(buf), "key-%d", i);
        owned_str_t o = owned_str_make(buf);
        assert(strset_insert(&s, &o) != strset_end(&s));  // triggers several growths
    }
    assert(strset_size(&s) == (size_t)N);
    for (int i = 0; i < N; i++) {
        snprintf(buf, sizeof(buf), "key-%d", i);
        owned_str_t probe = owned_str_make(buf);
        assert(strset_contains(&s, &probe));
        free(probe.str);
    }
    strset_destroy(&s);  // frees every relocated key exactly once
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
    const uint32_t N = 16;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i;
        assert(u32set_insert(&s, &k) != u32set_end(&s));
    }
    bool seen[16] = {false};
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
    test_growth_preserves_entries();
    test_iterator_stable_across_growth();
    test_reserve();
    test_clear_and_reuse();
    test_remove_head_and_tail_of_chain();
    test_small_index_type_capacity_limit();
    test_realloc_grow_path();
    test_empty_iteration();
    test_iteration_visits_all();
    test_iteration_removal_pattern();
    test_remove_at_moves_key_out();
    test_owned_key_insert_destroys_duplicate();
    test_owned_key_growth_relocates();
    test_owned_key_remove_destroys();
    test_algorithms_foreach();
    test_algorithms_find();
    test_algorithms_remove();
    printf("All hashset tests passed\n");
    return 0;
}
