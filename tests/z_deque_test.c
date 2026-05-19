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

// ── Instantiate int deque, capacity 8 ────────────────────────────────────────

#define _ZP_STATIC_DEQUE_TEMPLATE_ELEM_TYPE int
#define _ZP_STATIC_DEQUE_TEMPLATE_NAME intdeque
#define _ZP_STATIC_DEQUE_TEMPLATE_SIZE 8
#include "zenoh-pico/collections/static_deque_template.h"

// ── Helpers ───────────────────────────────────────────────────────────────────

static void test_new_is_empty(void) {
    printf("Test: new deque is empty\n");
    intdeque_t d = intdeque_new();
    assert(intdeque_is_empty(&d));
    assert(intdeque_size(&d) == 0);
    assert(intdeque_front(&d) == NULL);
    assert(intdeque_back(&d) == NULL);
    intdeque_destroy(&d);
}

static void test_push_back_pop_front(void) {
    printf("Test: push_back then pop_front preserves FIFO order\n");
    intdeque_t d = intdeque_new();
    int vals[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        assert(intdeque_push_back(&d, &vals[i]));
    }
    assert(intdeque_size(&d) == 3);
    for (int i = 0; i < 3; i++) {
        int out = -1;
        assert(intdeque_pop_front(&d, &out));
        assert(out == vals[i]);
    }
    assert(intdeque_is_empty(&d));
    intdeque_destroy(&d);
}

static void test_push_front_pop_front(void) {
    printf("Test: push_front then pop_front preserves stack (LIFO) order\n");
    intdeque_t d = intdeque_new();
    int vals[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        assert(intdeque_push_front(&d, &vals[i]));
    }
    assert(intdeque_size(&d) == 3);
    for (int i = 2; i >= 0; i--) {
        int out = -1;
        assert(intdeque_pop_front(&d, &out));
        assert(out == vals[i]);
    }
    assert(intdeque_is_empty(&d));
    intdeque_destroy(&d);
}

static void test_push_back_pop_back(void) {
    printf("Test: push_back then pop_back preserves stack (LIFO) order\n");
    intdeque_t d = intdeque_new();
    int vals[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        assert(intdeque_push_back(&d, &vals[i]));
    }
    for (int i = 2; i >= 0; i--) {
        int out = -1;
        assert(intdeque_pop_back(&d, &out));
        assert(out == vals[i]);
    }
    assert(intdeque_is_empty(&d));
    intdeque_destroy(&d);
}

static void test_push_front_pop_back(void) {
    printf("Test: push_front then pop_back preserves FIFO order\n");
    intdeque_t d = intdeque_new();
    int vals[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        assert(intdeque_push_front(&d, &vals[i]));
    }
    for (int i = 0; i < 3; i++) {
        int out = -1;
        assert(intdeque_pop_back(&d, &out));
        assert(out == vals[i]);
    }
    assert(intdeque_is_empty(&d));
    intdeque_destroy(&d);
}

static void test_front_back_peek(void) {
    printf("Test: front and back return correct pointers without removing\n");
    intdeque_t d = intdeque_new();
    int a = 1, b = 2, c = 3;
    assert(intdeque_push_back(&d, &a));
    assert(intdeque_push_back(&d, &b));
    assert(intdeque_push_back(&d, &c));
    assert(*intdeque_front(&d) == 1);
    assert(*intdeque_back(&d) == 3);
    assert(intdeque_size(&d) == 3);
    intdeque_destroy(&d);
}

static void test_capacity_full(void) {
    printf("Test: push fails when deque is full (capacity 8)\n");
    intdeque_t d = intdeque_new();
    int v = 0;
    for (int i = 0; i < 8; i++) {
        v = i;
        assert(intdeque_push_back(&d, &v));
    }
    assert(intdeque_size(&d) == 8);
    // Next push should fail
    v = 99;
    assert(!intdeque_push_back(&d, &v));
    assert(!intdeque_push_front(&d, &v));
    intdeque_destroy(&d);
}

static void test_pop_empty(void) {
    printf("Test: pop on empty deque returns false\n");
    intdeque_t d = intdeque_new();
    int out = -1;
    assert(!intdeque_pop_front(&d, &out));
    assert(!intdeque_pop_back(&d, &out));
    intdeque_destroy(&d);
}

