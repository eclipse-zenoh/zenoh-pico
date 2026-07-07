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

// ── Instantiate uint32_t -> uint32_t, default index type, small initial cap ──

// Identity hash: bucket = key % capacity. Using the identity makes collisions
// fully predictable, so the collision-chain tests below can deliberately place
// several keys into the same bucket.
static inline size_t u32_hash(const uint32_t *k) { return (size_t)(*k); }
static inline bool u32_eq(const uint32_t *a, const uint32_t *b) { return *a == *b; }

#define _ZP_HASHMAP_TEMPLATE_KEY_TYPE uint32_t
#define _ZP_HASHMAP_TEMPLATE_VAL_TYPE uint32_t
#define _ZP_HASHMAP_TEMPLATE_NAME u32map
#define _ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY 4
#define _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN u32_hash
#define _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN u32_eq
#include "zenoh-pico/collections/hashmap_template.h"

// ── Instantiate with a uint8_t index type to exercise the small-index path ───

#define _ZP_HASHMAP_TEMPLATE_KEY_TYPE uint32_t
#define _ZP_HASHMAP_TEMPLATE_VAL_TYPE uint32_t
#define _ZP_HASHMAP_TEMPLATE_NAME u8idxmap
#define _ZP_HASHMAP_TEMPLATE_INDEX_TYPE uint8_t
#define _ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY 2
#define _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN u32_hash
#define _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN u32_eq
#include "zenoh-pico/collections/hashmap_template.h"

// ── Instantiate with a custom REALLOC_FN to exercise the in-place grow path ──
// Both key and value are trivially movable, so growth resizes the pool with
// realloc instead of allocate-and-move.

#define _ZP_HASHMAP_TEMPLATE_KEY_TYPE uint32_t
#define _ZP_HASHMAP_TEMPLATE_VAL_TYPE uint32_t
#define _ZP_HASHMAP_TEMPLATE_NAME reallocmap
#define _ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY 2
#define _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN u32_hash
#define _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN u32_eq
#define _ZP_HASHMAP_TEMPLATE_REALLOC_FN(ptr, bytes) realloc(ptr, bytes)
#include "zenoh-pico/collections/hashmap_template.h"

// ── Instantiate with heap-allocated string values to test destroy/move ───────

static inline size_t int_hash(const int *k) { return (size_t)(*k); }
static inline bool int_eq(const int *a, const int *b) { return *a == *b; }

// Wrap the owned pointer in a struct so the value type is not itself a bare
// pointer (keeps const-correctness clean for const_get/const_at accessors).
typedef struct {
    char *str;
} owned_str_t;

