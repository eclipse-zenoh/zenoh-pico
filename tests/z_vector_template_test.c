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

// ── Section 1: trivially destructible/moveable int vector ────────────────────
//
// No custom destroy/move → TRIVIALLY_DESTRUCTIBLE and TRIVIALLY_MOVEABLE are
// defined, so destroy skips the loop and reserve uses memcpy.

#define _ZP_VECTOR_TEMPLATE_ELEM_TYPE int
#define _ZP_VECTOR_TEMPLATE_NAME intvec
#include "zenoh-pico/collections/vector_template.h"

// ── Section 2: non-trivially destructible/moveable "box" type ────────────────
//
// box_t holds a heap-allocated int. destroy must free it; move must transfer
// ownership without double-free. These exercise the element-wise paths.

typedef struct {
    int *ptr;
} box_t;

static int g_destroy_count = 0;
static int g_move_count = 0;

static void box_destroy(box_t *b) {
    if (b->ptr != NULL) {
        free(b->ptr);
        b->ptr = NULL;
        g_destroy_count++;
    }
}

// Move: transfer ownership then null the source (no destroy call on source —
// ownership is transferred, not copied).
static void box_move(box_t *dst, box_t *src) {
    *dst = *src;
    src->ptr = NULL;
    g_move_count++;
}

#define _ZP_VECTOR_TEMPLATE_ELEM_TYPE box_t
#define _ZP_VECTOR_TEMPLATE_NAME boxvec
#define _ZP_VECTOR_TEMPLATE_ELEM_DESTROY_FN_NAME(x) box_destroy(x)
#define _ZP_VECTOR_TEMPLATE_ELEM_MOVE_FN_NAME(dst, src) box_move(dst, src)
#include "zenoh-pico/collections/vector_template.h"

// ── Section 3: custom allocator ───────────────────────────────────────────────

static size_t g_alloc_count = 0;
static size_t g_free_count = 0;

static void *counted_alloc(size_t bytes) {
    g_alloc_count++;
    return malloc(bytes);
}
static void counted_free(void *ptr) {
    if (ptr != NULL) {
        g_free_count++;
    }
    free(ptr);
}

#define _ZP_VECTOR_TEMPLATE_ELEM_TYPE int
#define _ZP_VECTOR_TEMPLATE_NAME customvec
#define _ZP_VECTOR_TEMPLATE_ALLOC_FN_NAME(bytes) counted_alloc(bytes)
#define _ZP_VECTOR_TEMPLATE_FREE_FN_NAME(ptr) counted_free(ptr)
#include "zenoh-pico/collections/vector_template.h"

// ── Helpers ───────────────────────────────────────────────────────────────────

static box_t make_box(int value) {
    box_t b;
    b.ptr = (int *)malloc(sizeof(int));
    assert(b.ptr != NULL);
    *b.ptr = value;
    return b;
}

// ── int vector tests ──────────────────────────────────────────────────────────

static void test_new_is_empty(void) {
    printf("  new vector starts empty\n");
    intvec_t v = intvec_new();
    assert(intvec_is_empty(&v));
    assert(intvec_size(&v) == 0);
    assert(intvec_capacity(&v) == 0);
    assert(intvec_data(&v) == NULL);
    assert(intvec_front(&v) == NULL);
    assert(intvec_back(&v) == NULL);
    assert(intvec_get(&v, 0) == NULL);
    intvec_destroy(&v);
}

static void test_new_with_capacity(void) {
    printf("  new_with_capacity pre-allocates without adding elements\n");
    intvec_t v;
    assert(intvec_new_with_capacity(&v, 16));
    assert(intvec_is_empty(&v));
    assert(intvec_size(&v) == 0);
    assert(intvec_capacity(&v) == 16);
    assert(intvec_data(&v) != NULL);
    intvec_destroy(&v);
    assert(v._buffer == NULL);
}

static void test_new_with_capacity_zero(void) {
    printf("  new_with_capacity(0) creates empty vector with NULL buffer\n");
    intvec_t v;
    assert(intvec_new_with_capacity(&v, 0));
    assert(intvec_is_empty(&v));
    assert(intvec_capacity(&v) == 0);
    assert(v._buffer == NULL);
    intvec_destroy(&v);
}

static void test_push_back_and_get(void) {
    printf("  push_back appends elements in order\n");
    intvec_t v = intvec_new();
    int vals[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        assert(intvec_push_back(&v, &vals[i]));
    }
    assert(intvec_size(&v) == 3);
    for (int i = 0; i < 3; i++) {
        assert(*intvec_get(&v, (size_t)i) == vals[i]);
        assert(*intvec_cget(&v, (size_t)i) == vals[i]);
    }
    intvec_destroy(&v);
}

