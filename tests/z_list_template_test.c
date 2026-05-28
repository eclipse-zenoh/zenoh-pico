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

#undef NDEBUG
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// ── Instantiate a list of int ─────────────────────────────────────────────────

#define _ZP_LIST_TEMPLATE_ELEM_TYPE int
#define _ZP_LIST_TEMPLATE_NAME int_list
#include "zenoh-pico/collections/list_template.h"

// ── Instantiate a list of heap-allocated strings (non-trivial destroy/move) ──

// A simple struct whose destructor increments a global counter, giving tests
// an observable way to verify that destroy/move semantics are correct.
typedef struct {
    int value;
    int *destroy_count;  // points to a test-local counter
} counted_t;

static int g_destroy_count = 0;

static void counted_destroy(counted_t *p) {
    if (p->destroy_count != NULL) {
        (*p->destroy_count)++;
    }
}
static void counted_move(counted_t *dst, counted_t *src) {
    *dst = *src;
    src->destroy_count = NULL;  // prevent double-count on source destruction
}

#define _ZP_LIST_TEMPLATE_ELEM_TYPE counted_t
#define _ZP_LIST_TEMPLATE_NAME counted_list
#define _ZP_LIST_TEMPLATE_ELEM_DESTROY_FN(p) counted_destroy(p)
#define _ZP_LIST_TEMPLATE_ELEM_MOVE_FN(dst, src) counted_move(dst, src)
#include "zenoh-pico/collections/list_template.h"

// ── Instantiate a list of int with a custom tracking allocator ───────────────

typedef struct {
    int alloc_count;
    int free_count;
} tracking_alloc_ctx_t;

static void *tracking_alloc(tracking_alloc_ctx_t *ctx, size_t bytes) {
    ctx->alloc_count++;
    return malloc(bytes);
}
static void tracking_free(tracking_alloc_ctx_t *ctx, void *ptr) {
    ctx->free_count++;
    free(ptr);
}

#define _ZP_LIST_TEMPLATE_ELEM_TYPE int
#define _ZP_LIST_TEMPLATE_NAME int_alloc_list
#define _ZP_LIST_TEMPLATE_ALLOC_CTX_TYPE tracking_alloc_ctx_t
#define _ZP_LIST_TEMPLATE_ALLOC_FN(ctx, bytes) tracking_alloc(ctx, bytes)
#define _ZP_LIST_TEMPLATE_FREE_FN(ctx, ptr) tracking_free(ctx, ptr)
#include "zenoh-pico/collections/list_template.h"

static counted_t make_counted(int value, int *counter) {
    counted_t c;
    c.value = value;
    c.destroy_count = counter;
    return c;
}

// ── Tests: int list ───────────────────────────────────────────────────────────

static void test_int_list_new_empty(void) {
    int_list_t list = int_list_new();
    assert(int_list_size(&list) == 0);
    assert(int_list_is_empty(&list));
    assert(int_list_front(&list) == NULL);
    assert(int_list_back(&list) == NULL);
    assert(int_list_iter_begin(&list) == _ZP_LIST_ITER_INVALID);
    assert(int_list_iter_rbegin(&list) == _ZP_LIST_ITER_INVALID);
    int_list_destroy(&list);
}

static void test_int_list_push_back(void) {
    int_list_t list = int_list_new();

    int v1 = 10, v2 = 20, v3 = 30;
    assert(int_list_push_back(&list, &v1));
    assert(int_list_push_back(&list, &v2));
    assert(int_list_push_back(&list, &v3));

    assert(int_list_size(&list) == 3);
    assert(*int_list_front(&list) == 10);
    assert(*int_list_back(&list) == 30);

    int_list_destroy(&list);
}

static void test_int_list_push_front(void) {
    int_list_t list = int_list_new();

    int v1 = 10, v2 = 20, v3 = 30;
    assert(int_list_push_front(&list, &v1));
    assert(int_list_push_front(&list, &v2));
    assert(int_list_push_front(&list, &v3));

    assert(int_list_size(&list) == 3);
    assert(*int_list_front(&list) == 30);
    assert(*int_list_back(&list) == 10);

    int_list_destroy(&list);
}

