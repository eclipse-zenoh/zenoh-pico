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

// ── Instantiate a singly-linked list of int ───────────────────────────────────

#define _ZP_SLIST_TEMPLATE_ELEM_TYPE int
#define _ZP_SLIST_TEMPLATE_NAME int_slist
#include "zenoh-pico/collections/slist_template.h"

// ── Instantiate a singly-linked list with observable destructor side-effects ──

typedef struct {
    int value;
    int *destroy_count;  // points to a test-local counter; NULL means moved-out
} scounted_t;

static void scounted_destroy(scounted_t *p) {
    if (p->destroy_count != NULL) {
        (*p->destroy_count)++;
    }
}
static void scounted_move(scounted_t *dst, scounted_t *src) {
    *dst = *src;
    src->destroy_count = NULL;  // prevent double-count on source destruction
}

#define _ZP_SLIST_TEMPLATE_ELEM_TYPE scounted_t
#define _ZP_SLIST_TEMPLATE_NAME scounted_slist
#define _ZP_SLIST_TEMPLATE_ELEM_DESTROY_FN(p) scounted_destroy(p)
#define _ZP_SLIST_TEMPLATE_ELEM_MOVE_FN(dst, src) scounted_move(dst, src)
#include "zenoh-pico/collections/slist_template.h"

static scounted_t make_scounted(int value, int *counter) {
    scounted_t c;
    c.value = value;
    c.destroy_count = counter;
    return c;
}

// ── Instantiate a singly-linked list with a custom tracking allocator ─────────

typedef struct {
    int alloc_count;
    int free_count;
} stracking_alloc_ctx_t;

static void *stracking_alloc(stracking_alloc_ctx_t *ctx, size_t bytes) {
    ctx->alloc_count++;
    return malloc(bytes);
}
static void stracking_free(stracking_alloc_ctx_t *ctx, void *ptr) {
    ctx->free_count++;
    free(ptr);
}

#define _ZP_SLIST_TEMPLATE_ELEM_TYPE int
#define _ZP_SLIST_TEMPLATE_NAME int_salloc_slist
#define _ZP_SLIST_TEMPLATE_ALLOC_CTX_TYPE stracking_alloc_ctx_t
#define _ZP_SLIST_TEMPLATE_ALLOC_FN(ctx, bytes) stracking_alloc(ctx, bytes)
#define _ZP_SLIST_TEMPLATE_FREE_FN(ctx, ptr) stracking_free(ctx, ptr)
#include "zenoh-pico/collections/slist_template.h"

// ── Tests: int_slist ──────────────────────────────────────────────────────────

static void test_int_slist_new_empty(void) {
    int_slist_t list = int_slist_new();
    assert(int_slist_size(&list) == 0);
    assert(int_slist_is_empty(&list));
    assert(int_slist_front(&list) == NULL);
    assert(int_slist_cfront(&list) == NULL);
    assert(int_slist_iter_begin(&list) == _ZP_SLIST_ITER_INVALID);
    int_slist_destroy(&list);
}

static void test_int_slist_push_front(void) {
    int_slist_t list = int_slist_new();
    int v1 = 1, v2 = 2, v3 = 3;
    assert(int_slist_push_front(&list, &v1));
    assert(int_slist_push_front(&list, &v2));
    assert(int_slist_push_front(&list, &v3));

    assert(int_slist_size(&list) == 3);
    assert(*int_slist_front(&list) == 3);

    int expected[] = {3, 2, 1};
    int idx = 0;
    for (int_slist_iter_t it = int_slist_iter_begin(&list); it != _ZP_SLIST_ITER_INVALID;
         it = int_slist_iter_next(it)) {
        assert(it->value == expected[idx++]);
    }
    assert(idx == 3);

    int_slist_destroy(&list);
}

