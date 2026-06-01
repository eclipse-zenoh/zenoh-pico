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
// Tests for the resizable Robin Hood hashmap defined in hashmap_template.h.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#undef NDEBUG
#include <assert.h>

// ── Instantiate uint32_t -> uint32_t map ─────────────────────────────────────

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
#define _ZP_HASHMAP_TEMPLATE_NAME u32rhhmap
#define _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN u32_hash
#define _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN(a, b) u32_eq(a, b)
#define _ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY 4u  // small so resize is exercised early
#include "zenoh-pico/collections/hashmap_template.h"

// ── Instantiate a map with observable destructor side-effects ────────────────
// Key and value destructors increment global counters so tests can assert
// exactly how many destructions occurred.

static size_t g_key_dtor_count = 0;
static size_t g_val_dtor_count = 0;

static inline void counted_key_destroy(uint32_t *p) {
    (void)p;
    g_key_dtor_count++;
}
static inline void counted_val_destroy(uint32_t *p) {
    (void)p;
    g_val_dtor_count++;
}

#define _ZP_HASHMAP_TEMPLATE_KEY_TYPE uint32_t
#define _ZP_HASHMAP_TEMPLATE_VAL_TYPE uint32_t
#define _ZP_HASHMAP_TEMPLATE_NAME dtormap
#define _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN u32_hash
#define _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN(a, b) u32_eq(a, b)
#define _ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN(p) counted_key_destroy(p)
#define _ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN(p) counted_val_destroy(p)
// Plain copy move — the destroy counter fires only on explicit destroy calls,
// not implicitly inside every insert/move operation.
#define _ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY 4u
#include "zenoh-pico/collections/hashmap_template.h"

#define RESET_DTOR_COUNTS()   \
    do {                      \
        g_key_dtor_count = 0; \
        g_val_dtor_count = 0; \
    } while (0)

// ── Tests: destructor side-effects ────────────────────────────────────────────

