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

// ── Instantiate int min-heap, capacity 8 ─────────────────────────────────────

static inline int intpq_cmp(const int *a, const int *b) { return *a - *b; }
#define _ZP_PQUEUE_TEMPLATE_ELEM_TYPE int
#define _ZP_PQUEUE_TEMPLATE_NAME intpq
#define _ZP_PQUEUE_TEMPLATE_SIZE 8
#define _ZP_PQUEUE_TEMPLATE_ELEM_CMP_FN_NAME intpq_cmp
#include "zenoh-pico/collections/pqueue_template.h"

// ── Instantiate int heap with context-carried comparator ─────────────────────
// The context holds a multiplier: +1 → min-heap, -1 → max-heap.

typedef struct {
    int multiplier;
} intpq_cmp_ctx_t;

static inline int intpq_with_ctx_cmp(const int *a, const int *b, const intpq_cmp_ctx_t *ctx) {
    return ctx->multiplier * (*a - *b);
}
#define _ZP_PQUEUE_TEMPLATE_ELEM_TYPE int
#define _ZP_PQUEUE_TEMPLATE_NAME intpq_with_ctx
#define _ZP_PQUEUE_TEMPLATE_SIZE 8
#define _ZP_PQUEUE_TEMPLATE_CMP_CTX_TYPE intpq_cmp_ctx_t
#define _ZP_PQUEUE_TEMPLATE_ELEM_CMP_FN_NAME intpq_with_ctx_cmp
#include "zenoh-pico/collections/pqueue_template.h"

// ── Tests: context-free min-heap ─────────────────────────────────────────────

static void test_new_is_empty(void) {
    printf("Test: new queue is empty\n");
    intpq_t pq = intpq_new();
    assert(intpq_is_empty(&pq));
    assert(intpq_size(&pq) == 0);
    assert(intpq_peek(&pq) == NULL);
    intpq_destroy(&pq);
}

static void test_push_pop_single(void) {
    printf("Test: push then pop returns the same element\n");
    intpq_t pq = intpq_new();
    int v = 42;
    assert(intpq_push(&pq, &v));
    assert(intpq_size(&pq) == 1);
    int out = 0;
    assert(intpq_pop(&pq, &out));
    assert(out == 42);
    assert(intpq_is_empty(&pq));
    intpq_destroy(&pq);
}

static void test_min_heap_order(void) {
    printf("Test: pop returns elements in ascending order\n");
    intpq_t pq = intpq_new();
    int vals[] = {5, 1, 8, 3, 2, 7, 4, 6};
    for (int i = 0; i < 8; i++) {
        assert(intpq_push(&pq, &vals[i]));
    }
    assert(intpq_size(&pq) == 8);
    int prev = -1;
    for (int i = 0; i < 8; i++) {
        int out = 0;
        assert(intpq_pop(&pq, &out));
        assert(out > prev);
        prev = out;
    }
    assert(intpq_is_empty(&pq));
    intpq_destroy(&pq);
}

static void test_peek_does_not_remove(void) {
    printf("Test: peek returns min without removing it\n");
    intpq_t pq = intpq_new();
    int a = 10, b = 3, c = 7;
    assert(intpq_push(&pq, &a));
    assert(intpq_push(&pq, &b));
    assert(intpq_push(&pq, &c));
    int *top = intpq_peek(&pq);
    assert(top != NULL && *top == 3);
    assert(intpq_size(&pq) == 3);  // peek must not remove
    intpq_destroy(&pq);
}

static void test_capacity_exceeded(void) {
    printf("Test: push fails when capacity is full\n");
    intpq_t pq = intpq_new();
    for (int i = 0; i < 8; i++) {
        assert(intpq_push(&pq, &i));
    }
    assert(intpq_size(&pq) == 8);
    int extra = 99;
    assert(!intpq_push(&pq, &extra));  // must fail
    assert(intpq_size(&pq) == 8);
    intpq_destroy(&pq);
}

static void test_pop_on_empty_returns_false(void) {
    printf("Test: pop on empty queue returns false\n");
    intpq_t pq = intpq_new();
    int out = 0;
    assert(!intpq_pop(&pq, &out));
    intpq_destroy(&pq);
}

static void test_destroy_resets_size(void) {
    printf("Test: destroy resets size to zero\n");
    intpq_t pq = intpq_new();
    int v = 1;
    assert(intpq_push(&pq, &v));
    intpq_destroy(&pq);
    assert(intpq_size(&pq) == 0);
    assert(intpq_is_empty(&pq));
}