#define _ZP_HASHMAP_TEMPLATE_KEY_TYPE int
#define _ZP_HASHMAP_TEMPLATE_VAL_TYPE owned_str_t
#define _ZP_HASHMAP_TEMPLATE_NAME strmap
#define _ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY 2
#define _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN int_hash
#define _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN int_eq
#define _ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(dst, src) (*(dst) = *(src), (src)->str = NULL)
#define _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN(p) free((p)->str)
#include "zenoh-pico/collections/hashmap_template.h"

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_new_is_empty(void) {
    printf("Test: new map is empty\n");
    u32map_t m = u32map_new();
    assert(u32map_is_empty(&m));
    assert(u32map_size(&m) == 0);
    assert(u32map_begin(&m) == u32map_end(&m));
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
    printf("Test: get on missing key returns NULL (and on empty map)\n");
    u32map_t m = u32map_new();
    assert(u32map_get(&m, &(uint32_t){42}) == NULL);
    uint32_t k = 1, v = 1;
    assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
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
    assert(n->val.str != NULL);
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
    assert(n->val.str != NULL);
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

static void test_growth_preserves_entries(void) {
    printf("Test: map grows beyond initial capacity, all entries retrievable\n");
    u32map_t m = u32map_new();
    const uint32_t N = 1000;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i * 3;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    assert(u32map_size(&m) == N);
    assert(u32map_capacity(&m) >= N);
    for (uint32_t i = 0; i < N; i++) {
        uint32_t *got = u32map_get(&m, &i);
        assert(got != NULL && *got == i * 3);
    }
    u32map_destroy(&m);
}

static void test_iterator_stable_across_growth(void) {
    printf("Test: iterators (indices) preserved across rehashing/growth\n");
    u32map_t m = u32map_new();
    // Insert one entry and capture its iterator.
    uint32_t k = 12345, v = 67890;
    u32map_iter_t it = u32map_insert(&m, &k, &v);
    assert(it != u32map_end(&m));
    // The very first inserted entry occupies slot 0.
    assert(it == 0);

    // Force several growths by inserting many other keys.
    for (uint32_t i = 1; i <= 500; i++) {
        uint32_t kk = i, vv = i;
        assert(u32map_insert(&m, &kk, &vv) != u32map_end(&m));
    }

    // The captured iterator must still refer to the same entry.
    u32map_elem_t *n = u32map_at(&m, it);
    assert(n->key == 12345 && n->val == 67890);
    // And looking the key up again yields the same iterator.
    assert(u32map_get_iter(&m, &(uint32_t){12345}) == it);
    u32map_destroy(&m);
}

static void test_clear_and_reuse(void) {
    printf("Test: clear empties the map but keeps backing store for reuse\n");
    u32map_t m = u32map_new();
    for (uint32_t i = 0; i < 50; i++) {
        uint32_t k = i, v = i * 10;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    assert(u32map_size(&m) == 50);
    size_t cap = u32map_capacity(&m);
    u32map_clear(&m);
    assert(u32map_is_empty(&m));
    assert(u32map_capacity(&m) == cap);  // backing store retained
    assert(u32map_begin(&m) == u32map_end(&m));
    // Reuse
    for (uint32_t i = 0; i < 50; i++) {
        uint32_t k = i + 100, v = i;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    assert(u32map_size(&m) == 50);
    u32map_destroy(&m);
}

static void test_remove_head_and_tail_of_chain(void) {
    printf("Test: remove head/tail of collision chains\n");
    u32map_t m = u32map_new();
    assert(u32map_reserve(&m, 8));
    // With bucket_count=8, keys 0 and 8 collide.
    uint32_t k0 = 0, v0 = 100, k8 = 8, v8 = 800;
    assert(u32map_insert(&m, &k0, &v0) != u32map_end(&m));
    assert(u32map_insert(&m, &k8, &v8) != u32map_end(&m));
    assert(u32map_size(&m) == 2);
    assert(u32map_remove(&m, &(uint32_t){8}, NULL));  // head (inserted last)
    assert(*u32map_get(&m, &(uint32_t){0}) == 100);
    assert(u32map_get(&m, &(uint32_t){8}) == NULL);
    assert(u32map_remove(&m, &(uint32_t){0}, NULL));
    assert(u32map_is_empty(&m));
    u32map_destroy(&m);
}

static void test_iteration_visits_all(void) {
    printf("Test: forward iteration visits every live entry exactly once\n");
    u32map_t m = u32map_new();
    const uint32_t N = 64;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i * 10;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    bool seen[64] = {false};
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

static void test_iteration_removal_pattern(void) {
    printf("Test: safe removal of even keys during iteration\n");
    u32map_t m = u32map_new();
    const uint32_t N = 40;
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

static void test_remove_at_moves_value_out(void) {
    printf("Test: remove_at with out_val moves node out correctly\n");
    u32map_t m = u32map_new();
    uint32_t k = 55, v = 550;
    u32map_iter_t it = u32map_insert(&m, &k, &v);
    assert(it != u32map_end(&m));
    u32map_elem_t out;
    u32map_remove_at(&m, it, &out, NULL);
    assert(out.key == 55 && out.val == 550);
    assert(u32map_is_empty(&m));
    u32map_destroy(&m);
}

static void test_small_index_type_growth(void) {
    printf("Test: uint8_t index type grows and addresses up to 255 entries\n");
    u8idxmap_t m = u8idxmap_new();
    const uint32_t N = 200;  // < 255 max addressable with uint8_t
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i + 1;
        assert(u8idxmap_insert(&m, &k, &v) != u8idxmap_end(&m));
    }
    assert(u8idxmap_size(&m) == N);
    for (uint32_t i = 0; i < N; i++) {
        uint32_t *got = u8idxmap_get(&m, &i);
        assert(got != NULL && *got == i + 1);
    }
    u8idxmap_destroy(&m);
}

static void test_small_index_type_capacity_limit(void) {
    printf("Test: uint8_t index type refuses to grow past max addressable capacity\n");
    u8idxmap_t m = u8idxmap_new();
    // Max addressable capacity for uint8_t is 255 (sentinel reserved).
    uint32_t inserted = 0;
    for (uint32_t i = 0; i < 1000; i++) {
        uint32_t k = i, v = i;
        if (u8idxmap_insert(&m, &k, &v) == u8idxmap_end(&m)) {
            break;
        }
        inserted++;
    }
    // Should be able to insert up to 255 entries, then fail.
    assert(inserted == 255);
    assert(u8idxmap_size(&m) == 255);
    u8idxmap_destroy(&m);
}

static void test_value_destroy_called(void) {
    printf("Test: heap-allocated values are destroyed on remove/clear/destroy\n");
    strmap_t m = strmap_new();
    for (int i = 0; i < 20; i++) {
        owned_str_t s;
        s.str = (char *)malloc(16);
        assert(s.str != NULL);
        snprintf(s.str, 16, "val-%d", i);
        assert(strmap_insert(&m, &i, &s) != strmap_end(&m));
    }
    assert(strmap_size(&m) == 20);

    // Overwrite an existing key — old value must be freed by the template.
    owned_str_t replacement;
    replacement.str = (char *)malloc(16);
    assert(replacement.str != NULL);
    snprintf(replacement.str, 16, "new");
    int key = 0;
    assert(strmap_insert(&m, &key, &replacement) != strmap_end(&m));
    assert(strmap_size(&m) == 20);

    // Remove one with move-out: we own the returned value.
    owned_str_t out;
    out.str = NULL;
    assert(strmap_remove(&m, &(int){5}, &out));
    free(out.str);
    assert(strmap_size(&m) == 19);

    // Remove one with destroy.
    assert(strmap_remove(&m, &(int){6}, NULL));
    assert(strmap_size(&m) == 18);

    // destroy frees the rest (run under valgrind to confirm no leaks).
    strmap_destroy(&m);
    assert(strmap_is_empty(&m));
}

static void test_realloc_grow_path(void) {
    printf("Test: in-place realloc grow path preserves entries and iterators\n");
    reallocmap_t m = reallocmap_new();
    // Capture an iterator to the first entry, then force many growths.
    uint32_t k0 = 777, v0 = 888;
    reallocmap_iter_t it = reallocmap_insert(&m, &k0, &v0);
    assert(it != reallocmap_end(&m));
    assert(it == 0);

    const uint32_t N = 500;
    for (uint32_t i = 1; i < N; i++) {
        uint32_t k = i, v = i * 7;
        assert(reallocmap_insert(&m, &k, &v) != reallocmap_end(&m));
    }
    assert(reallocmap_size(&m) == N);

    // Iterator must still point at the original entry after realloc-based growth.
    reallocmap_elem_t *n = reallocmap_at(&m, it);
    assert(n->key == 777 && n->val == 888);
    assert(reallocmap_get_iter(&m, &(uint32_t){777}) == it);

    // All entries retrievable.
    for (uint32_t i = 1; i < N; i++) {
        uint32_t *got = reallocmap_get(&m, &i);
        assert(got != NULL && *got == i * 7);
    }
    reallocmap_destroy(&m);
}

// Regression test for the free-list rebuild during growth of a PARTIALLY-FILLED
// map (capacity > size, i.e. some old slots are already on the free list).
// This exercises all three relocation strategies: memcpy (u32map), realloc
// (reallocmap) and per-entry move with a destructor (strmap).
static void test_partial_fill_grow_preserves_free_list(void) {
    printf("Test: grow on a partially-filled map keeps old free slots usable\n");

    // ── Strategy 2: memcpy relocation (default trivially-movable map) ──
    {
        u32map_t m = u32map_new();
        // Fill to capacity then remove a chunk so the free list spans old slots.
        for (uint32_t i = 0; i < 16; i++) {
            uint32_t k = i, v = i * 10;
            assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
        }
        for (uint32_t i = 0; i < 6; i++) {
            assert(u32map_remove(&m, &i, NULL));
        }
        assert(u32map_size(&m) == 10);
        // Force a grow while the map is NOT full.
        assert(u32map_reserve(&m, 64));
        size_t cap = u32map_capacity(&m);
        assert(cap >= 64);
        // Every slot up to capacity must be allocatable: fill the whole pool.
        for (uint32_t i = 16; u32map_size(&m) < cap; i++) {
            uint32_t k = i, v = i;
            assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
        }
        assert(u32map_size(&m) == cap);
        // Surviving entries are intact.
        for (uint32_t i = 6; i < 16; i++) {
            uint32_t *got = u32map_get(&m, &i);
            assert(got != NULL && *got == i * 10);
        }
        u32map_destroy(&m);
    }

    // ── Strategy 1: realloc relocation ──
    {
        reallocmap_t m = reallocmap_new();
        for (uint32_t i = 0; i < 16; i++) {
            uint32_t k = i, v = i * 10;
            assert(reallocmap_insert(&m, &k, &v) != reallocmap_end(&m));
        }
        for (uint32_t i = 0; i < 6; i++) {
            assert(reallocmap_remove(&m, &i, NULL));
        }
        assert(reallocmap_size(&m) == 10);
        assert(reallocmap_reserve(&m, 64));
        size_t cap = reallocmap_capacity(&m);
        assert(cap >= 64);
        for (uint32_t i = 16; reallocmap_size(&m) < cap; i++) {
            uint32_t k = i, v = i;
            assert(reallocmap_insert(&m, &k, &v) != reallocmap_end(&m));
        }
        assert(reallocmap_size(&m) == cap);
        for (uint32_t i = 6; i < 16; i++) {
            uint32_t *got = reallocmap_get(&m, &i);
            assert(got != NULL && *got == i * 10);
        }
        reallocmap_destroy(&m);
    }

    // ── Strategy 3: per-entry move with a value destructor ──
    {
        strmap_t m = strmap_new();
        for (int i = 0; i < 16; i++) {
            owned_str_t s;
            s.str = (char *)malloc(16);
            assert(s.str != NULL);
            snprintf(s.str, 16, "v-%d", i);
            assert(strmap_insert(&m, &i, &s) != strmap_end(&m));
        }
        for (int i = 0; i < 6; i++) {
            assert(strmap_remove(&m, &i, NULL));
        }
        assert(strmap_size(&m) == 10);
        assert(strmap_reserve(&m, 64));
        size_t cap = strmap_capacity(&m);
        assert(cap >= 64);
        for (int i = 16; strmap_size(&m) < cap; i++) {
            owned_str_t s;
            s.str = (char *)malloc(16);
            assert(s.str != NULL);
            snprintf(s.str, 16, "v-%d", i);
            assert(strmap_insert(&m, &i, &s) != strmap_end(&m));
        }
        assert(strmap_size(&m) == cap);
        // Surviving entries kept their values across the move-based growth.
        for (int i = 6; i < 16; i++) {
            char expected[16];
            snprintf(expected, sizeof(expected), "v-%d", i);
            const owned_str_t *got = strmap_const_get(&m, &i);
            assert(got != NULL && strcmp(got->str, expected) == 0);
        }
        strmap_destroy(&m);  // frees remaining strings (run under valgrind/ASan)
    }
}

// ── Tests: algorithms_template.h macros ───────────────────────────────────────

static void test_algorithms_foreach(void) {
    printf("Test: _ZP_FOREACH visits every entry exactly once\n");
    u32map_t m = u32map_new();
    const uint32_t N = 20;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i * 5;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    bool seen[20] = {false};
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
    const uint32_t N = 15;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i + 100;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    bool seen[15] = {false};
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
    printf("Test: _ZP_FOREACH_VAL iterates values\n");
    u32map_t m = u32map_new();
    uint32_t sum = 0;
    for (uint32_t i = 1; i <= 10; i++) {
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
    for (uint32_t i = 0; i < 20; i++) {
        uint32_t k = i, v = i * 3;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    // Predicate: entry whose value equals 21 (key 7)
    const u32map_elem_t *found = NULL;
    _ZP_CONST_FIND(u32map, &m, found, _->val == 21);

    assert(found != NULL && found->key == 7 && found->val == 21);

    // Predicate that matches nothing
    _ZP_CONST_FIND(u32map, &m, found, _->val == 9999);
    assert(found == NULL);

    u32map_destroy(&m);
}

static void test_algorithms_find_val(void) {
    printf("Test: _ZP_CONST_FIND_VAL locates first matching value\n");
    u32map_t m = u32map_new();
    for (uint32_t i = 0; i < 12; i++) {
        uint32_t k = i, v = i * 2;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    const uint32_t *found_val = NULL;
    _ZP_CONST_FIND_VAL(u32map, &m, found_val, *_ == 14);  // key 7 -> val 14
    assert(found_val != NULL && *found_val == 14);

    _ZP_CONST_FIND_VAL(u32map, &m, found_val, *_ == 9999);
    assert(found_val == NULL);

    u32map_destroy(&m);
}

static void test_algorithms_itfind(void) {
    printf("Test: _ZP_IT_FIND positions iterator on first matching entry\n");
    u32map_t m = u32map_new();
    const uint32_t N = 20;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i * 5;  // unique keys and values
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }

    // Found over the full range; node is mutable through at().
    u32map_iter_t it = u32map_begin(&m);
    u32map_iter_t end_it = u32map_end(&m);
    _ZP_IT_FIND(u32map, &m, it, end_it, _->key == 7);
    assert(it != end_it);
    u32map_iter_t pos = it;
    u32map_elem_t *node = u32map_at(&m, it);
    assert(node->key == 7 && node->val == 35);

    // begin bound: searching strictly after the match finds nothing (unique key).
    it = u32map_iter_next(&m, pos);
    end_it = u32map_end(&m);
    _ZP_IT_FIND(u32map, &m, it, end_it, _->key == 7);
    assert(it == end_it);

    // end bound: excluding the matching slot finds nothing.
    it = u32map_begin(&m);
    end_it = pos;
    _ZP_IT_FIND(u32map, &m, it, end_it, _->key == 7);
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

    // Mutation through the located node is visible via get().
    u32map_at(&m, pos)->val = 700;
    assert(*u32map_get(&m, &(uint32_t){7}) == 700);

    u32map_destroy(&m);
}

static void test_algorithms_citfind(void) {
    printf("Test: _ZP_CONST_IT_FIND positions iterator on first matching entry (const)\n");
    u32map_t m = u32map_new();
    const uint32_t N = 16;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i + 100;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }

    // Found over the full range; node accessed via const_at().
    u32map_iter_t it = u32map_begin(&m);
    u32map_iter_t end_it = u32map_end(&m);
    _ZP_CONST_IT_FIND(u32map, &m, it, end_it, _->key == 9);
    assert(it != end_it);
    u32map_iter_t pos = it;
    const u32map_elem_t *node = u32map_const_at(&m, it);
    assert(node->key == 9 && node->val == 109);

    // begin bound: searching strictly after the match finds nothing.
    it = u32map_iter_next(&m, pos);
    end_it = u32map_end(&m);
    _ZP_CONST_IT_FIND(u32map, &m, it, end_it, _->key == 9);
    assert(it == end_it);

    // end bound: excluding the matching slot finds nothing.
    it = u32map_begin(&m);
    end_it = pos;
    _ZP_CONST_IT_FIND(u32map, &m, it, end_it, _->key == 9);
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
    const uint32_t N = 18;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i * 2;  // unique values
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }

    // Found by value; value is mutable through at().
    u32map_iter_t it = u32map_begin(&m);
    u32map_iter_t end_it = u32map_end(&m);
    _ZP_IT_FIND(u32map, &m, it, end_it, _->val == 12);  // key 6 -> val 12
    assert(it != end_it);
    u32map_iter_t pos = it;
    assert(u32map_at(&m, it)->key == 6 && u32map_at(&m, it)->val == 12);

    // begin bound: searching strictly after the match finds nothing.
    it = u32map_iter_next(&m, pos);
    end_it = u32map_end(&m);
    _ZP_IT_FIND(u32map, &m, it, end_it, _->val == 12);
    assert(it == end_it);

    // end bound: excluding the matching slot finds nothing.
    it = u32map_begin(&m);
    end_it = pos;
    _ZP_IT_FIND(u32map, &m, it, end_it, _->val == 12);
    assert(it == pos);

    // No match advances to end.
    it = u32map_begin(&m);
    end_it = u32map_end(&m);
    _ZP_IT_FIND(u32map, &m, it, end_it, _->val == 9999);
    assert(it == end_it);

    // Mutation through the located value is visible via get().
    u32map_at(&m, pos)->val = 1200;
    assert(*u32map_get(&m, &(uint32_t){6}) == 1200);

    u32map_destroy(&m);
}

static void test_algorithms_citfind_val(void) {
    printf("Test: _ZP_CONST_IT_FIND_VAL positions iterator on first matching value (const)\n");
    u32map_t m = u32map_new();
    const uint32_t N = 14;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i * 3;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }

    // Found by value via const_at().
    u32map_iter_t it = u32map_begin(&m);
    u32map_iter_t end_it = u32map_end(&m);
    _ZP_CONST_IT_FIND(u32map, &m, it, end_it, _->val == 15);  // key 5 -> val 15
    assert(it != end_it);
    u32map_iter_t pos = it;
    assert(u32map_const_at(&m, it)->key == 5 && u32map_const_at(&m, it)->val == 15);

    // begin bound: searching strictly after the match finds nothing.
    it = u32map_iter_next(&m, pos);
    end_it = u32map_end(&m);
    _ZP_CONST_IT_FIND(u32map, &m, it, end_it, _->val == 15);
    assert(it == end_it);

    // end bound: excluding the matching slot finds nothing.
    it = u32map_begin(&m);
    end_it = pos;
    _ZP_CONST_IT_FIND(u32map, &m, it, end_it, _->val == 15);
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
    const uint32_t N = 30;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i;
        assert(u32map_insert(&m, &k, &v) != u32map_end(&m));
    }
    // Remove all entries with odd keys
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
    for (uint32_t i = 0; i < 16; i++) {
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
    test_contains();
    test_growth_preserves_entries();
    test_iterator_stable_across_growth();
    test_clear_and_reuse();
    test_remove_head_and_tail_of_chain();
    test_iteration_visits_all();
    test_iteration_removal_pattern();
    test_remove_at_moves_value_out();
    test_small_index_type_growth();
    test_small_index_type_capacity_limit();
    test_value_destroy_called();
    test_realloc_grow_path();
    test_partial_fill_grow_preserves_free_list();
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
    printf("All hashmap_template tests passed\n");
    return 0;
}
