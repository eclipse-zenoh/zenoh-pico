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

#include "zenoh-pico/collections/algorithms_template.h"

// ── Instantiate int vector, capacity 8 ───────────────────────────────────────

#define _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE int
#define _ZP_STATIC_VECTOR_TEMPLATE_NAME intvec
#define _ZP_STATIC_VECTOR_TEMPLATE_SIZE 8
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

// ── algorithms_template.h tests ───────────────────────────────────────────────

static void test_foreach(void) {
    printf("Test: _ZP_FOREACH visits every element in order\n");
    intvec_t v = intvec_new();
    int sum = 0;
    for (int i = 1; i <= 6; i++) {
        assert(intvec_push_back(&v, &i));
        sum += i;
    }
    int *elem;
    int got_sum = 0;
    _ZP_FOREACH(intvec, &v, elem) { got_sum += *elem; }
    assert(got_sum == sum);

    int expected = 1;
    _ZP_FOREACH(intvec, &v, elem) {
        assert(*elem == expected);
        expected++;
    }
    intvec_destroy(&v);
}

static void test_cforeach(void) {
    printf("Test: _ZP_CFOREACH visits every element via const pointer\n");
    intvec_t v = intvec_new();
    for (int i = 0; i < 5; i++) {
        assert(intvec_push_back(&v, &i));
    }
    const int *elem;
    int idx = 0;
    _ZP_CFOREACH(intvec, &v, elem) {
        assert(*elem == idx);
        idx++;
    }
    assert(idx == 5);
    intvec_destroy(&v);
}

static void test_find(void) {
    printf("Test: _ZP_FIND locates matching element, returns NULL when absent\n");
    intvec_t v = intvec_new();
    for (int i = 0; i < 8; i++) {
        assert(intvec_push_back(&v, &i));
    }
    const int *found;
#define pred_eq5(e) (*(e) == 5)
    _ZP_FIND(intvec, &v, found, pred_eq5);
#undef pred_eq5
    assert(found != NULL && *found == 5);

#define pred_eq99(e) (*(e) == 99)
    _ZP_FIND(intvec, &v, found, pred_eq99);
#undef pred_eq99
    assert(found == NULL);

    intvec_t empty = intvec_new();
#define pred_eq0(e) (*(e) == 0)
    _ZP_FIND(intvec, &empty, found, pred_eq0);
#undef pred_eq0
    assert(found == NULL);
    intvec_destroy(&empty);
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
    test_foreach();
    test_cforeach();
    test_find();
    printf("All vector tests passed.\n");
    return 0;
}