static void test_int_slist_push_back(void) {
    int_slist_t list = int_slist_new();
    int v1 = 10, v2 = 20, v3 = 30;
    assert(int_slist_push_back(&list, &v1));
    assert(int_slist_push_back(&list, &v2));
    assert(int_slist_push_back(&list, &v3));

    assert(int_slist_size(&list) == 3);
    assert(*int_slist_front(&list) == 10);

    int expected[] = {10, 20, 30};
    int idx = 0;
    for (int_slist_iter_t it = int_slist_iter_begin(&list); it != _ZP_SLIST_ITER_INVALID;
         it = int_slist_iter_next(it)) {
        assert(it->value == expected[idx++]);
    }
    assert(idx == 3);

    int_slist_destroy(&list);
}

static void test_int_slist_pop_front(void) {
    int_slist_t list = int_slist_new();
    int v1 = 1, v2 = 2, v3 = 3;
    int_slist_push_back(&list, &v1);
    int_slist_push_back(&list, &v2);
    int_slist_push_back(&list, &v3);

    int out = 0;
    assert(int_slist_pop_front(&list, &out));
    assert(out == 1);
    assert(int_slist_size(&list) == 2);

    assert(int_slist_pop_front(&list, &out));
    assert(out == 2);

    assert(int_slist_pop_front(&list, &out));
    assert(out == 3);

    assert(!int_slist_pop_front(&list, &out));  // empty
    assert(int_slist_is_empty(&list));

    int_slist_destroy(&list);
}

static void test_int_slist_pop_front_null(void) {
    // pop with NULL out should destroy in place (no-op for int)
    int_slist_t list = int_slist_new();
    int v1 = 7, v2 = 8;
    int_slist_push_back(&list, &v1);
    int_slist_push_back(&list, &v2);

    assert(int_slist_pop_front(&list, NULL));
    assert(int_slist_size(&list) == 1);
    assert(*int_slist_front(&list) == 8);

    assert(int_slist_pop_front(&list, NULL));
    assert(int_slist_is_empty(&list));

    int_slist_destroy(&list);
}

static void test_int_slist_insert_after_null_prepends(void) {
    // insert_after(NULL) should prepend to front
    int_slist_t list = int_slist_new();
    int v1 = 2, v2 = 1;
    int_slist_push_back(&list, &v1);
    assert(int_slist_insert_after(&list, _ZP_SLIST_ITER_INVALID, &v2));

    assert(int_slist_size(&list) == 2);
    assert(*int_slist_front(&list) == 1);

    int expected[] = {1, 2};
    int idx = 0;
    for (int_slist_iter_t it = int_slist_iter_begin(&list); it != _ZP_SLIST_ITER_INVALID;
         it = int_slist_iter_next(it)) {
        assert(it->value == expected[idx++]);
    }
    assert(idx == 2);

    int_slist_destroy(&list);
}

static void test_int_slist_insert_after_mid(void) {
    int_slist_t list = int_slist_new();
    int v1 = 1, v3 = 3, v2 = 2;
    int_slist_push_back(&list, &v1);
    int_slist_push_back(&list, &v3);

    // Insert 2 after the first node (value 1)
    int_slist_iter_t head = int_slist_iter_begin(&list);
    assert(head != _ZP_SLIST_ITER_INVALID);
    assert(int_slist_insert_after(&list, head, &v2));

    assert(int_slist_size(&list) == 3);

    int expected[] = {1, 2, 3};
    int idx = 0;
    for (int_slist_iter_t it = int_slist_iter_begin(&list); it != _ZP_SLIST_ITER_INVALID;
         it = int_slist_iter_next(it)) {
        assert(it->value == expected[idx++]);
    }
    assert(idx == 3);

    int_slist_destroy(&list);
}

