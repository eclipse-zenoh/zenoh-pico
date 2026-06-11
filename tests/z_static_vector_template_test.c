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

#undef NDEBUG
#include <assert.h>

#include "zenoh-pico/collections/algorithms_template.h"

// ── Instantiate int vector, capacity 8 ───────────────────────────────────────

#define _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE int
#define _ZP_STATIC_VECTOR_TEMPLATE_NAME intvec
#define _ZP_STATIC_VECTOR_TEMPLATE_SIZE 8
#include "zenoh-pico/collections/static_vector_template.h"

// ── Instantiate non-trivially-movable "box" vector, capacity 8 ───────────────
//
// box_t holds a heap-allocated int. A custom move function transfers ownership
// and nulls the source, so this instantiation exercises the element-wise
// (non-memmove) fallback paths of append/insert/remove.

typedef struct {
    int *ptr;
} box_t;

static int g_box_destroy_count = 0;
static int g_box_move_count = 0;

static void box_destroy(box_t *b) {
    if (b->ptr != NULL) {
        free(b->ptr);
        b->ptr = NULL;
        g_box_destroy_count++;
    }
}

static void box_move(box_t *dst, box_t *src) {
    *dst = *src;
    src->ptr = NULL;
    g_box_move_count++;
}

static box_t make_box(int value) {
    box_t b;
    b.ptr = (int *)malloc(sizeof(int));
    assert(b.ptr != NULL);
    *b.ptr = value;
    return b;
}

#define _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE box_t
#define _ZP_STATIC_VECTOR_TEMPLATE_NAME boxvec
#define _ZP_STATIC_VECTOR_TEMPLATE_SIZE 8
#define _ZP_STATIC_VECTOR_TEMPLATE_ELEM_DESTROY_FN(x) box_destroy(x)
#define _ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN(dst, src) box_move(dst, src)
#include "zenoh-pico/collections/static_vector_template.h"

// ── Tests ─────────────────────────────────────────────────────────────────────

static void test_new_is_empty(void) {
    printf("Test: new vector is empty\n");
    intvec_t v = intvec_new();
    assert(intvec_is_empty(&v));
    assert(intvec_size(&v) == 0);
    assert(intvec_capacity(&v) == 8);
    assert(intvec_front(&v) == NULL);
    assert(intvec_back(&v) == NULL);
    assert(intvec_get(&v, 0) == NULL);
    intvec_destroy(&v);
}