static void test_wrap_around_back(void) {
    printf("Test: circular wrap-around with push_back/pop_front\n");
    intdeque_t d = intdeque_new();
    // Fill to 4, drain 4, then fill 8 — this forces wrap-around
    int v;
    for (int i = 0; i < 4; i++) {
        v = i;
        assert(intdeque_push_back(&d, &v));
    }
    for (int i = 0; i < 4; i++) {
        int out;
        assert(intdeque_pop_front(&d, &out));
        assert(out == i);
    }
    assert(intdeque_is_empty(&d));
    for (int i = 0; i < 8; i++) {
        v = 100 + i;
        assert(intdeque_push_back(&d, &v));
    }
    assert(intdeque_size(&d) == 8);
    for (int i = 0; i < 8; i++) {
        int out;
        assert(intdeque_pop_front(&d, &out));
        assert(out == 100 + i);
    }
    assert(intdeque_is_empty(&d));
    intdeque_destroy(&d);
}

static void test_wrap_around_front(void) {
    printf("Test: circular wrap-around with push_front/pop_back\n");
    intdeque_t d = intdeque_new();
    int v;
    v = 200;
    for (int i = 0; i < 8; i++) {
        v = 200 + i;
        assert(intdeque_push_front(&d, &v));  // start walks down from SIZE
    }
    // Order from front: 207,206,...,201,200
    assert(intdeque_size(&d) == 8);
    for (int i = 7; i >= 1; i--) {
        int out;
        assert(intdeque_pop_front(&d, &out));
        assert(out == 200 + i);
    }
    int out;
    assert(intdeque_pop_front(&d, &out));
    assert(out == 200);
    assert(intdeque_is_empty(&d));

    // Also verify wrap-around pop_back ordering
    v = 300;
    assert(intdeque_push_back(&d, &v));
    for (int i = 1; i < 5; i++) {
        v = 300 + i;
        assert(intdeque_push_front(&d, &v));
    }
    // front->back: 304,303,302,301,300
    assert(intdeque_size(&d) == 5);
    assert(intdeque_pop_back(&d, &out));
    assert(out == 300);
    assert(intdeque_pop_front(&d, &out));
    assert(out == 304);
    intdeque_destroy(&d);
}

static void test_mixed_push_pop(void) {
    printf("Test: interleaved push_front/push_back and pop_front/pop_back\n");
    intdeque_t d = intdeque_new();
    int a = 1, b = 2, c = 3, d_val = 4;
    // [1]
    assert(intdeque_push_back(&d, &a));
    // [2, 1]
    assert(intdeque_push_front(&d, &b));
    // [2, 1, 3]
    assert(intdeque_push_back(&d, &c));
    // [4, 2, 1, 3]
    assert(intdeque_push_front(&d, &d_val));
    assert(intdeque_size(&d) == 4);

    int out;
    // pop front: 4
    assert(intdeque_pop_front(&d, &out));
    assert(out == 4);
    // pop back: 3
    assert(intdeque_pop_back(&d, &out));
    assert(out == 3);
    // pop front: 2
    assert(intdeque_pop_front(&d, &out));
    assert(out == 2);
    // pop front: 1
    assert(intdeque_pop_front(&d, &out));
    assert(out == 1);

    assert(intdeque_is_empty(&d));
    intdeque_destroy(&d);
}

static void test_destroy_non_empty(void) {
    printf("Test: destroy on non-empty deque does not crash\n");
    intdeque_t d = intdeque_new();
    int v;
    for (int i = 0; i < 5; i++) {
        v = i;
        intdeque_push_back(&d, &v);
    }
    intdeque_destroy(&d);
    assert(intdeque_is_empty(&d));
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main(void) {
    test_new_is_empty();
    test_push_back_pop_front();
    test_push_front_pop_front();
    test_push_back_pop_back();
    test_push_front_pop_back();
    test_front_back_peek();
    test_capacity_full();
    test_pop_empty();
    test_wrap_around_back();
    test_wrap_around_front();
    test_mixed_push_pop();
    test_destroy_non_empty();
    printf("All deque tests passed.\n");
    return 0;
}