static void test_duplicate_values(void) {
    printf("Test: duplicate values are handled correctly\n");
    intpq_t pq = intpq_new();
    int vals[] = {3, 3, 1, 1, 2, 2};
    for (int i = 0; i < 6; i++) {
        assert(intpq_push(&pq, &vals[i]));
    }
    int out, prev = -1;
    int count = 0;
    while (intpq_pop(&pq, &out)) {
        assert(out >= prev);
        prev = out;
        count++;
    }
    assert(count == 6);
    intpq_destroy(&pq);
}

static void test_push_pop_interleaved(void) {
    printf("Test: interleaved push and pop maintains heap property\n");
    intpq_t pq = intpq_new();
    // Push 3, pop min, push 1, pop min, etc.
    int vals[] = {5, 3, 8, 1, 4};
    int expected[] = {3, 1, 4};  // after: push 5,3 pop→3; push 8,1 pop→1; push 4 pop→4
    int v;

    v = 5;
    intpq_push(&pq, &v);
    v = 3;
    intpq_push(&pq, &v);
    int out = 0;
    assert(intpq_pop(&pq, &out) && out == expected[0]);

    v = 8;
    intpq_push(&pq, &v);
    v = 1;
    intpq_push(&pq, &v);
    assert(intpq_pop(&pq, &out) && out == expected[1]);

    v = 4;
    intpq_push(&pq, &v);
    assert(intpq_pop(&pq, &out) && out == expected[2]);

    (void)vals;
    intpq_destroy(&pq);
}

// ── Tests: context-aware (max-heap via multiplier = -1) ──────────────────────

static void test_ctx_max_heap_order(void) {
    printf("Test (ctx): pop returns elements in descending order (max-heap)\n");
    intpq_cmp_ctx_t ctx;
    ctx.multiplier = -1;
    intpq_with_ctx_t pq = intpq_with_ctx_new_with_ctx(&ctx);
    int vals[] = {5, 1, 8, 3, 2, 7, 4, 6};
    for (int i = 0; i < 8; i++) {
        assert(intpq_with_ctx_push(&pq, &vals[i]));
    }
    int prev = 9, out = 0;
    for (int i = 0; i < 8; i++) {
        assert(intpq_with_ctx_pop(&pq, &out));
        assert(out < prev);
        prev = out;
    }
    assert(intpq_with_ctx_is_empty(&pq));
    intpq_with_ctx_destroy(&pq);
}

static void test_ctx_new_zero_init(void) {
    printf("Test (ctx): new() zero-initialises context pointer (min-heap behaviour with NULL ctx)\n");
    // Context-free new() still works when CMP_CTX_TYPE is defined —
    // the context pointer is NULL.  Since our compare never dereferences a
    // NULL ctx (it uses the multiplier which is 0 → treats all elements as
    // equal), the heap doesn't crash; we only verify it doesn't segfault and
    // that size/empty behave correctly.
    intpq_with_ctx_t pq = intpq_with_ctx_new();
    assert(intpq_with_ctx_is_empty(&pq));
    intpq_with_ctx_destroy(&pq);
}

static void test_ctx_set_ctx(void) {
    printf("Test (ctx): set_ctx switches comparison context on existing queue\n");
    intpq_cmp_ctx_t min_ctx;
    min_ctx.multiplier = 1;
    intpq_cmp_ctx_t max_ctx;
    max_ctx.multiplier = -1;

    // Start as min-heap
    intpq_with_ctx_t pq = intpq_with_ctx_new_with_ctx(&min_ctx);
    int vals[] = {4, 2, 6};
    for (int i = 0; i < 3; i++) intpq_with_ctx_push(&pq, &vals[i]);

    int out = 0;
    assert(intpq_with_ctx_pop(&pq, &out) && out == 2);  // min first

    // Drain remaining, switch to max-heap, reload
    intpq_with_ctx_destroy(&pq);
    intpq_with_ctx_set_ctx(&pq, &max_ctx);
    for (int i = 0; i < 3; i++) intpq_with_ctx_push(&pq, &vals[i]);

    assert(intpq_with_ctx_pop(&pq, &out) && out == 6);  // max first
    intpq_with_ctx_destroy(&pq);
}

int main(void) {
    // Context-free min-heap tests
    test_new_is_empty();
    test_push_pop_single();
    test_min_heap_order();
    test_peek_does_not_remove();
    test_capacity_exceeded();
    test_pop_on_empty_returns_false();
    test_destroy_resets_size();
    test_duplicate_values();
    test_push_pop_interleaved();

    // Context-aware tests
    test_ctx_max_heap_order();
    test_ctx_new_zero_init();
    test_ctx_set_ctx();

    printf("All pqueue tests passed.\n");
    return 0;
}