static void test_int_slist_insert_after_tail(void) {
    // insert_after(last_node) should append to the back, keeping tail valid
    int_slist_t list = int_slist_new();
    int v1 = 1, v2 = 2, v3 = 3;
    int_slist_push_back(&list, &v1);
    int_slist_push_back(&list, &v2);

    // Find the last node
    int_slist_iter_t it = int_slist_iter_begin(&list);
    while (int_slist_iter_next(it) != _ZP_SLIST_ITER_INVALID) {
        it = int_slist_iter_next(it);
    }
    assert(int_slist_insert_after(&list, it, &v3));
    assert(int_slist_size(&list) == 3);

    // Verify a subsequent push_back still works correctly
    int v4 = 4;
    int_slist_push_back(&list, &v4);
    assert(int_slist_size(&list) == 4);

    int expected[] = {1, 2, 3, 4};
    int idx = 0;
    for (int_slist_iter_t jt = int_slist_iter_begin(&list); jt != _ZP_SLIST_ITER_INVALID;
         jt = int_slist_iter_next(jt)) {
        assert(jt->value == expected[idx++]);
    }
    assert(idx == 4);

    int_slist_destroy(&list);
}

static void test_int_slist_remove_after_null_removes_front(void) {
    int_slist_t list = int_slist_new();
    int v1 = 1, v2 = 2;
    int_slist_push_back(&list, &v1);
    int_slist_push_back(&list, &v2);

    int out = 0;
    assert(int_slist_remove_after(&list, NULL, &out));
    assert(out == 1);
    assert(int_slist_size(&list) == 1);
    assert(*int_slist_front(&list) == 2);

    // Remove the last element via NULL prev
    assert(int_slist_remove_after(&list, NULL, &out));
    assert(out == 2);
    assert(int_slist_is_empty(&list));

    // Verify push_back still works on the now-empty list
    int v3 = 99;
    int_slist_push_back(&list, &v3);
    assert(int_slist_size(&list) == 1);
    assert(*int_slist_front(&list) == 99);

    int_slist_destroy(&list);
}

static void test_int_slist_remove_after_mid(void) {
    int_slist_t list = int_slist_new();
    int v1 = 1, v2 = 2, v3 = 3;
    int_slist_push_back(&list, &v1);
    int_slist_push_back(&list, &v2);
    int_slist_push_back(&list, &v3);

    // Remove the node after head (value 2)
    int_slist_iter_t head = int_slist_iter_begin(&list);
    int out = 0;
    assert(int_slist_remove_after(&list, head, &out));
    assert(out == 2);
    assert(int_slist_size(&list) == 2);

    int expected[] = {1, 3};
    int idx = 0;
    for (int_slist_iter_t it = int_slist_iter_begin(&list); it != _ZP_SLIST_ITER_INVALID;
         it = int_slist_iter_next(it)) {
        assert(it->value == expected[idx++]);
    }
    assert(idx == 2);

    int_slist_destroy(&list);
}

static void test_int_slist_remove_after_tail_returns_false(void) {
    int_slist_t list = int_slist_new();
    int v1 = 1;
    int_slist_push_back(&list, &v1);

    int_slist_iter_t only = int_slist_iter_begin(&list);
    int out = 0;
    // There is no node after the only node
    assert(!int_slist_remove_after(&list, only, &out));
    assert(int_slist_size(&list) == 1);

    int_slist_destroy(&list);
}

static void test_int_slist_remove_after_empty_returns_false(void) {
    int_slist_t list = int_slist_new();
    int out = 0;
    assert(!int_slist_remove_after(&list, NULL, &out));
    int_slist_destroy(&list);
}

static void test_int_slist_forward_iteration(void) {
    int_slist_t list = int_slist_new();
    for (int i = 1; i <= 5; i++) {
        int_slist_push_back(&list, &i);
    }
    int expected = 1;
    for (int_slist_iter_t it = int_slist_iter_begin(&list); it != _ZP_SLIST_ITER_INVALID;
         it = int_slist_iter_next(it)) {
        assert(it->value == expected++);
    }
    assert(expected == 6);
    int_slist_destroy(&list);
}