static void test_auto_grow(void) {
    printf("  push_back grows the buffer automatically (no fixed limit)\n");
    intvec_t v = intvec_new();
    // Push 100 elements — should trigger several doublings starting from 0.
    for (int i = 0; i < 100; i++) {
        assert(intvec_push_back(&v, &i));
    }
    assert(intvec_size(&v) == 100);
    for (int i = 0; i < 100; i++) {
        assert(*intvec_get(&v, (size_t)i) == i);
    }
    assert(intvec_capacity(&v) >= 100);
    intvec_destroy(&v);
}

static void test_pop_back(void) {
    printf("  pop_back removes from the back in LIFO order\n");
    intvec_t v = intvec_new();
    int vals[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) assert(intvec_push_back(&v, &vals[i]));
    for (int i = 2; i >= 0; i--) {
        int out = -1;
        assert(intvec_pop_back(&v, &out));
        assert(out == vals[i]);
    }
    assert(intvec_is_empty(&v));
    intvec_destroy(&v);
}

static void test_pop_empty_returns_false(void) {
    printf("  pop_back on empty vector returns false\n");
    intvec_t v = intvec_new();
    int out = -1;
    assert(!intvec_pop_back(&v, &out));
    intvec_destroy(&v);
}

static void test_pop_back_null_out(void) {
    printf("  pop_back with NULL out destroys element in place\n");
    intvec_t v = intvec_new();
    int a = 42;
    assert(intvec_push_back(&v, &a));
    assert(intvec_pop_back(&v, NULL));
    assert(intvec_is_empty(&v));
    intvec_destroy(&v);
}

static void test_front_back(void) {
    printf("  front and back return correct pointers without removing elements\n");
    intvec_t v = intvec_new();
    int a = 1, b = 2, c = 3;
    assert(intvec_push_back(&v, &a));
    assert(intvec_push_back(&v, &b));
    assert(intvec_push_back(&v, &c));
    assert(*intvec_front(&v) == 1);
    assert(*intvec_back(&v) == 3);
    assert(intvec_size(&v) == 3);
    intvec_destroy(&v);
}

static void test_get_out_of_bounds(void) {
    printf("  get/cget return NULL for out-of-bounds indices\n");
    intvec_t v = intvec_new();
    int a = 42;
    assert(intvec_push_back(&v, &a));
    assert(intvec_get(&v, 0) != NULL);
    assert(intvec_get(&v, 1) == NULL);
    assert(intvec_get(&v, 100) == NULL);
    assert(intvec_cget(&v, 1) == NULL);
    intvec_destroy(&v);
}

static void test_reserve_grows_capacity(void) {
    printf("  reserve grows capacity without changing size\n");
    intvec_t v = intvec_new();
    assert(intvec_reserve(&v, 64));
    assert(intvec_capacity(&v) == 64);
    assert(intvec_size(&v) == 0);
    assert(intvec_is_empty(&v));
    // reserve with smaller value is a no-op
    assert(intvec_reserve(&v, 8));
    assert(intvec_capacity(&v) == 64);
    intvec_destroy(&v);
}

static void test_reserve_preserves_elements(void) {
    printf("  reserve preserves existing elements (uses memcpy for trivially moveable)\n");
    intvec_t v = intvec_new();
    int vals[] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++) assert(intvec_push_back(&v, &vals[i]));
    assert(intvec_reserve(&v, 100));
    assert(intvec_size(&v) == 5);
    for (int i = 0; i < 5; i++) {
        assert(*intvec_get(&v, (size_t)i) == vals[i]);
    }
    intvec_destroy(&v);
}

static void test_data_pointer(void) {
    printf("  data() returns pointer to raw buffer usable for iteration\n");
    intvec_t v = intvec_new();
    int vals[] = {5, 6, 7};
    for (int i = 0; i < 3; i++) assert(intvec_push_back(&v, &vals[i]));
    int *d = intvec_data(&v);
    assert(d != NULL);
    for (int i = 0; i < 3; i++) assert(d[i] == vals[i]);
    intvec_destroy(&v);
}

static void test_destroy_resets(void) {
    printf("  destroy resets vector to empty state\n");
    intvec_t v = intvec_new();
    for (int i = 0; i < 5; i++) assert(intvec_push_back(&v, &i));
    intvec_destroy(&v);
    assert(intvec_is_empty(&v));
    assert(intvec_size(&v) == 0);
    assert(intvec_capacity(&v) == 0);
    assert(v._buffer == NULL);
}

// ── box_t vector tests (non-trivially destructible/moveable) ─────────────────