static void test_push_back_and_get(void) {
    printf("Test: push_back appends elements in order\n");
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

static void test_pop_back(void) {
    printf("Test: pop_back removes from the end (LIFO order)\n");
    intvec_t v = intvec_new();
    int vals[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        assert(intvec_push_back(&v, &vals[i]));
    }
    for (int i = 2; i >= 0; i--) {
        int out = -1;
        assert(intvec_pop_back(&v, &out));
        assert(out == vals[i]);
    }
    assert(intvec_is_empty(&v));
    intvec_destroy(&v);
}

static void test_front_back_peek(void) {
    printf("Test: front and back return correct pointers without removing\n");
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

static void test_capacity_full(void) {
    printf("Test: push_back fails when vector is full (capacity 8)\n");
    intvec_t v = intvec_new();
    for (int i = 0; i < 8; i++) {
        assert(intvec_push_back(&v, &i));
    }
    assert(intvec_size(&v) == 8);
    int x = 99;
    assert(!intvec_push_back(&v, &x));
    intvec_destroy(&v);
}

static void test_pop_empty(void) {
    printf("Test: pop_back on empty vector returns false\n");
    intvec_t v = intvec_new();
    int out = -1;
    assert(!intvec_pop_back(&v, &out));
    intvec_destroy(&v);
}

static void test_append(void) {
    printf("Test: append moves a range of elements onto the back\n");
    intvec_t v = intvec_new();
    int head[] = {1, 2};
    for (int i = 0; i < 2; i++) {
        assert(intvec_push_back(&v, &head[i]));
    }
    int tail[] = {3, 4, 5};
    assert(intvec_append(&v, tail, 3));
    assert(intvec_size(&v) == 5);
    for (int i = 0; i < 5; i++) {
        assert(*intvec_get(&v, (size_t)i) == i + 1);
    }
    intvec_destroy(&v);
}

static void test_append_empty_range(void) {
    printf("Test: append with len 0 is a no-op and succeeds\n");
    intvec_t v = intvec_new();
    int a = 1;
    assert(intvec_push_back(&v, &a));
    assert(intvec_append(&v, NULL, 0));
    assert(intvec_size(&v) == 1);
    assert(*intvec_get(&v, 0) == 1);
    intvec_destroy(&v);
}

static void test_append_exact_capacity(void) {
    printf("Test: append succeeds when it exactly fills the vector\n");
    intvec_t v = intvec_new();
    int vals[] = {1, 2, 3, 4, 5, 6, 7, 8};
    assert(intvec_append(&v, vals, 8));
    assert(intvec_size(&v) == 8);
    for (int i = 0; i < 8; i++) {
        assert(*intvec_get(&v, (size_t)i) == vals[i]);
    }
    intvec_destroy(&v);
}

static void test_append_overflow(void) {
    printf("Test: append fails and leaves vector unchanged when capacity is exceeded\n");
    intvec_t v = intvec_new();
    int head[] = {1, 2, 3, 4, 5};
    assert(intvec_append(&v, head, 5));
    int tail[] = {6, 7, 8, 9};  // would need 9 slots, capacity is 8
    assert(!intvec_append(&v, tail, 4));
    assert(intvec_size(&v) == 5);
    for (int i = 0; i < 5; i++) {
        assert(*intvec_get(&v, (size_t)i) == i + 1);
    }
    intvec_destroy(&v);
}

static void test_get_out_of_bounds(void) {
    printf("Test: get returns NULL for out-of-bounds index\n");
    intvec_t v = intvec_new();
    int a = 42;
    assert(intvec_push_back(&v, &a));
    assert(intvec_get(&v, 0) != NULL);
    assert(intvec_get(&v, 1) == NULL);
    assert(intvec_get(&v, 100) == NULL);
    assert(intvec_const_get(&v, 1) == NULL);
    intvec_destroy(&v);
}

static void test_insert_at_index(void) {
    printf("Test: insert shifts elements and places value at correct index\n");
    intvec_t v = intvec_new();
    int vals[] = {1, 2, 3};
    for (int i = 0; i < 3; i++) {
        assert(intvec_push_back(&v, &vals[i]));
    }
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
    printf("Test: insert at index 0 prepends the element\n");
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
    printf("Test: insert at size is equivalent to push_back\n");
    intvec_t v = intvec_new();
    int a = 1, b = 2, c = 99;
    assert(intvec_push_back(&v, &a));
    assert(intvec_push_back(&v, &b));
    assert(intvec_insert(&v, 2, &c));
    assert(*intvec_back(&v) == 99);
    assert(intvec_size(&v) == 3);
    intvec_destroy(&v);
}

static void test_insert_out_of_bounds(void) {
    printf("Test: insert beyond size returns false\n");
    intvec_t v = intvec_new();
    int x = 1;
    assert(!intvec_insert(&v, 1, &x));  // size is 0, index 1 is out of bounds
    intvec_destroy(&v);
}

static void test_insert_full(void) {
    printf("Test: insert fails when vector is full\n");
    intvec_t v = intvec_new();
    for (int i = 0; i < 8; i++) {
        assert(intvec_push_back(&v, &i));
    }
    int x = 99;
    assert(!intvec_insert(&v, 0, &x));
    intvec_destroy(&v);
}

static void test_remove_at_index(void) {
    printf("Test: remove shifts elements and returns the correct value\n");
    intvec_t v = intvec_new();
    int vals[] = {1, 2, 3, 4};
    for (int i = 0; i < 4; i++) {
        assert(intvec_push_back(&v, &vals[i]));
    }
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

static void test_remove_front(void) {
    printf("Test: remove at index 0 removes the front element\n");
    intvec_t v = intvec_new();
    int a = 10, b = 20, c = 30;
    assert(intvec_push_back(&v, &a));
    assert(intvec_push_back(&v, &b));
    assert(intvec_push_back(&v, &c));
    int out = -1;
    assert(intvec_remove(&v, 0, &out));
    assert(out == 10);
    assert(*intvec_front(&v) == 20);
    assert(intvec_size(&v) == 2);
    intvec_destroy(&v);
}

static void test_remove_back(void) {
    printf("Test: remove at last index removes the back element\n");
    intvec_t v = intvec_new();
    int a = 10, b = 20, c = 30;
    assert(intvec_push_back(&v, &a));
    assert(intvec_push_back(&v, &b));
    assert(intvec_push_back(&v, &c));
    int out = -1;
    assert(intvec_remove(&v, 2, &out));
    assert(out == 30);
    assert(*intvec_back(&v) == 20);
    assert(intvec_size(&v) == 2);
    intvec_destroy(&v);
}

static void test_remove_out_of_bounds(void) {
    printf("Test: remove out of bounds returns false\n");
    intvec_t v = intvec_new();
    int a = 1;
    assert(intvec_push_back(&v, &a));
    int out = -1;
    assert(!intvec_remove(&v, 1, &out));
    assert(!intvec_remove(&v, 100, &out));
    intvec_destroy(&v);
}

static void test_remove_null_out(void) {
    printf("Test: remove with NULL out destroys the element in place\n");
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

static void test_pop_back_null_out(void) {
    printf("Test: pop_back with NULL out destroys the element in place\n");
    intvec_t v = intvec_new();
    int a = 42;
    assert(intvec_push_back(&v, &a));
    assert(intvec_pop_back(&v, NULL));
    assert(intvec_is_empty(&v));
    intvec_destroy(&v);
}

static void test_data_pointer(void) {
    printf("Test: data returns pointer to internal buffer\n");
    intvec_t v = intvec_new();
    int vals[] = {5, 6, 7};
    for (int i = 0; i < 3; i++) {
        assert(intvec_push_back(&v, &vals[i]));
    }
    int *d = intvec_data(&v);
    assert(d != NULL);
    for (int i = 0; i < 3; i++) {
        assert(d[i] == vals[i]);
    }
    intvec_destroy(&v);
}

static void test_destroy_non_empty(void) {
    printf("Test: destroy on non-empty vector resets it\n");
    intvec_t v = intvec_new();
    for (int i = 0; i < 5; i++) {
        assert(intvec_push_back(&v, &i));
    }
    intvec_destroy(&v);
    assert(intvec_is_empty(&v));
    assert(intvec_size(&v) == 0);
}

static void test_interleaved_insert_remove(void) {
    printf("Test: interleaved insert and remove operations\n");
    intvec_t v = intvec_new();
    int a = 1, b = 2, c = 3, d = 4;
    // Build [1, 2, 3]
    assert(intvec_push_back(&v, &a));
    assert(intvec_push_back(&v, &b));
    assert(intvec_push_back(&v, &c));
    // Insert 4 at index 1: [1, 4, 2, 3]
    assert(intvec_insert(&v, 1, &d));
    // Remove index 2 (value 2): [1, 4, 3]
    int out;
    assert(intvec_remove(&v, 2, &out));
    assert(out == 2);
    assert(intvec_size(&v) == 3);
    assert(*intvec_get(&v, 0) == 1);
    assert(*intvec_get(&v, 1) == 4);
    assert(*intvec_get(&v, 2) == 3);
    intvec_destroy(&v);
}

// ── box_t vector tests (non-trivially movable, element-wise fallback) ────────

static void test_box_append_element_wise_move(void) {
    printf("Test: box append moves each element once, transferring ownership\n");
    boxvec_t v = boxvec_new();
    box_t seed = make_box(0);
    assert(boxvec_push_back(&v, &seed));

    box_t src[3];
    for (int i = 0; i < 3; i++) {
        src[i] = make_box(i + 1);
    }
    g_box_move_count = 0;
    assert(boxvec_append(&v, src, 3));
    // Each appended element must have been moved exactly once
    assert(g_box_move_count == 3);
    // Ownership transferred: the source boxes were nulled out by box_move
    for (int i = 0; i < 3; i++) {
        assert(src[i].ptr == NULL);
    }
    assert(boxvec_size(&v) == 4);
    for (int i = 0; i < 4; i++) {
        assert(*boxvec_get(&v, (size_t)i)->ptr == i);
    }
    g_box_destroy_count = 0;
    boxvec_destroy(&v);
    assert(g_box_destroy_count == 4);
}

static void test_box_insert_element_wise_move(void) {
    printf("Test: box insert shifts elements without double-free\n");
    boxvec_t v = boxvec_new();
    for (int i = 0; i < 3; i++) {
        box_t b = make_box(i * 10);  // [0, 10, 20]
        assert(boxvec_push_back(&v, &b));
    }
    box_t mid = make_box(99);
    // Insert 99 at index 1: [0, 99, 10, 20]
    assert(boxvec_insert(&v, 1, &mid));
    assert(mid.ptr == NULL);  // ownership transferred
    assert(boxvec_size(&v) == 4);
    assert(*boxvec_get(&v, 0)->ptr == 0);
    assert(*boxvec_get(&v, 1)->ptr == 99);
    assert(*boxvec_get(&v, 2)->ptr == 10);
    assert(*boxvec_get(&v, 3)->ptr == 20);
    g_box_destroy_count = 0;
    boxvec_destroy(&v);
    assert(g_box_destroy_count == 4);
}

static void test_box_remove_element_wise_move(void) {
    printf("Test: box remove shifts elements and moves out without double-free\n");
    boxvec_t v = boxvec_new();
    for (int i = 0; i < 4; i++) {
        box_t b = make_box(i + 1);  // [1, 2, 3, 4]
        assert(boxvec_push_back(&v, &b));
    }
    // Remove index 1 (value 2): [1, 3, 4]
    box_t out = {NULL};
    g_box_destroy_count = 0;
    assert(boxvec_remove(&v, 1, &out));
    assert(out.ptr != NULL && *out.ptr == 2);
    assert(g_box_destroy_count == 0);  // moved out, not destroyed
    box_destroy(&out);
    assert(g_box_destroy_count == 1);

    assert(boxvec_size(&v) == 3);
    assert(*boxvec_get(&v, 0)->ptr == 1);
    assert(*boxvec_get(&v, 1)->ptr == 3);
    assert(*boxvec_get(&v, 2)->ptr == 4);

    // Remove with NULL out destroys the element in place
    g_box_destroy_count = 0;
    assert(boxvec_remove(&v, 0, NULL));
    assert(g_box_destroy_count == 1);
    assert(boxvec_size(&v) == 2);

    g_box_destroy_count = 0;
    boxvec_destroy(&v);
    assert(g_box_destroy_count == 2);
}

// ── algorithms_template.h tests ───────────────────────────────────────────────

static void test_foreach(void) {
    printf("Test: _ZP_FOREACH visits every element in order\n");
    intvec_t v = intvec_new();
    int sum = 0;
    for (int i = 1; i <= 6; i++) {
        assert(intvec_push_back(&v, &i));
        sum += i;
    }
    int *elem = NULL;
    int got_sum = 0;
    _ZP_FOREACH (intvec, &v, elem) {
        got_sum += *elem;
    }
    assert(got_sum == sum);

    int expected = 1;
    _ZP_FOREACH (intvec, &v, elem) {
        assert(*elem == expected);
        expected++;
    }
    intvec_destroy(&v);
}

static void test_cforeach(void) {
    printf("Test: _ZP_CONST_FOREACH visits every element via const pointer\n");
    intvec_t v = intvec_new();
    for (int i = 0; i < 5; i++) {
        assert(intvec_push_back(&v, &i));
    }
    const int *elem = NULL;
    int idx = 0;
    _ZP_CONST_FOREACH (intvec, &v, elem) {
        assert(*elem == idx);
        idx++;
    }
    assert(idx == 5);
    intvec_destroy(&v);
}

static void test_find(void) {
    printf("Test: _ZP_CONST_FIND locates matching element, returns NULL when absent\n");
    intvec_t v = intvec_new();
    for (int i = 0; i < 8; i++) {
        assert(intvec_push_back(&v, &i));
    }
    const int *found = NULL;
    _ZP_CONST_FIND(intvec, &v, found, *_ == 5);
    assert(found != NULL && *found == 5);

    _ZP_CONST_FIND(intvec, &v, found, *_ == 99);
    assert(found == NULL);

    intvec_t empty = intvec_new();
    _ZP_CONST_FIND(intvec, &empty, found, *_ == 0);
    assert(found == NULL);
    intvec_destroy(&empty);
    intvec_destroy(&v);
}

static void test_itfind(void) {
    printf("Test: _ZP_IT_FIND positions iterator on first match within [begin, end)\n");
    intvec_t v = intvec_new();
    for (int i = 0; i < 8; i++) {
        assert(intvec_push_back(&v, &i));  // value i at index i
    }

    // Found: begin lands on the matching element.
    intvec_iter_t it = intvec_begin(&v);
    intvec_iter_t end_it = intvec_end(&v);
    _ZP_IT_FIND(intvec, &v, it, end_it, *_ == 5);
    assert(it != end_it);
    assert(*intvec_at(&v, it) == 5);

    // Not found: begin advances to end.
    it = intvec_begin(&v);
    end_it = intvec_end(&v);
    _ZP_IT_FIND(intvec, &v, it, end_it, *_ == 99);
    assert(it == end_it);

    // Respects begin bound: a match before begin is not found.
    it = 4;  // skip index 2
    end_it = intvec_end(&v);
    _ZP_IT_FIND(intvec, &v, it, end_it, *_ == 2);
    assert(it == end_it);

    // Respects end bound: a match at/after end is not found.
    it = intvec_begin(&v);
    end_it = 4;  // restrict search to indices [0, 4)
    _ZP_IT_FIND(intvec, &v, it, end_it, *_ == 6);
    assert(it == end_it);

    // Positive sub-range search.
    it = 1;
    end_it = 7;
    _ZP_IT_FIND(intvec, &v, it, end_it, *_ == 4);
    assert(it == 4);
    assert(*intvec_at(&v, it) == 4);

    intvec_destroy(&v);
}

static void test_citfind(void) {
    printf("Test: _ZP_CONST_IT_FIND positions iterator on first match via const access\n");
    intvec_t v = intvec_new();
    for (int i = 0; i < 8; i++) {
        assert(intvec_push_back(&v, &i));
    }

    // Found via const access.
    intvec_iter_t it = intvec_begin(&v);
    intvec_iter_t end_it = intvec_end(&v);
    _ZP_CONST_IT_FIND(intvec, &v, it, end_it, *_ == 3);
    assert(it != end_it);
    assert(*intvec_const_at(&v, it) == 3);

    // Not found.
    it = intvec_begin(&v);
    end_it = intvec_end(&v);
    _ZP_CONST_IT_FIND(intvec, &v, it, end_it, *_ == 77);
    assert(it == end_it);

    // Empty range (begin == end) finds nothing.
    it = intvec_begin(&v);
    end_it = it;
    _ZP_CONST_IT_FIND(intvec, &v, it, end_it, true);
    assert(it == end_it);

    intvec_destroy(&v);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main(void) {
    test_new_is_empty();
    test_push_back_and_get();
    test_pop_back();
    test_front_back_peek();
    test_capacity_full();
    test_pop_empty();
    test_append();
    test_append_empty_range();
    test_append_exact_capacity();
    test_append_overflow();
    test_get_out_of_bounds();
    test_insert_at_index();
    test_insert_at_front();
    test_insert_at_end();
    test_insert_out_of_bounds();
    test_insert_full();
    test_remove_at_index();
    test_remove_front();
    test_remove_back();
    test_remove_out_of_bounds();
    test_remove_null_out();
    test_pop_back_null_out();
    test_data_pointer();
    test_destroy_non_empty();
    test_interleaved_insert_remove();
    test_box_append_element_wise_move();
    test_box_insert_element_wise_move();
    test_box_remove_element_wise_move();
    test_foreach();
    test_cforeach();
    test_find();
    test_itfind();
    test_citfind();
    printf("All vector tests passed.\n");
    return 0;
}