static void test_int_slist_const_iteration(void) {
    int_slist_t list = int_slist_new();
    int v1 = 10, v2 = 20;
    int_slist_push_back(&list, &v1);
    int_slist_push_back(&list, &v2);

    const int_slist_t *clist = &list;
    int expected[] = {10, 20};
    int idx = 0;
    for (int_slist_citer_t it = int_slist_iter_cbegin(clist); it != _ZP_SLIST_ITER_INVALID;
         it = int_slist_iter_cnext(it)) {
        assert(it->value == expected[idx++]);
    }
    assert(idx == 2);
    int_slist_destroy(&list);
}

// ── Tests: scounted_slist (observable destructor) ─────────────────────────────

static void test_scounted_slist_destroy_calls_destructor(void) {
    int count = 0;
    scounted_slist_t list = scounted_slist_new();

    scounted_t a = make_scounted(1, &count);
    scounted_t b = make_scounted(2, &count);
    scounted_t c = make_scounted(3, &count);
    scounted_slist_push_back(&list, &a);
    scounted_slist_push_back(&list, &b);
    scounted_slist_push_back(&list, &c);

    assert(count == 0);
    scounted_slist_destroy(&list);
    assert(count == 3);
}

static void test_scounted_slist_move_semantics_on_push(void) {
    int count = 0;
    scounted_slist_t list = scounted_slist_new();

    scounted_t a = make_scounted(42, &count);
    scounted_slist_push_back(&list, &a);

    // Source must have been moved out
    assert(a.destroy_count == NULL);
    // Destroying the moved-from local must not increment count
    scounted_destroy(&a);
    assert(count == 0);

    scounted_slist_destroy(&list);
    assert(count == 1);
}

static void test_scounted_slist_pop_front_moves_out(void) {
    int count = 0;
    scounted_slist_t list = scounted_slist_new();

    scounted_t a = make_scounted(7, &count);
    scounted_slist_push_back(&list, &a);

    scounted_t out;
    assert(scounted_slist_pop_front(&list, &out));
    assert(count == 0);  // moved, not destroyed
    assert(out.value == 7);
    assert(out.destroy_count == &count);

    scounted_destroy(&out);
    assert(count == 1);

    scounted_slist_destroy(&list);
}

static void test_scounted_slist_pop_front_null_destroys(void) {
    int count = 0;
    scounted_slist_t list = scounted_slist_new();

    scounted_t a = make_scounted(99, &count);
    scounted_slist_push_back(&list, &a);

    assert(scounted_slist_pop_front(&list, NULL));
    assert(count == 1);  // destroyed in place

    scounted_slist_destroy(&list);
}

static void test_scounted_slist_remove_after_moves_out(void) {
    int count = 0;
    scounted_slist_t list = scounted_slist_new();

    scounted_t a = make_scounted(1, &count);
    scounted_t b = make_scounted(2, &count);
    scounted_t c = make_scounted(3, &count);
    scounted_slist_push_back(&list, &a);
    scounted_slist_push_back(&list, &b);
    scounted_slist_push_back(&list, &c);

    // Remove the middle element (after head)
    scounted_slist_iter_t head = scounted_slist_iter_begin(&list);
    scounted_t out;
    assert(scounted_slist_remove_after(&list, head, &out));
    assert(count == 0);  // moved, not destroyed
    assert(out.value == 2);

    scounted_destroy(&out);
    assert(count == 1);

    scounted_slist_destroy(&list);
    assert(count == 3);  // elements 1 and 3 also destroyed
}

static void test_scounted_slist_remove_after_null_destroys(void) {
    int count = 0;
    scounted_slist_t list = scounted_slist_new();

    scounted_t a = make_scounted(1, &count);
    scounted_t b = make_scounted(2, &count);
    scounted_slist_push_back(&list, &a);
    scounted_slist_push_back(&list, &b);

    // Remove front with NULL out → destroy in place
    scounted_slist_iter_t head = scounted_slist_iter_begin(&list);
    assert(scounted_slist_remove_after(&list, head, NULL));
    assert(count == 1);  // element 2 destroyed immediately

    scounted_slist_destroy(&list);
    assert(count == 2);  // element 1 also destroyed
}