static void test_box_push_and_read(void) {
    printf("  box: push_back stores values correctly\n");
    boxvec_t v = boxvec_new();
    for (int i = 0; i < 5; i++) {
        box_t b = make_box(i * 10);
        assert(boxvec_push_back(&v, &b));
    }
    assert(boxvec_size(&v) == 5);
    for (int i = 0; i < 5; i++) {
        assert(*boxvec_get(&v, (size_t)i)->ptr == i * 10);
    }
    g_destroy_count = 0;
    boxvec_destroy(&v);
    // destroy must have called box_destroy for each element
    assert(g_destroy_count == 5);
}

static void test_box_reserve_uses_element_wise_move(void) {
    printf("  box: reserve uses element-wise move (no memcpy), no double-free\n");
    boxvec_t v = boxvec_new();
    for (int i = 0; i < 4; i++) {
        box_t b = make_box(i + 1);
        assert(boxvec_push_back(&v, &b));
    }
    g_move_count = 0;
    // Force a grow larger than current capacity
    size_t old_cap = boxvec_capacity(&v);
    assert(boxvec_reserve(&v, old_cap + 32));
    // Each of the 4 elements must have been moved once
    assert(g_move_count == 4);
    // Values must be intact
    for (int i = 0; i < 4; i++) {
        assert(*boxvec_get(&v, (size_t)i)->ptr == i + 1);
    }
    g_destroy_count = 0;
    boxvec_destroy(&v);
    assert(g_destroy_count == 4);
}

static void test_box_auto_grow_element_wise_move(void) {
    printf("  box: auto-grow via push_back uses element-wise move\n");
    boxvec_t v = boxvec_new();
    // Push enough to trigger multiple reallocations
    const int N = 33;
    for (int i = 0; i < N; i++) {
        box_t b = make_box(i);
        assert(boxvec_push_back(&v, &b));
    }
    assert((int)boxvec_size(&v) == N);
    for (int i = 0; i < N; i++) {
        assert(*boxvec_get(&v, (size_t)i)->ptr == i);
    }
    g_destroy_count = 0;
    boxvec_destroy(&v);
    assert(g_destroy_count == N);
}

static void test_box_pop_back_moves_out(void) {
    printf("  box: pop_back moves element into out without double-free\n");
    boxvec_t v = boxvec_new();
    box_t b = make_box(42);
    assert(boxvec_push_back(&v, &b));
    box_t out = {NULL};
    g_destroy_count = 0;
    assert(boxvec_pop_back(&v, &out));
    assert(out.ptr != NULL);
    assert(*out.ptr == 42);
    // The element was moved out: destroy should not have been called yet
    assert(g_destroy_count == 0);
    box_destroy(&out);
    assert(g_destroy_count == 1);
    boxvec_destroy(&v);
}

static void test_box_pop_back_null_destroys(void) {
    printf("  box: pop_back with NULL out calls destroy on element\n");
    boxvec_t v = boxvec_new();
    box_t b = make_box(7);
    assert(boxvec_push_back(&v, &b));
    g_destroy_count = 0;
    assert(boxvec_pop_back(&v, NULL));
    assert(g_destroy_count == 1);
    boxvec_destroy(&v);
}

// ── custom allocator tests ────────────────────────────────────────────────────

static void test_custom_alloc_called(void) {
    printf("  custom alloc/free hooks are invoked on grow and destroy\n");
    g_alloc_count = 0;
    g_free_count = 0;

    customvec_t v = customvec_new();
    assert(v._buffer == NULL);
    assert(g_alloc_count == 0);

    // First push triggers allocation
    int x = 1;
    assert(customvec_push_back(&v, &x));
    size_t allocs_after_first = g_alloc_count;
    assert(allocs_after_first >= 1);

    // Push until a second allocation (grow)
    for (int i = 1; i < 100; i++) {
        assert(customvec_push_back(&v, &i));
    }
    assert(g_alloc_count > allocs_after_first);

    size_t frees_before_destroy = g_free_count;
    customvec_destroy(&v);
    assert(g_free_count == frees_before_destroy + 1);
    assert(v._buffer == NULL);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main(void) {
    printf("=== int vector (trivially destructible + moveable) ===\n");
    test_new_is_empty();
    test_new_with_capacity();
    test_new_with_capacity_zero();
    test_push_back_and_get();
    test_auto_grow();
    test_pop_back();
    test_pop_empty_returns_false();
    test_pop_back_null_out();
    test_front_back();
    test_get_out_of_bounds();
    test_reserve_grows_capacity();
    test_reserve_preserves_elements();
    test_data_pointer();
    test_destroy_resets();

    printf("\n=== box_t vector (non-trivially destructible/moveable) ===\n");
    test_box_push_and_read();
    test_box_reserve_uses_element_wise_move();
    test_box_auto_grow_element_wise_move();
    test_box_pop_back_moves_out();
    test_box_pop_back_null_destroys();

    printf("\n=== custom allocator ===\n");
    test_custom_alloc_called();

    printf("\nAll vector_template tests passed.\n");
    return 0;
}