static void test_int_list_pop_back(void) {
    int_list_t list = int_list_new();
    int v1 = 1, v2 = 2, v3 = 3;
    int_list_push_back(&list, &v1);
    int_list_push_back(&list, &v2);
    int_list_push_back(&list, &v3);

    int out = 0;
    assert(int_list_pop_back(&list, &out));
    assert(out == 3);
    assert(int_list_size(&list) == 2);

    assert(int_list_pop_back(&list, &out));
    assert(out == 2);

    assert(int_list_pop_back(&list, &out));
    assert(out == 1);

    assert(!int_list_pop_back(&list, &out));  // empty
    assert(int_list_is_empty(&list));

    int_list_destroy(&list);
}

static void test_int_list_pop_front(void) {
    int_list_t list = int_list_new();
    int v1 = 1, v2 = 2, v3 = 3;
    int_list_push_back(&list, &v1);
    int_list_push_back(&list, &v2);
    int_list_push_back(&list, &v3);

    int out = 0;
    assert(int_list_pop_front(&list, &out));
    assert(out == 1);
    assert(int_list_size(&list) == 2);

    assert(int_list_pop_front(&list, &out));
    assert(out == 2);

    assert(int_list_pop_front(&list, &out));
    assert(out == 3);

    assert(!int_list_pop_front(&list, &out));  // empty
    assert(int_list_is_empty(&list));

    int_list_destroy(&list);
}

static void test_int_list_pop_discard(void) {
    // pop_back / pop_front with NULL out should just destroy in place (no-op for int)
    int_list_t list = int_list_new();
    int v1 = 42, v2 = 99;
    int_list_push_back(&list, &v1);
    int_list_push_back(&list, &v2);

    assert(int_list_pop_front(&list, NULL));
    assert(int_list_size(&list) == 1);
    assert(*int_list_front(&list) == 99);

    assert(int_list_pop_back(&list, NULL));
    assert(int_list_is_empty(&list));

    int_list_destroy(&list);
}

static void test_int_list_insert_after(void) {
    int_list_t list = int_list_new();
    int v1 = 1, v2 = 3, vmid = 2;

    int_list_push_back(&list, &v1);
    int_list_push_back(&list, &v2);

    // Insert 2 after the node holding 1 (head)
    int_list_iter_t head = int_list_iter_begin(&list);
    assert(head != _ZP_LIST_ITER_INVALID);
    assert(int_list_insert_after(&list, head, &vmid));

    assert(int_list_size(&list) == 3);

    // Verify order: 1 -> 2 -> 3
    int expected[] = {1, 2, 3};
    int idx = 0;
    for (int_list_iter_t it = int_list_iter_begin(&list); it != _ZP_LIST_ITER_INVALID; it = int_list_iter_next(it)) {
        assert(it->value == expected[idx++]);
    }
    assert(idx == 3);

    int_list_destroy(&list);
}

static void test_int_list_insert_before(void) {
    int_list_t list = int_list_new();
    int v1 = 1, v2 = 3, vmid = 2;

    int_list_push_back(&list, &v1);
    int_list_push_back(&list, &v2);

    // Insert 2 before the node holding 3 (tail)
    int_list_iter_t tail = int_list_iter_rbegin(&list);
    assert(tail != _ZP_LIST_ITER_INVALID);
    assert(int_list_insert_before(&list, tail, &vmid));

    assert(int_list_size(&list) == 3);

    int expected[] = {1, 2, 3};
    int idx = 0;
    for (int_list_iter_t it = int_list_iter_begin(&list); it != _ZP_LIST_ITER_INVALID; it = int_list_iter_next(it)) {
        assert(it->value == expected[idx++]);
    }
    assert(idx == 3);

    int_list_destroy(&list);
}

