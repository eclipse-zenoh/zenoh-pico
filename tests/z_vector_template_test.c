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
#define _ZP_VECTOR_TEMPLATE_ELEM_DESTROY_FN(x) box_destroy(x)
#define _ZP_VECTOR_TEMPLATE_ELEM_MOVE_FN(dst, src) box_move(dst, src)
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
#define _ZP_VECTOR_TEMPLATE_ALLOC_FN(bytes) counted_alloc(bytes)
#define _ZP_VECTOR_TEMPLATE_FREE_FN(ptr) counted_free(ptr)
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

static void test_init_with_capacity(void) {
    printf("  init_with_capacity pre-allocates without adding elements\n");
    intvec_t v;
    assert(intvec_init_with_capacity(&v, 16));
    assert(intvec_is_empty(&v));
    assert(intvec_size(&v) == 0);
    assert(intvec_capacity(&v) == 16);
    assert(intvec_data(&v) != NULL);
    intvec_destroy(&v);
    assert(v._buffer == NULL);
}

static void test_init_with_capacity_zero(void) {
    printf("  init_with_capacity(0) creates empty vector with NULL buffer\n");
    intvec_t v;
    assert(intvec_init_with_capacity(&v, 0));
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
        assert(*intvec_const_get(&v, (size_t)i) == vals[i]);
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

static void test_append(void) {
    printf("  append moves a range of elements and grows the buffer as needed\n");
    intvec_t v = intvec_new();
    int head[] = {1, 2};
    for (int i = 0; i < 2; i++) assert(intvec_push_back(&v, &head[i]));
    int tail[50];
    for (int i = 0; i < 50; i++) tail[i] = i + 3;
    assert(intvec_append(&v, tail, 50));
    assert(intvec_size(&v) == 52);
    assert(intvec_capacity(&v) >= 52);
    for (int i = 0; i < 52; i++) {
        assert(*intvec_get(&v, (size_t)i) == i + 1);
    }
    intvec_destroy(&v);
}

static void test_append_empty_range(void) {
    printf("  append with len 0 is a no-op and never allocates\n");
    intvec_t v = intvec_new();
    assert(intvec_append(&v, NULL, 0));
    assert(intvec_is_empty(&v));
    assert(intvec_capacity(&v) == 0);
    assert(intvec_data(&v) == NULL);
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

static void test_insert_at_index(void) {
    printf("  insert shifts elements and places value at correct index\n");
    intvec_t v = intvec_new();
    int vals[] = {1, 2, 3};
    for (int i = 0; i < 3; i++) assert(intvec_push_back(&v, &vals[i]));
    // Insert 99 at index 1: [1, 99, 2, 3]
    int x = 99;
    assert(intvec_insert(&v, 1, &x));
    assert(intvec_size(&v) == 4);
    assert(*intvec_get(&v, 0) == 1);
    assert(*intvec_get(&v, 1) == 99);
    assert(*intvec_get(&v, 2) == 2);
    assert(*intvec_get(&v, 3) == 3);
    intvec_destroy(&v);
}

static void test_insert_at_front(void) {
    printf("  insert at index 0 prepends the element\n");
    intvec_t v = intvec_new();
    int a = 10, b = 20, front = 5;
    assert(intvec_push_back(&v, &a));
    assert(intvec_push_back(&v, &b));
    assert(intvec_insert(&v, 0, &front));
    assert(*intvec_front(&v) == 5);
    assert(*intvec_get(&v, 1) == 10);
    assert(*intvec_get(&v, 2) == 20);
    intvec_destroy(&v);
}

static void test_insert_at_end(void) {
    printf("  insert at size is equivalent to push_back\n");
    intvec_t v = intvec_new();
    int a = 1, b = 2, c = 99;
    assert(intvec_push_back(&v, &a));
    assert(intvec_push_back(&v, &b));
    assert(intvec_insert(&v, 2, &c));
    assert(*intvec_back(&v) == 99);
    assert(intvec_size(&v) == 3);
    intvec_destroy(&v);
}

static void test_insert_grows(void) {
    printf("  insert grows the buffer when at capacity\n");
    intvec_t v = intvec_new();
    // Fill the vector exactly to a power-of-two capacity so the next insert must grow.
    for (int i = 0; i < 4; i++) assert(intvec_push_back(&v, &i));  // [0, 1, 2, 3]
    size_t cap_before = intvec_capacity(&v);
    assert(intvec_size(&v) == cap_before);
    int x = 99;
    assert(intvec_insert(&v, 2, &x));  // [0, 1, 99, 2, 3]
    assert(intvec_capacity(&v) > cap_before);
    assert(intvec_size(&v) == 5);
    int expected[] = {0, 1, 99, 2, 3};
    for (size_t i = 0; i < 5; i++) assert(*intvec_get(&v, i) == expected[i]);
    intvec_destroy(&v);
}

static void test_insert_out_of_bounds(void) {
    printf("  insert beyond size returns false\n");
    intvec_t v = intvec_new();
    int x = 1;
    assert(!intvec_insert(&v, 1, &x));  // size is 0, index 1 is out of bounds
    assert(intvec_is_empty(&v));
    intvec_destroy(&v);
}

static void test_remove_at_index(void) {
    printf("  remove shifts elements and returns the correct value\n");
    intvec_t v = intvec_new();
    int vals[] = {1, 2, 3, 4};
    for (int i = 0; i < 4; i++) assert(intvec_push_back(&v, &vals[i]));
    // Remove index 1 (value 2): [1, 3, 4]
    int out = -1;
    assert(intvec_remove(&v, 1, &out));
    assert(out == 2);
    assert(intvec_size(&v) == 3);
    assert(*intvec_get(&v, 0) == 1);
    assert(*intvec_get(&v, 1) == 3);
    assert(*intvec_get(&v, 2) == 4);
    intvec_destroy(&v);
}

static void test_remove_front_back(void) {
    printf("  remove at first and last index removes the right element\n");
    intvec_t v = intvec_new();
    int a = 10, b = 20, c = 30;
    assert(intvec_push_back(&v, &a));
    assert(intvec_push_back(&v, &b));
    assert(intvec_push_back(&v, &c));
    int out = -1;
    assert(intvec_remove(&v, 0, &out));  // remove front
    assert(out == 10);
    assert(*intvec_front(&v) == 20);
    assert(intvec_remove(&v, intvec_size(&v) - 1, &out));  // remove back
    assert(out == 30);
    assert(*intvec_back(&v) == 20);
    assert(intvec_size(&v) == 1);
    intvec_destroy(&v);
}

static void test_remove_out_of_bounds(void) {
    printf("  remove out of bounds returns false\n");
    intvec_t v = intvec_new();
    int a = 1;
    assert(intvec_push_back(&v, &a));
    int out = -1;
    assert(!intvec_remove(&v, 1, &out));
    assert(!intvec_remove(&v, 100, &out));
    assert(intvec_size(&v) == 1);
    intvec_destroy(&v);
}

static void test_remove_null_out(void) {
    printf("  remove with NULL out destroys the element in place\n");
    intvec_t v = intvec_new();
    int a = 1, b = 2, c = 3;
    assert(intvec_push_back(&v, &a));
    assert(intvec_push_back(&v, &b));
    assert(intvec_push_back(&v, &c));
    assert(intvec_remove(&v, 1, NULL));
    assert(intvec_size(&v) == 2);
    assert(*intvec_get(&v, 0) == 1);
    assert(*intvec_get(&v, 1) == 3);
    intvec_destroy(&v);
}

static void test_swap_remove_middle(void) {
    printf("  swap_remove moves the last element into the removed slot (order not preserved)\n");
    intvec_t v = intvec_new();
    int vals[] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++) assert(intvec_push_back(&v, &vals[i]));  // [1, 2, 3, 4, 5]
    // Remove index 1 (value 2): last element (5) fills the hole -> [1, 5, 3, 4]
    int out = -1;
    assert(intvec_swap_remove(&v, 1, &out));
    assert(out == 2);
    assert(intvec_size(&v) == 4);
    assert(*intvec_get(&v, 0) == 1);
    assert(*intvec_get(&v, 1) == 5);
    assert(*intvec_get(&v, 2) == 3);
    assert(*intvec_get(&v, 3) == 4);
    intvec_destroy(&v);
}

static void test_swap_remove_last(void) {
    printf("  swap_remove of the last element is a plain pop (no self-move)\n");
    intvec_t v = intvec_new();
    int vals[] = {1, 2, 3};
    for (int i = 0; i < 3; i++) assert(intvec_push_back(&v, &vals[i]));
    int out = -1;
    assert(intvec_swap_remove(&v, 2, &out));  // remove last -> [1, 2]
    assert(out == 3);
    assert(intvec_size(&v) == 2);
    assert(*intvec_get(&v, 0) == 1);
    assert(*intvec_get(&v, 1) == 2);
    intvec_destroy(&v);
}

static void test_swap_remove_single(void) {
    printf("  swap_remove of the only element empties the vector\n");
    intvec_t v = intvec_new();
    int a = 42;
    assert(intvec_push_back(&v, &a));
    int out = -1;
    assert(intvec_swap_remove(&v, 0, &out));
    assert(out == 42);
    assert(intvec_is_empty(&v));
    intvec_destroy(&v);
}

static void test_swap_remove_null_out(void) {
    printf("  swap_remove with NULL out destroys the element in place\n");
    intvec_t v = intvec_new();
    int vals[] = {1, 2, 3, 4};
    for (int i = 0; i < 4; i++) assert(intvec_push_back(&v, &vals[i]));
    assert(intvec_swap_remove(&v, 0, NULL));  // destroy 1, move 4 into slot 0 -> [4, 2, 3]
    assert(intvec_size(&v) == 3);
    assert(*intvec_get(&v, 0) == 4);
    assert(*intvec_get(&v, 1) == 2);
    assert(*intvec_get(&v, 2) == 3);
    intvec_destroy(&v);
}

static void test_swap_remove_out_of_bounds(void) {
    printf("  swap_remove out of bounds returns false\n");
    intvec_t v = intvec_new();
    int a = 1;
    assert(intvec_push_back(&v, &a));
    int out = -1;
    assert(!intvec_swap_remove(&v, 1, &out));
    assert(!intvec_swap_remove(&v, 100, &out));
    assert(intvec_size(&v) == 1);
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
    assert(intvec_const_get(&v, 1) == NULL);
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

static void test_box_append_element_wise_move(void) {
    printf("  box: append moves each element once, transferring ownership\n");
    boxvec_t v = boxvec_new();
    box_t seed = make_box(0);
    assert(boxvec_push_back(&v, &seed));
    // Reserve enough room so append itself does not trigger a grow (which would
    // move the existing seed element); this isolates the moves to the appended ones.
    assert(boxvec_reserve(&v, 6));

    box_t src[5];
    for (int i = 0; i < 5; i++) {
        src[i] = make_box(i + 1);
    }
    g_move_count = 0;
    assert(boxvec_append(&v, src, 5));
    // Each appended element must have been moved exactly once
    assert(g_move_count == 5);
    // Ownership transferred: the source boxes were nulled out by box_move
    for (int i = 0; i < 5; i++) {
        assert(src[i].ptr == NULL);
    }
    assert(boxvec_size(&v) == 6);
    for (int i = 0; i < 6; i++) {
        assert(*boxvec_get(&v, (size_t)i)->ptr == i);
    }
    g_destroy_count = 0;
    boxvec_destroy(&v);
    assert(g_destroy_count == 6);
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

static void test_box_insert_element_wise_move(void) {
    printf("  box: insert shifts elements via element-wise move, transferring ownership\n");
    boxvec_t v = boxvec_new();
    box_t seed[3];
    for (int i = 0; i < 3; i++) {
        seed[i] = make_box(i);  // [0, 1, 2]
        assert(boxvec_push_back(&v, &seed[i]));
    }
    box_t mid = make_box(99);
    assert(boxvec_insert(&v, 1, &mid));  // [0, 99, 1, 2]
    // Ownership transferred: the source box was nulled out by box_move.
    assert(mid.ptr == NULL);
    assert(boxvec_size(&v) == 4);
    int expected[] = {0, 99, 1, 2};
    for (int i = 0; i < 4; i++) {
        assert(*boxvec_get(&v, (size_t)i)->ptr == expected[i]);
    }
    g_destroy_count = 0;
    boxvec_destroy(&v);
    assert(g_destroy_count == 4);
}

static void test_box_remove_moves_out(void) {
    printf("  box: remove moves element into out without double-free\n");
    boxvec_t v = boxvec_new();
    box_t seed[3];
    for (int i = 0; i < 3; i++) {
        seed[i] = make_box(i + 1);  // [1, 2, 3]
        assert(boxvec_push_back(&v, &seed[i]));
    }
    box_t out = {NULL};
    g_destroy_count = 0;
    assert(boxvec_remove(&v, 1, &out));  // remove value 2 -> [1, 3]
    assert(out.ptr != NULL && *out.ptr == 2);
    // The element was moved out: destroy should not have been called yet.
    assert(g_destroy_count == 0);
    assert(boxvec_size(&v) == 2);
    assert(*boxvec_get(&v, 0)->ptr == 1);
    assert(*boxvec_get(&v, 1)->ptr == 3);
    box_destroy(&out);
    assert(g_destroy_count == 1);
    boxvec_destroy(&v);
}

static void test_box_remove_null_destroys(void) {
    printf("  box: remove with NULL out calls destroy on element\n");
    boxvec_t v = boxvec_new();
    box_t seed[3];
    for (int i = 0; i < 3; i++) {
        seed[i] = make_box(i + 1);  // [1, 2, 3]
        assert(boxvec_push_back(&v, &seed[i]));
    }
    g_destroy_count = 0;
    assert(boxvec_remove(&v, 0, NULL));  // destroy value 1 -> [2, 3]
    assert(g_destroy_count == 1);
    assert(boxvec_size(&v) == 2);
    assert(*boxvec_get(&v, 0)->ptr == 2);
    boxvec_destroy(&v);
}

static void test_box_swap_remove_moves_out(void) {
    printf("  box: swap_remove moves element out and relocates the last via element-wise move\n");
    boxvec_t v = boxvec_new();
    box_t seed[4];
    for (int i = 0; i < 4; i++) {
        seed[i] = make_box(i + 1);  // [1, 2, 3, 4]
        assert(boxvec_push_back(&v, &seed[i]));
    }
    box_t out = {NULL};
    g_destroy_count = 0;
    g_move_count = 0;
    assert(boxvec_swap_remove(&v, 1, &out));  // out=2, move 4 into slot 1 -> [1, 4, 3]
    // One move to extract into out, one move to relocate the last element.
    assert(g_move_count == 2);
    // The removed element was moved out, not destroyed.
    assert(g_destroy_count == 0);
    assert(out.ptr != NULL && *out.ptr == 2);
    assert(boxvec_size(&v) == 3);
    assert(*boxvec_get(&v, 0)->ptr == 1);
    assert(*boxvec_get(&v, 1)->ptr == 4);
    assert(*boxvec_get(&v, 2)->ptr == 3);
    box_destroy(&out);
    assert(g_destroy_count == 1);
    boxvec_destroy(&v);
}

static void test_box_swap_remove_last_null_destroys(void) {
    printf("  box: swap_remove of the last element with NULL out destroys it without relocation\n");
    boxvec_t v = boxvec_new();
    box_t seed[3];
    for (int i = 0; i < 3; i++) {
        seed[i] = make_box(i + 1);  // [1, 2, 3]
        assert(boxvec_push_back(&v, &seed[i]));
    }
    g_destroy_count = 0;
    g_move_count = 0;
    assert(boxvec_swap_remove(&v, 2, NULL));  // destroy last (3), no relocation -> [1, 2]
    assert(g_destroy_count == 1);
    assert(g_move_count == 0);
    assert(boxvec_size(&v) == 2);
    assert(*boxvec_get(&v, 0)->ptr == 1);
    assert(*boxvec_get(&v, 1)->ptr == 2);
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

// ── algorithms_template.h tests: heap vector (intvec) ────────────────────────

static void test_heap_foreach(void) {
    printf("  _ZP_FOREACH visits every element of heap vector\n");
    intvec_t v = intvec_new();
    int sum = 0;
    for (int i = 1; i <= 10; i++) {
        assert(intvec_push_back(&v, &i));
        sum += i;
    }
    int *elem = NULL;
    int got_sum = 0;
    _ZP_FOREACH (intvec, &v, elem) {
        got_sum += *elem;
    }
    assert(got_sum == sum);

    // Verify order
    int expected = 1;
    _ZP_FOREACH (intvec, &v, elem) {
        assert(*elem == expected);
        expected++;
    }
    intvec_destroy(&v);
}

static void test_heap_cforeach(void) {
    printf("  _ZP_CONST_FOREACH visits every element via const pointer\n");
    intvec_t v = intvec_new();
    for (int i = 0; i < 8; i++) {
        assert(intvec_push_back(&v, &i));
    }
    const int *elem = NULL;
    int idx = 0;
    _ZP_CONST_FOREACH (intvec, &v, elem) {
        assert(*elem == idx);
        idx++;
    }
    assert(idx == 8);
    intvec_destroy(&v);
}

static void test_heap_find(void) {
    printf("  _ZP_CONST_FIND locates first matching element, returns NULL when absent\n");
    intvec_t v = intvec_new();
    for (int i = 0; i < 10; i++) {
        assert(intvec_push_back(&v, &i));
    }
    const int *found = NULL;
    _ZP_CONST_FIND(intvec, &v, found, *_ == 7);
    assert(found != NULL && *found == 7);

    _ZP_CONST_FIND(intvec, &v, found, *_ == 99);
    assert(found == NULL);

    // Empty vector returns NULL
    intvec_t empty = intvec_new();
    _ZP_CONST_FIND(intvec, &empty, found, *_ == 0);
    assert(found == NULL);
    intvec_destroy(&empty);
    intvec_destroy(&v);
}

static void test_heap_itfind(void) {
    printf("  _ZP_IT_FIND positions iterator on first match within [begin, end)\n");
    intvec_t v = intvec_new();
    for (int i = 0; i < 10; i++) {
        assert(intvec_push_back(&v, &i));  // value i at index i
    }

    // Found: begin lands on the matching element.
    intvec_iter_t it = intvec_begin(&v);
    intvec_iter_t end_it = intvec_end(&v);
    _ZP_IT_FIND(intvec, &v, it, end_it, *_ == 6);
    assert(it != end_it);
    assert(*intvec_at(&v, it) == 6);

    // Not found: begin advances to end.
    it = intvec_begin(&v);
    end_it = intvec_end(&v);
    _ZP_IT_FIND(intvec, &v, it, end_it, *_ == 99);
    assert(it == end_it);

    // Respects begin bound: a match located before begin is not found.
    it = 7;  // start past index 3
    end_it = intvec_end(&v);
    _ZP_IT_FIND(intvec, &v, it, end_it, *_ == 3);
    assert(it == end_it);

    // Respects end bound: a match located at/after end is not found.
    it = intvec_begin(&v);
    end_it = 5;  // restrict search to indices [0, 5)
    _ZP_IT_FIND(intvec, &v, it, end_it, *_ == 8);
    assert(it == end_it);

    // Positive sub-range search: finds a match that lies strictly inside [begin, end).
    it = 2;
    end_it = 8;
    _ZP_IT_FIND(intvec, &v, it, end_it, *_ == 5);
    assert(it == 5);
    assert(*intvec_at(&v, it) == 5);

    intvec_destroy(&v);
}

static void test_heap_citfind(void) {
    printf("  _ZP_CONST_IT_FIND positions iterator on first match via const access\n");
    intvec_t v = intvec_new();
    for (int i = 0; i < 10; i++) {
        assert(intvec_push_back(&v, &i));
    }

    // Found via const access.
    intvec_iter_t it = intvec_begin(&v);
    intvec_iter_t end_it = intvec_end(&v);
    _ZP_CONST_IT_FIND(intvec, &v, it, end_it, *_ == 4);
    assert(it != end_it);
    assert(*intvec_const_at(&v, it) == 4);

    // Not found.
    it = intvec_begin(&v);
    end_it = intvec_end(&v);
    _ZP_CONST_IT_FIND(intvec, &v, it, end_it, *_ == 42);
    assert(it == end_it);

    // Empty range (begin == end) finds nothing.
    it = intvec_begin(&v);
    end_it = it;
    _ZP_CONST_IT_FIND(intvec, &v, it, end_it, true);
    assert(it == end_it);

    intvec_destroy(&v);
}

static void test_heap_remove_at(void) {
    printf("  remove_at removes the element and reports the next iterator\n");
    intvec_t v = intvec_new();
    for (int i = 0; i < 5; i++) assert(intvec_push_back(&v, &i));  // [0, 1, 2, 3, 4]

    // Remove a middle element: next iterator stays at the same index (tail shifted left).
    int out = -1;
    intvec_iter_t next = SIZE_MAX;
    intvec_remove_at(&v, 1, &out, &next);  // remove value 1 -> [0, 2, 3, 4]
    assert(out == 1);
    assert(next == 1);
    assert(intvec_size(&v) == 4);
    assert(*intvec_at(&v, 1) == 2);

    // Remove the last element: next iterator equals end().
    next = SIZE_MAX;
    intvec_remove_at(&v, intvec_size(&v) - 1, NULL, &next);  // remove value 4 -> [0, 2, 3]
    assert(next == intvec_end(&v));
    assert(intvec_size(&v) == 3);

    // next_idx may be NULL.
    intvec_remove_at(&v, 0, NULL, NULL);  // [2, 3]
    assert(intvec_size(&v) == 2);
    assert(*intvec_at(&v, 0) == 2);

    intvec_destroy(&v);
}

static void test_heap_zp_remove(void) {
    printf("  _ZP_REMOVE erases every element matching the predicate\n");
    intvec_t v = intvec_new();
    for (int i = 0; i < 10; i++) assert(intvec_push_back(&v, &i));  // [0..9]

    _ZP_REMOVE(intvec, &v, *_ % 2 != 0);  // drop all odd values
    assert(intvec_size(&v) == 5);
    int expected[] = {0, 2, 4, 6, 8};
    for (size_t i = 0; i < 5; i++) assert(*intvec_at(&v, i) == expected[i]);

    // Removing everything leaves an empty vector.
    _ZP_REMOVE(intvec, &v, true);
    assert(intvec_is_empty(&v));

    intvec_destroy(&v);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main(void) {
    printf("=== int vector (trivially destructible + moveable) ===\n");
    test_new_is_empty();
    test_init_with_capacity();
    test_init_with_capacity_zero();
    test_push_back_and_get();
    test_auto_grow();
    test_append();
    test_append_empty_range();
    test_pop_back();
    test_pop_empty_returns_false();
    test_pop_back_null_out();
    test_insert_at_index();
    test_insert_at_front();
    test_insert_at_end();
    test_insert_grows();
    test_insert_out_of_bounds();
    test_remove_at_index();
    test_remove_front_back();
    test_remove_out_of_bounds();
    test_remove_null_out();
    test_swap_remove_middle();
    test_swap_remove_last();
    test_swap_remove_single();
    test_swap_remove_null_out();
    test_swap_remove_out_of_bounds();
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
    test_box_append_element_wise_move();
    test_box_pop_back_moves_out();
    test_box_pop_back_null_destroys();
    test_box_insert_element_wise_move();
    test_box_remove_moves_out();
    test_box_remove_null_destroys();
    test_box_swap_remove_moves_out();
    test_box_swap_remove_last_null_destroys();

    printf("\n=== custom allocator ===\n");
    test_custom_alloc_called();

    printf("\n=== algorithms_template.h: heap vector (intvec) ===\n");
    test_heap_foreach();
    test_heap_cforeach();
    test_heap_find();
    test_heap_itfind();
    test_heap_citfind();
    test_heap_remove_at();
    test_heap_zp_remove();

    printf("\nAll vector_template tests passed.\n");
    return 0;
}