static void test_dtor_remove_calls_key_and_val_destroy(void) {
    printf("Test (dtor): remove calls key and value destructors\n");
    dtormap_t m;
    assert(dtormap_new(&m));
    RESET_DTOR_COUNTS();

    uint32_t k = 1, v = 10;
    assert(dtormap_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    // Insert itself does not destroy anything.
    assert(g_key_dtor_count == 0 && g_val_dtor_count == 0);

    assert(dtormap_remove(&m, &(uint32_t){1}, NULL));
    // remove with out_val == NULL must destroy both key and value.
    assert(g_key_dtor_count == 1);
    assert(g_val_dtor_count == 1);

    dtormap_destroy(&m);
}

static void test_dtor_remove_with_out_val_does_not_destroy_val(void) {
    printf("Test (dtor): remove with out_val moves value out, does not destroy it\n");
    dtormap_t m;
    assert(dtormap_new(&m));
    RESET_DTOR_COUNTS();

    uint32_t k = 2, v = 20;
    assert(dtormap_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);

    uint32_t out = 0;
    assert(dtormap_remove(&m, &(uint32_t){2}, &out));
    assert(out == 20);
    // Key must be destroyed, value must be moved out (not destroyed).
    assert(g_key_dtor_count == 1);
    assert(g_val_dtor_count == 0);

    dtormap_destroy(&m);
}

static void test_dtor_insert_duplicate_destroys_old_val_and_new_key(void) {
    printf("Test (dtor): inserting duplicate key destroys old value and new key\n");
    dtormap_t m;
    assert(dtormap_new(&m));
    RESET_DTOR_COUNTS();

    uint32_t k1 = 5, v1 = 50;
    assert(dtormap_insert(&m, &k1, &v1) != _ZP_HASHMAP_ITER_INVALID);
    assert(g_key_dtor_count == 0 && g_val_dtor_count == 0);

    uint32_t k2 = 5, v2 = 99;
    assert(dtormap_insert(&m, &k2, &v2) != _ZP_HASHMAP_ITER_INVALID);
    // The duplicate key (k2) must be destroyed; the old value (v1=50) must be destroyed.
    assert(g_key_dtor_count == 1);  // new key destroyed
    assert(g_val_dtor_count == 1);  // old value destroyed
    // The new value must be in the map.
    assert(*dtormap_get(&m, &(uint32_t){5}) == 99);

    dtormap_destroy(&m);
    // destroy must clean up the remaining live entry (key + value).
    assert(g_key_dtor_count == 2);
    assert(g_val_dtor_count == 2);
}

static void test_dtor_destroy_calls_destructors_for_all_entries(void) {
    printf("Test (dtor): destroy calls key and value destructors for every live entry\n");
    dtormap_t m;
    assert(dtormap_new(&m));
    RESET_DTOR_COUNTS();

    const uint32_t N = 20;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i;
        assert(dtormap_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }
    assert(g_key_dtor_count == 0 && g_val_dtor_count == 0);

    dtormap_destroy(&m);
    assert(g_key_dtor_count == N);
    assert(g_val_dtor_count == N);
}

static void test_dtor_remove_at_without_out_destroys_both(void) {
    printf("Test (dtor): remove_at with out_val==NULL destroys key and value\n");
    dtormap_t m;
    assert(dtormap_new(&m));
    RESET_DTOR_COUNTS();

    uint32_t k = 7, v = 77;
    size_t it = dtormap_insert(&m, &k, &v);
    assert(it != _ZP_HASHMAP_ITER_INVALID);

    dtormap_remove_at(&m, it, NULL, NULL);
    assert(g_key_dtor_count == 1);
    assert(g_val_dtor_count == 1);

    dtormap_destroy(&m);
}

static void test_dtor_remove_at_with_out_does_not_destroy_val(void) {
    printf("Test (dtor): remove_at with out_val moves node out, does not destroy it\n");
    dtormap_t m;
    assert(dtormap_new(&m));
    RESET_DTOR_COUNTS();

    uint32_t k = 8, v = 88;
    size_t it = dtormap_insert(&m, &k, &v);
    assert(it != _ZP_HASHMAP_ITER_INVALID);

    dtormap_node_t out;
    dtormap_remove_at(&m, it, &out, NULL);
    assert(out.key == 8 && out.val == 88);
    // Both key and value are moved into `out`; neither should be destroyed yet.
    assert(g_key_dtor_count == 0);
    assert(g_val_dtor_count == 0);

    dtormap_destroy(&m);
}

static void test_dtor_destroy_after_removes_counts_correctly(void) {
    printf("Test (dtor): destroy after partial removals counts correctly\n");
    dtormap_t m;
    assert(dtormap_new(&m));
    RESET_DTOR_COUNTS();

    const uint32_t N = 10;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i;
        assert(dtormap_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }

    // Remove half via remove (destroys both key and val for each).
    for (uint32_t i = 0; i < N; i += 2) {
        assert(dtormap_remove(&m, &i, NULL));
    }
    assert(g_key_dtor_count == N / 2);
    assert(g_val_dtor_count == N / 2);

    // destroy must clean up the remaining N/2 entries.
    dtormap_destroy(&m);
    assert(g_key_dtor_count == N);
    assert(g_val_dtor_count == N);
}

static void test_dtor_resize_does_not_call_destructors(void) {
    printf("Test (dtor): internal resize does not invoke any destructors\n");
    dtormap_t m;
    assert(dtormap_new(&m));  // initial capacity = 4
    RESET_DTOR_COUNTS();

    // Inserting >3 elements forces at least one resize (load > 0.75 * 4 = 3).
    const uint32_t N = 32;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i;
        assert(dtormap_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }
    // No destructor must fire during resize: entries are moved, not destroyed.
    assert(g_key_dtor_count == 0);
    assert(g_val_dtor_count == 0);

    dtormap_destroy(&m);
    assert(g_key_dtor_count == N);
    assert(g_val_dtor_count == N);
}

// ── Tests: basic operations ───────────────────────────────────────────────────

static void test_new_is_empty(void) {
    printf("Test: new map is empty\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new(&m));
    assert(u32rhhmap_is_empty(&m));
    assert(u32rhhmap_size(&m) == 0);
    assert(u32rhhmap_is_valid(&m));
    u32rhhmap_destroy(&m);
    assert(!u32rhhmap_is_valid(&m));
}

static void test_new_with_capacity(void) {
    printf("Test: new_with_capacity rounds up to power of two\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new_with_capacity(&m, 3));  // should become 4
    assert(u32rhhmap_is_empty(&m));
    u32rhhmap_destroy(&m);
}

static void test_insert_and_get(void) {
    printf("Test: insert then get returns value\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new(&m));
    uint32_t k = 42, v = 1234;
    size_t it = u32rhhmap_insert(&m, &k, &v);
    assert(it != _ZP_HASHMAP_ITER_INVALID);
    assert(u32rhhmap_size(&m) == 1);
    uint32_t *got = u32rhhmap_get(&m, &(uint32_t){42});
    assert(got != NULL && *got == 1234);
    u32rhhmap_destroy(&m);
}

static void test_get_missing_returns_null(void) {
    printf("Test: get on missing key returns NULL\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new(&m));
    assert(u32rhhmap_get(&m, &(uint32_t){99}) == NULL);
    u32rhhmap_destroy(&m);
}

static void test_contains(void) {
    printf("Test: contains reflects insertions and removals\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new(&m));
    assert(!u32rhhmap_contains(&m, &(uint32_t){7}));
    uint32_t k = 7, v = 70;
    assert(u32rhhmap_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    assert(u32rhhmap_contains(&m, &(uint32_t){7}));
    assert(u32rhhmap_remove(&m, &(uint32_t){7}, NULL));
    assert(!u32rhhmap_contains(&m, &(uint32_t){7}));
    u32rhhmap_destroy(&m);
}

static void test_insert_updates_existing(void) {
    printf("Test: inserting duplicate key updates value, size stays the same\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new(&m));
    uint32_t k = 5, v1 = 50, v2 = 99;
    assert(u32rhhmap_insert(&m, &k, &v1) != _ZP_HASHMAP_ITER_INVALID);
    uint32_t k2 = 5;  // same key value; k already moved
    assert(u32rhhmap_insert(&m, &k2, &v2) != _ZP_HASHMAP_ITER_INVALID);
    assert(u32rhhmap_size(&m) == 1);
    assert(*u32rhhmap_get(&m, &(uint32_t){5}) == 99);
    u32rhhmap_destroy(&m);
}

static void test_remove_existing_moves_value(void) {
    printf("Test: remove existing key moves value out\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new(&m));
    uint32_t k = 7, v = 70;
    assert(u32rhhmap_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    uint32_t out = 0;
    assert(u32rhhmap_remove(&m, &(uint32_t){7}, &out));
    assert(out == 70);
    assert(u32rhhmap_size(&m) == 0);
    assert(u32rhhmap_get(&m, &(uint32_t){7}) == NULL);
    u32rhhmap_destroy(&m);
}

static void test_remove_missing_returns_false(void) {
    printf("Test: remove on missing key returns false\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new(&m));
    assert(!u32rhhmap_remove(&m, &(uint32_t){123}, NULL));
    u32rhhmap_destroy(&m);
}

static void test_remove_at(void) {
    printf("Test: remove_at by iterator index\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new(&m));
    uint32_t k = 3, v = 33;
    size_t it = u32rhhmap_insert(&m, &k, &v);
    assert(it != _ZP_HASHMAP_ITER_INVALID);
    u32rhhmap_node_t out;
    u32rhhmap_remove_at(&m, it, &out, NULL);
    assert(out.key == 3 && out.val == 33);
    assert(u32rhhmap_size(&m) == 0);
    u32rhhmap_destroy(&m);
}

// ── Tests: resize / growth ────────────────────────────────────────────────────

static void test_resize_on_load(void) {
    printf("Test: map grows automatically when load factor is exceeded\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new(&m));  // initial capacity = 4 (configured above)

    // Insert more elements than the initial capacity can hold at 0.75 load
    // (4 * 0.75 = 3 elements trigger growth to 8 → 8*0.75=6 → growth to 16 …)
    const uint32_t N = 64;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i * 2;
        assert(u32rhhmap_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }
    assert(u32rhhmap_size(&m) == N);
    // All entries must still be reachable after multiple resizes.
    for (uint32_t i = 0; i < N; i++) {
        uint32_t *got = u32rhhmap_get(&m, &i);
        assert(got != NULL && *got == i * 2);
    }
    u32rhhmap_destroy(&m);
}

static void test_resize_preserves_entries(void) {
    printf("Test: entries survive resize (re-insert into larger table)\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new_with_capacity(&m, 2));  // force small start

    uint32_t keys[] = {10, 20, 30, 40, 50};
    for (size_t i = 0; i < 5; i++) {
        uint32_t k = keys[i], v = keys[i] + 1;
        assert(u32rhhmap_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }
    for (size_t i = 0; i < 5; i++) {
        uint32_t *got = u32rhhmap_get(&m, &keys[i]);
        assert(got != NULL && *got == keys[i] + 1);
    }
    u32rhhmap_destroy(&m);
}

// ── Instantiate a collision-forcing map (constant hash) ───────────────────────
// Every key maps to bucket 0, maximising PSL depth and exercising Robin Hood
// insertion and backward-shift deletion.

#define _ZP_HASHMAP_TEMPLATE_KEY_TYPE uint32_t
#define _ZP_HASHMAP_TEMPLATE_VAL_TYPE uint32_t
#define _ZP_HASHMAP_TEMPLATE_NAME collmap
#define _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN(_k) ((size_t)0)
#define _ZP_HASHMAP_TEMPLATE_KEY_EQ_FN(a, b) (*(a) == *(b))
#define _ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY 16u
#include "zenoh-pico/collections/hashmap_template.h"

// ── Tests: collision handling ─────────────────────────────────────────────────

static void test_many_collisions(void) {
    printf("Test: many keys colliding into the same ideal bucket\n");
    collmap_t m;
    assert(collmap_new(&m));
    const uint32_t N = 20;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i * 3;
        assert(collmap_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }
    assert(collmap_size(&m) == N);
    for (uint32_t i = 0; i < N; i++) {
        uint32_t *got = collmap_get(&m, &i);
        assert(got != NULL && *got == i * 3);
    }
    // Remove half the entries and verify the rest.
    for (uint32_t i = 0; i < N; i += 2) {
        assert(collmap_remove(&m, &i, NULL));
    }
    assert(collmap_size(&m) == N / 2);
    for (uint32_t i = 1; i < N; i += 2) {
        uint32_t *got = collmap_get(&m, &i);
        assert(got != NULL && *got == i * 3);
    }
    for (uint32_t i = 0; i < N; i += 2) {
        assert(collmap_get(&m, &i) == NULL);
    }
    collmap_destroy(&m);
}

// ── Tests: collmap — remove_at, iteration  ──────────
// All keys share bucket 0 (constant hash), so every operation exercises the
// deepest PSL chains and the most complex backward-shift paths.

static void test_collmap_remove_at(void) {
    printf("Test (collmap): remove_at\n");
    collmap_t m;
    assert(collmap_new(&m));

    // Insert a chain of keys all hashing to bucket 0.
    const uint32_t N = 8;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i * 10;
        assert(collmap_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }
    assert(collmap_size(&m) == N);

    // Obtain the iterator for the head of the chain (first occupied slot).
    size_t it = collmap_iter(&m);
    assert(it != _ZP_HASHMAP_ITER_INVALID);
    uint32_t removed_key = collmap_node_at(&m, it)->key;

    // Remove head
    collmap_remove_at(&m, it, NULL, NULL);
    assert(collmap_size(&m) == N - 1);

    // Every remaining key must still be findable.
    for (uint32_t i = 0; i < N; i++) {
        uint32_t *got = collmap_get(&m, &i);
        if (i == removed_key) {
            assert(got == NULL);
        } else {
            assert(got != NULL && *got == i * 10);
        }
    }
    collmap_destroy(&m);
}

static void test_collmap_iteration_visits_all(void) {
    printf("Test (collmap): iteration visits every entry exactly once on a deep chain\n");
    collmap_t m;
    assert(collmap_new(&m));

    const uint32_t N = 15;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i;
        assert(collmap_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }

    bool seen[15] = {false};
    size_t count = 0;
    for (size_t i = collmap_iter(&m); i != _ZP_HASHMAP_ITER_INVALID; i = collmap_iter_next(&m, i)) {
        collmap_node_t *n = collmap_node_at(&m, i);
        assert(n->key < N && !seen[n->key]);
        assert(n->val == n->key);
        seen[n->key] = true;
        count++;
    }
    assert(count == N);
    for (uint32_t i = 0; i < N; i++) {
        assert(seen[i]);
    }
    collmap_destroy(&m);
}

static void test_collmap_iteration_removal_pattern(void) {
    printf("Test (collmap): safe removal during iteration on a deep chain\n");
    collmap_t m;
    assert(collmap_new(&m));

    const uint32_t N = 16;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i;
        assert(collmap_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }

    // Remove all even keys during iteration
    for (size_t i = collmap_iter(&m); i != _ZP_HASHMAP_ITER_INVALID;) {
        collmap_node_t *n = collmap_node_at(&m, i);
        if (n->key % 2 == 0) {
            collmap_remove_at(&m, i, NULL, &i);
        } else {
            i = collmap_iter_next(&m, i);
        }
    }

    assert(collmap_size(&m) == N / 2);
    for (uint32_t i = 0; i < N; i++) {
        uint32_t *got = collmap_get(&m, &i);
        if (i % 2 == 0) {
            assert(got == NULL);
        } else {
            assert(got != NULL && *got == i);
        }
    }
    collmap_destroy(&m);
}

// ── Tests: remove_at + backward-shift ────────────────────────────────────────

static void test_remove_at2(void) {
    printf("Test: remove_at with multiple elements\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new(&m));

    // Insert entries that land in adjacent slots so backward-shift
    // moves one of them into the vacated slot.
    uint32_t k0 = 0, v0 = 10;
    uint32_t k1 = 1, v1 = 20;
    size_t it0 = u32rhhmap_insert(&m, &k0, &v0);
    size_t it1 = u32rhhmap_insert(&m, &k1, &v1);
    assert(it0 != _ZP_HASHMAP_ITER_INVALID);
    assert(it1 != _ZP_HASHMAP_ITER_INVALID);

    // Remove first; the backward-shift may move the second into its slot.
    u32rhhmap_remove_at(&m, it0, NULL, NULL);
    assert(u32rhhmap_size(&m) == 1);
    // Regardless of shift, the remaining key must be findable.
    assert(u32rhhmap_get(&m, &(uint32_t){1}) != NULL);
    u32rhhmap_destroy(&m);
}

// ── Tests: iteration ──────────────────────────────────────────────────────────

static void test_iteration_visits_all(void) {
    printf("Test: forward iteration visits every live entry exactly once\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new(&m));
    const uint32_t N = 30;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i;
        assert(u32rhhmap_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }

    bool seen[30] = {false};
    size_t count = 0;
    for (size_t i = u32rhhmap_iter(&m); i != _ZP_HASHMAP_ITER_INVALID; i = u32rhhmap_iter_next(&m, i)) {
        u32rhhmap_node_t *n = u32rhhmap_node_at(&m, i);
        assert(n->key < N && !seen[n->key]);
        assert(n->val == n->key);
        seen[n->key] = true;
        count++;
    }
    assert(count == N);
    u32rhhmap_destroy(&m);
}

static void test_iteration_removal_pattern(void) {
    printf("Test: safe iteration with conditional removal\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new(&m));
    const uint32_t N = 20;
    for (uint32_t i = 0; i < N; i++) {
        uint32_t k = i, v = i;
        assert(u32rhhmap_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    }

    // Remove all even keys during iteration.
    for (size_t i = u32rhhmap_iter(&m); i != _ZP_HASHMAP_ITER_INVALID;) {
        u32rhhmap_node_t *n = u32rhhmap_node_at(&m, i);
        if (n->key % 2 == 0) {
            u32rhhmap_remove_at(&m, i, NULL, &i);
        } else {
            i = u32rhhmap_iter_next(&m, i);
        }
    }

    assert(u32rhhmap_size(&m) == N / 2);
    for (uint32_t i = 0; i < N; i++) {
        uint32_t *got = u32rhhmap_get(&m, &i);
        if (i % 2 == 0) {
            assert(got == NULL);
        } else {
            assert(got != NULL && *got == i);
        }
    }
    u32rhhmap_destroy(&m);
}

static void test_empty_iteration(void) {
    printf("Test: iteration over empty map yields no elements\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new(&m));
    assert(u32rhhmap_iter(&m) == _ZP_HASHMAP_ITER_INVALID);
    u32rhhmap_destroy(&m);
}

// ── Tests: destroy and reinitialise ──────────────────────────────────────────

static void test_destroy_then_reinit(void) {
    printf("Test: destroy then new on the same struct\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new(&m));
    uint32_t k = 1, v = 1;
    assert(u32rhhmap_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    u32rhhmap_destroy(&m);
    assert(!u32rhhmap_is_valid(&m));

    assert(u32rhhmap_new(&m));
    assert(u32rhhmap_size(&m) == 0);
    k = 2;
    v = 2;
    assert(u32rhhmap_insert(&m, &k, &v) != _ZP_HASHMAP_ITER_INVALID);
    assert(*u32rhhmap_get(&m, &(uint32_t){2}) == 2);
    u32rhhmap_destroy(&m);
}

// ── Tests: get_iter ───────────────────────────────────────────────────────────

static void test_get_iter_invalid_on_missing(void) {
    printf("Test: get_iter returns INVALID for missing key\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new(&m));
    assert(u32rhhmap_get_iter(&m, &(uint32_t){999}) == _ZP_HASHMAP_ITER_INVALID);
    u32rhhmap_destroy(&m);
}

static void test_get_iter_consistent_with_insert(void) {
    printf("Test: get_iter after insert points to the same value\n");
    u32rhhmap_t m;
    assert(u32rhhmap_new(&m));
    uint32_t k = 77, v = 777;
    size_t ins_it = u32rhhmap_insert(&m, &k, &v);
    size_t get_it = u32rhhmap_get_iter(&m, &(uint32_t){77});
    assert(ins_it == get_it);
    assert(u32rhhmap_node_at(&m, get_it)->val == 777);
    u32rhhmap_destroy(&m);
}

// ── main ──────────────────────────────────────────────────────────────────────

int main(void) {
    test_new_is_empty();
    test_new_with_capacity();
    test_insert_and_get();
    test_get_missing_returns_null();
    test_contains();
    test_insert_updates_existing();
    test_remove_existing_moves_value();
    test_remove_missing_returns_false();
    test_remove_at();
    test_resize_on_load();
    test_resize_preserves_entries();
    test_many_collisions();
    test_collmap_remove_at();
    test_collmap_iteration_visits_all();
    test_collmap_iteration_removal_pattern();
    test_remove_at2();
    test_iteration_visits_all();
    test_iteration_removal_pattern();
    test_empty_iteration();
    test_destroy_then_reinit();
    test_get_iter_invalid_on_missing();
    test_get_iter_consistent_with_insert();
    test_dtor_remove_calls_key_and_val_destroy();
    test_dtor_remove_with_out_val_does_not_destroy_val();
    test_dtor_insert_duplicate_destroys_old_val_and_new_key();
    test_dtor_destroy_calls_destructors_for_all_entries();
    test_dtor_remove_at_without_out_destroys_both();
    test_dtor_remove_at_with_out_does_not_destroy_val();
    test_dtor_destroy_after_removes_counts_correctly();
    test_dtor_resize_does_not_call_destructors();
    printf("All tests passed.\n");
    return 0;
}