static void test_int_list_remove_mid(void) {
    int_list_t list = int_list_new();
    int v1 = 1, v2 = 2, v3 = 3;
    int_list_push_back(&list, &v1);
    int_list_push_back(&list, &v2);
    int_list_push_back(&list, &v3);

    // Find node with value 2 and remove it
    int_list_iter_t it = int_list_iter_begin(&list);
    it = int_list_iter_next(it);  // advance to node 2
    assert(it != _ZP_LIST_ITER_INVALID);
    assert(it->value == 2);

    int out = 0;
    int_list_remove(&list, it, &out);
    assert(out == 2);
    assert(int_list_size(&list) == 2);
    assert(*int_list_front(&list) == 1);
    assert(*int_list_back(&list) == 3);

    int_list_destroy(&list);
}

static void test_int_list_forward_iteration(void) {
    int_list_t list = int_list_new();
    for (int i = 1; i <= 5; i++) {
        int_list_push_back(&list, &i);
    }

    int expected = 1;
    for (int_list_iter_t it = int_list_iter_begin(&list); it != _ZP_LIST_ITER_INVALID; it = int_list_iter_next(it)) {
        assert(it->value == expected++);
    }
    assert(expected == 6);

    int_list_destroy(&list);
}

static void test_int_list_reverse_iteration(void) {
    int_list_t list = int_list_new();
    for (int i = 1; i <= 5; i++) {
        int_list_push_back(&list, &i);
    }

    int expected = 5;
    for (int_list_iter_t it = int_list_iter_rbegin(&list); it != _ZP_LIST_ITER_INVALID; it = int_list_iter_prev(it)) {
        assert(it->value == expected--);
    }
    assert(expected == 0);

    int_list_destroy(&list);
}

static void test_int_list_const_iteration(void) {
    int_list_t list = int_list_new();
    int v1 = 10, v2 = 20;
    int_list_push_back(&list, &v1);
    int_list_push_back(&list, &v2);

    const int_list_t *clist = &list;
    int expected[] = {10, 20};
    int idx = 0;
    for (int_list_citer_t it = int_list_iter_cbegin(clist); it != _ZP_LIST_ITER_INVALID; it = int_list_iter_cnext(it)) {
        assert(it->value == expected[idx++]);
    }
    assert(idx == 2);

    int_list_destroy(&list);
}

static void test_int_list_insert_after_null(void) {
    // insert_after(NULL) should prepend to front
    int_list_t list = int_list_new();
    int v1 = 2, v2 = 1;
    int_list_push_back(&list, &v1);
    int_list_insert_after(&list, _ZP_LIST_ITER_INVALID, &v2);

    assert(int_list_size(&list) == 2);
    assert(*int_list_front(&list) == 1);
    assert(*int_list_back(&list) == 2);

    int_list_destroy(&list);
}

static void test_int_list_insert_before_null(void) {
    // insert_before(NULL) should append to back
    int_list_t list = int_list_new();
    int v1 = 1, v2 = 2;
    int_list_push_back(&list, &v1);
    int_list_insert_before(&list, _ZP_LIST_ITER_INVALID, &v2);

    assert(int_list_size(&list) == 2);
    assert(*int_list_front(&list) == 1);
    assert(*int_list_back(&list) == 2);

    int_list_destroy(&list);
}

// ── Tests: counted_list (observable destroy side-effects) ────────────────────

static void test_counted_list_destroy_calls_destructor(void) {
    int count = 0;
    counted_list_t list = counted_list_new();

    counted_t a = make_counted(1, &count);
    counted_t b = make_counted(2, &count);
    counted_t c = make_counted(3, &count);
    counted_list_push_back(&list, &a);
    counted_list_push_back(&list, &b);
    counted_list_push_back(&list, &c);

    assert(count == 0);
    counted_list_destroy(&list);
    assert(count == 3);  // all three elements destroyed
}

static void test_counted_list_pop_back_moves_out(void) {
    int count = 0;
    counted_list_t list = counted_list_new();

    counted_t a = make_counted(42, &count);
    counted_list_push_back(&list, &a);

    counted_t out;
    assert(counted_list_pop_back(&list, &out));
    assert(count == 0);  // not destroyed, moved out
    assert(out.value == 42);
    assert(out.destroy_count == &count);

    // Simulate caller cleaning up the moved-out value
    counted_destroy(&out);
    assert(count == 1);

    counted_list_destroy(&list);
}