// ── Tests: int_salloc_slist (custom allocator) ────────────────────────────────

static void test_salloc_slist_alloc_called_on_push(void) {
    stracking_alloc_ctx_t ctx = {0, 0};
    int_salloc_slist_t list = int_salloc_slist_new(ctx);

    int v1 = 1, v2 = 2, v3 = 3;
    int_salloc_slist_push_back(&list, &v1);
    int_salloc_slist_push_back(&list, &v2);
    int_salloc_slist_push_back(&list, &v3);

    assert(list._alloc_ctx.alloc_count == 3);
    assert(list._alloc_ctx.free_count == 0);

    int_salloc_slist_destroy(&list);
    assert(list._alloc_ctx.free_count == 3);
}

static void test_salloc_slist_free_called_on_pop(void) {
    stracking_alloc_ctx_t ctx = {0, 0};
    int_salloc_slist_t list = int_salloc_slist_new(ctx);

    int v1 = 10, v2 = 20;
    int_salloc_slist_push_back(&list, &v1);
    int_salloc_slist_push_back(&list, &v2);
    assert(list._alloc_ctx.alloc_count == 2);

    int out = 0;
    int_salloc_slist_pop_front(&list, &out);
    assert(out == 10);
    assert(list._alloc_ctx.free_count == 1);

    int_salloc_slist_pop_front(&list, &out);
    assert(out == 20);
    assert(list._alloc_ctx.free_count == 2);
    assert(list._alloc_ctx.alloc_count == 2);

    int_salloc_slist_destroy(&list);
}

static void test_salloc_slist_balanced_alloc_free(void) {
    stracking_alloc_ctx_t ctx = {0, 0};
    int_salloc_slist_t list = int_salloc_slist_new(ctx);

    for (int i = 0; i < 10; i++) {
        int_salloc_slist_push_back(&list, &i);
    }
    assert(list._alloc_ctx.alloc_count == 10);

    for (int i = 0; i < 5; i++) {
        int_salloc_slist_pop_front(&list, NULL);
    }
    assert(list._alloc_ctx.free_count == 5);

    int_salloc_slist_destroy(&list);
    assert(list._alloc_ctx.free_count == 10);
    assert(list._alloc_ctx.alloc_count == list._alloc_ctx.free_count);
}

// ── main ──────────────────────────────────────────────────────────────────────

int main(void) {
    // int_slist — basic operations
    test_int_slist_new_empty();
    test_int_slist_push_front();
    test_int_slist_push_back();
    test_int_slist_pop_front();
    test_int_slist_pop_front_null();

    // int_slist — insert_after
    test_int_slist_insert_after_null_prepends();
    test_int_slist_insert_after_mid();
    test_int_slist_insert_after_tail();

    // int_slist — remove_after
    test_int_slist_remove_after_null_removes_front();
    test_int_slist_remove_after_mid();
    test_int_slist_remove_after_tail_returns_false();
    test_int_slist_remove_after_empty_returns_false();

    // int_slist — iteration
    test_int_slist_forward_iteration();
    test_int_slist_const_iteration();

    // scounted_slist — observable destructor / move semantics
    test_scounted_slist_destroy_calls_destructor();
    test_scounted_slist_move_semantics_on_push();
    test_scounted_slist_pop_front_moves_out();
    test_scounted_slist_pop_front_null_destroys();
    test_scounted_slist_remove_after_moves_out();
    test_scounted_slist_remove_after_null_destroys();

    // int_salloc_slist — custom allocator with observable alloc/free side-effects
    test_salloc_slist_alloc_called_on_push();
    test_salloc_slist_free_called_on_pop();
    test_salloc_slist_balanced_alloc_free();

    printf("All slist_template tests passed.\n");
    return 0;
}