static void test_counted_list_pop_back_null_destroys(void) {
    int count = 0;
    counted_list_t list = counted_list_new();

    counted_t a = make_counted(7, &count);
    counted_list_push_back(&list, &a);

    assert(counted_list_pop_back(&list, NULL));
    assert(count == 1);  // destroyed in place

    counted_list_destroy(&list);
}

static void test_counted_list_pop_front_moves_out(void) {
    int count = 0;
    counted_list_t list = counted_list_new();

    counted_t a = make_counted(10, &count);
    counted_t b = make_counted(20, &count);
    counted_list_push_back(&list, &a);
    counted_list_push_back(&list, &b);

    counted_t out;
    assert(counted_list_pop_front(&list, &out));
    assert(count == 0);
    assert(out.value == 10);

    counted_destroy(&out);
    assert(count == 1);

    counted_list_destroy(&list);
    assert(count == 2);  // remaining element (20) destroyed
}

static void test_counted_list_remove_mid_moves_out(void) {
    int count = 0;
    counted_list_t list = counted_list_new();

    counted_t a = make_counted(1, &count);
    counted_t b = make_counted(2, &count);
    counted_t c = make_counted(3, &count);
    counted_list_push_back(&list, &a);
    counted_list_push_back(&list, &b);
    counted_list_push_back(&list, &c);

    counted_list_iter_t mid = counted_list_iter_next(counted_list_iter_begin(&list));
    assert(mid->value.value == 2);

    counted_t out;
    counted_list_remove(&list, mid, &out);
    assert(count == 0);  // moved out, not destroyed
    assert(out.value == 2);

    counted_destroy(&out);
    assert(count == 1);

    counted_list_destroy(&list);
    assert(count == 3);  // elements 1 and 3 also destroyed
}

static void test_counted_list_remove_mid_null_destroys(void) {
    int count = 0;
    counted_list_t list = counted_list_new();

    counted_t a = make_counted(1, &count);
    counted_t b = make_counted(2, &count);
    counted_t c = make_counted(3, &count);
    counted_list_push_back(&list, &a);
    counted_list_push_back(&list, &b);
    counted_list_push_back(&list, &c);

    counted_list_iter_t mid = counted_list_iter_next(counted_list_iter_begin(&list));
    counted_list_remove(&list, mid, NULL);
    assert(count == 1);  // element 2 destroyed immediately

    counted_list_destroy(&list);
    assert(count == 3);  // elements 1 and 3 also destroyed
}

static void test_counted_list_move_semantics_on_push(void) {
    // After push_back, the source element's destroy_count should be NULL
    // (moved out), so a subsequent counted_destroy on it must not increment.
    int count = 0;
    counted_list_t list = counted_list_new();

    counted_t a = make_counted(99, &count);
    counted_list_push_back(&list, &a);

    // 'a' has been moved: its destroy_count is now NULL
    assert(a.destroy_count == NULL);

    // Destroying the moved-from local must not increment count
    counted_destroy(&a);
    assert(count == 0);

    counted_list_destroy(&list);
    assert(count == 1);  // element inside the list destroyed
}

// ── Tests: int_alloc_list (custom allocator with observable side-effects) ─────

static void test_alloc_list_alloc_called_on_push(void) {
    tracking_alloc_ctx_t ctx = {0, 0};
    int_alloc_list_t list = int_alloc_list_new(ctx);

    int v1 = 1, v2 = 2, v3 = 3;
    int_alloc_list_push_back(&list, &v1);
    int_alloc_list_push_back(&list, &v2);
    int_alloc_list_push_back(&list, &v3);

    // One allocation per node
    assert(list._alloc_ctx.alloc_count == 3);
    assert(list._alloc_ctx.free_count == 0);

    int_alloc_list_destroy(&list);

    // One free per node
    assert(list._alloc_ctx.free_count == 3);
}

static void test_alloc_list_free_called_on_pop(void) {
    tracking_alloc_ctx_t ctx = {0, 0};
    int_alloc_list_t list = int_alloc_list_new(ctx);

    int v1 = 10, v2 = 20;
    int_alloc_list_push_back(&list, &v1);
    int_alloc_list_push_back(&list, &v2);
    assert(list._alloc_ctx.alloc_count == 2);

    int out = 0;
    int_alloc_list_pop_front(&list, &out);
    assert(out == 10);
    assert(list._alloc_ctx.free_count == 1);

    int_alloc_list_pop_back(&list, &out);
    assert(out == 20);
    assert(list._alloc_ctx.free_count == 2);

    assert(list._alloc_ctx.alloc_count == 2);  // no extra allocations

    int_alloc_list_destroy(&list);
}

static void test_alloc_list_free_called_on_remove(void) {
    tracking_alloc_ctx_t ctx = {0, 0};
    int_alloc_list_t list = int_alloc_list_new(ctx);

    int v1 = 1, v2 = 2, v3 = 3;
    int_alloc_list_push_back(&list, &v1);
    int_alloc_list_push_back(&list, &v2);
    int_alloc_list_push_back(&list, &v3);
    assert(list._alloc_ctx.alloc_count == 3);

    // Remove the middle node
    int_alloc_list_iter_t mid = int_alloc_list_iter_next(int_alloc_list_iter_begin(&list));
    int_alloc_list_remove(&list, mid, NULL);
    assert(list._alloc_ctx.free_count == 1);

    int_alloc_list_destroy(&list);
    assert(list._alloc_ctx.free_count == 3);  // 2 remaining nodes freed by destroy
}

static void test_alloc_list_balanced_alloc_free(void) {
    // Verify alloc and free counts are balanced after a sequence of operations.
    tracking_alloc_ctx_t ctx = {0, 0};
    int_alloc_list_t list = int_alloc_list_new(ctx);

    for (int i = 0; i < 10; i++) {
        int_alloc_list_push_back(&list, &i);
    }
    assert(list._alloc_ctx.alloc_count == 10);

    // Pop half from the front
    for (int i = 0; i < 5; i++) {
        int_alloc_list_pop_front(&list, NULL);
    }
    assert(list._alloc_ctx.free_count == 5);

    // Destroy the rest
    int_alloc_list_destroy(&list);
    assert(list._alloc_ctx.free_count == 10);
    assert(list._alloc_ctx.alloc_count == list._alloc_ctx.free_count);
}

// ── main ──────────────────────────────────────────────────────────────────────

int main(void) {
    // int list — basic operations
    test_int_list_new_empty();
    test_int_list_push_back();
    test_int_list_push_front();
    test_int_list_pop_back();
    test_int_list_pop_front();
    test_int_list_pop_discard();

    // int list — mid-list insertion and removal
    test_int_list_insert_after();
    test_int_list_insert_before();
    test_int_list_remove_mid();
    test_int_list_insert_after_null();
    test_int_list_insert_before_null();

    // int list — iteration
    test_int_list_forward_iteration();
    test_int_list_reverse_iteration();
    test_int_list_const_iteration();

    // counted_list — non-trivial element lifecycle with observable side-effects
    test_counted_list_destroy_calls_destructor();
    test_counted_list_pop_back_moves_out();
    test_counted_list_pop_back_null_destroys();
    test_counted_list_pop_front_moves_out();
    test_counted_list_remove_mid_moves_out();
    test_counted_list_remove_mid_null_destroys();
    test_counted_list_move_semantics_on_push();

    // int_alloc_list — custom allocator with observable alloc/free side-effects
    test_alloc_list_alloc_called_on_push();
    test_alloc_list_free_called_on_pop();
    test_alloc_list_free_called_on_remove();
    test_alloc_list_balanced_alloc_free();

    printf("All list_template tests passed.\n");
    return 0;
}
