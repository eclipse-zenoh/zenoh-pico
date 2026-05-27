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

// Tests for variant_template.h
//
// Four instantiations are tested in the same TU:
//
//   1. my_variant  — 2-type, default 1/2 names
//   2. my_result   — 2-type, custom names "ok" / "err"
//   3. my_tri      — 3-type, custom names "i32" / "f64" / "str"
//   4. my_quad     — 4-type, default names 1 / 2 / 3 / 4
//
// A very small heap-allocated string type is used for arms that need
// non-trivial move/destroy callbacks.

#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ─── helpers: heap string ────────────────────────────────────────────────────

typedef struct {
    char *ptr;
} hstring_t;

static hstring_t hstring_make(const char *s) {
    hstring_t h;
    h.ptr = strdup(s);
    assert(h.ptr);
    return h;
}

static void hstring_destroy(hstring_t *h) {
    free(h->ptr);
    h->ptr = NULL;
}

static void hstring_move(hstring_t *dst, hstring_t *src) {
    dst->ptr = src->ptr;
    src->ptr = NULL;
}

// ─── Instantiation 1: my_variant (default 1/2 names) ────────────────────────

#define _ZP_VARIANT_TEMPLATE_NAME my_variant
#define _ZP_VARIANT_TEMPLATE_1_TYPE int
#define _ZP_VARIANT_TEMPLATE_2_TYPE hstring_t
#define _ZP_VARIANT_TEMPLATE_2_DESTROY_FN(ptr) hstring_destroy(ptr)
#define _ZP_VARIANT_TEMPLATE_2_MOVE_FN(dst, src) hstring_move(dst, src)
#include "zenoh-pico/collections/variant_template.h"

// ─── Instantiation 2: my_result (custom names ok / err) ─────────────────────

#define _ZP_VARIANT_TEMPLATE_NAME my_result
#define _ZP_VARIANT_TEMPLATE_1_TYPE int
#define _ZP_VARIANT_TEMPLATE_1_NAME ok
#define _ZP_VARIANT_TEMPLATE_2_TYPE hstring_t
#define _ZP_VARIANT_TEMPLATE_2_NAME err
#define _ZP_VARIANT_TEMPLATE_2_DESTROY_FN(ptr) hstring_destroy(ptr)
#define _ZP_VARIANT_TEMPLATE_2_MOVE_FN(dst, src) hstring_move(dst, src)
#include "zenoh-pico/collections/variant_template.h"

// ─── Instantiation 3: my_tri (3 types: int / double / hstring_t) ────────────

#define _ZP_VARIANT_TEMPLATE_NAME my_tri
#define _ZP_VARIANT_TEMPLATE_1_TYPE int
#define _ZP_VARIANT_TEMPLATE_1_NAME i32
#define _ZP_VARIANT_TEMPLATE_2_TYPE double
#define _ZP_VARIANT_TEMPLATE_2_NAME f64
#define _ZP_VARIANT_TEMPLATE_3_TYPE hstring_t
#define _ZP_VARIANT_TEMPLATE_3_NAME str
#define _ZP_VARIANT_TEMPLATE_3_DESTROY_FN(ptr) hstring_destroy(ptr)
#define _ZP_VARIANT_TEMPLATE_3_MOVE_FN(dst, src) hstring_move(dst, src)
#include "zenoh-pico/collections/variant_template.h"

// ─── Instantiation 4: my_quad (4 types, default names 1/2/3/4) ──────────────

#define _ZP_VARIANT_TEMPLATE_NAME my_quad
#define _ZP_VARIANT_TEMPLATE_1_TYPE int
#define _ZP_VARIANT_TEMPLATE_2_TYPE double
#define _ZP_VARIANT_TEMPLATE_3_TYPE float
#define _ZP_VARIANT_TEMPLATE_4_TYPE hstring_t
#define _ZP_VARIANT_TEMPLATE_4_DESTROY_FN(ptr) hstring_destroy(ptr)
#define _ZP_VARIANT_TEMPLATE_4_MOVE_FN(dst, src) hstring_move(dst, src)
#include "zenoh-pico/collections/variant_template.h"

// ─── Tests for my_variant (default names) ────────────────────────────────────

static void test_default_none(void) {
    my_variant_t v = my_variant_none();
    assert(my_variant_is_none(&v));
    assert(!my_variant_is_1(&v));
    assert(!my_variant_is_2(&v));
    assert(my_variant_tag(&v) == my_variant_tag_none);
    my_variant_destroy(&v);
    assert(my_variant_is_none(&v));
    printf("  test_default_none              OK\n");
}

static void test_default_from_a(void) {
    int x = 42;
    my_variant_t v = my_variant_from_1(&x);
    assert(!my_variant_is_none(&v));
    assert(my_variant_is_1(&v));
    assert(!my_variant_is_2(&v));
    assert(my_variant_tag(&v) == my_variant_tag_1);
    assert(*my_variant_get_1(&v) == 42);
    my_variant_destroy(&v);
    assert(my_variant_is_none(&v));
    printf("  test_default_from_a            OK\n");
}

static void test_default_from_b(void) {
    hstring_t s = hstring_make("hello");
    my_variant_t v = my_variant_from_2(&s);
    // s is moved: ptr should be NULL now
    assert(s.ptr == NULL);
    assert(my_variant_is_2(&v));
    assert(strcmp(my_variant_get_2(&v)->ptr, "hello") == 0);
    my_variant_destroy(&v);
    assert(my_variant_is_none(&v));
    printf("  test_default_from_b            OK\n");
}

static void test_default_move(void) {
    hstring_t s = hstring_make("world");
    my_variant_t src = my_variant_from_2(&s);
    my_variant_t dst = my_variant_none();

    my_variant_move(&dst, &src);

    assert(my_variant_is_none(&src));
    assert(my_variant_is_2(&dst));
    assert(strcmp(my_variant_get_2(&dst)->ptr, "world") == 0);

    // Move onto a non-empty dst (old A value must be destroyed)
    int x = 7;
    my_variant_t dst2 = my_variant_from_1(&x);
    my_variant_move(&dst2, &dst);

    assert(my_variant_is_none(&dst));
    assert(my_variant_is_2(&dst2));
    assert(strcmp(my_variant_get_2(&dst2)->ptr, "world") == 0);
    my_variant_destroy(&dst2);
    printf("  test_default_move              OK\n");
}

static void test_default_take(void) {
    // take_a succeeds when holding A
    int x = 99;
    my_variant_t v = my_variant_from_1(&x);
    int out_a = 0;
    assert(my_variant_take_1(&v, &out_a));
    assert(out_a == 99);
    assert(my_variant_is_none(&v));

    // take_a fails when holding B
    hstring_t s = hstring_make("hi");
    v = my_variant_from_2(&s);
    int out_a2 = 0;
    assert(!my_variant_take_1(&v, &out_a2));
    assert(my_variant_is_2(&v));  // unchanged

    // take_b succeeds when holding B
    hstring_t out_b;
    memset(&out_b, 0, sizeof(out_b));
    assert(my_variant_take_2(&v, &out_b));
    assert(strcmp(out_b.ptr, "hi") == 0);
    assert(my_variant_is_none(&v));
    hstring_destroy(&out_b);

    // take_b fails when holding NONE
    assert(!my_variant_take_2(&v, &out_b));
    printf("  test_default_take              OK\n");
}

// ─── Tests for my_result (custom names ok / err) ─────────────────────────────

static void test_custom_none(void) {
    my_result_t r = my_result_none();
    assert(my_result_is_none(&r));
    assert(!my_result_is_ok(&r));
    assert(!my_result_is_err(&r));
    assert(my_result_tag(&r) == my_result_tag_none);
    my_result_destroy(&r);
    printf("  test_custom_none               OK\n");
}

static void test_custom_from_ok(void) {
    int code = 0;
    my_result_t r = my_result_from_ok(&code);
    assert(my_result_is_ok(&r));
    assert(!my_result_is_err(&r));
    assert(my_result_tag(&r) == my_result_tag_ok);
    assert(*my_result_get_ok(&r) == 0);
    my_result_destroy(&r);
    assert(my_result_is_none(&r));
    printf("  test_custom_from_ok            OK\n");
}

static void test_custom_from_err(void) {
    hstring_t msg = hstring_make("something went wrong");
    my_result_t r = my_result_from_err(&msg);
    assert(msg.ptr == NULL);  // moved
    assert(my_result_is_err(&r));
    assert(my_result_tag(&r) == my_result_tag_err);
    assert(strcmp(my_result_get_err(&r)->ptr, "something went wrong") == 0);
    my_result_destroy(&r);
    assert(my_result_is_none(&r));
    printf("  test_custom_from_err           OK\n");
}

static void test_custom_move_ok_to_err(void) {
    int code = 1;
    my_result_t ok_r = my_result_from_ok(&code);

    hstring_t msg = hstring_make("oops");
    my_result_t err_r = my_result_from_err(&msg);

    // Move err_r into ok_r; old ok value (trivial int) is destroyed, err moves
    my_result_move(&ok_r, &err_r);

    assert(my_result_is_none(&err_r));
    assert(my_result_is_err(&ok_r));
    assert(strcmp(my_result_get_err(&ok_r)->ptr, "oops") == 0);
    my_result_destroy(&ok_r);
    printf("  test_custom_move_ok_to_err     OK\n");
}

static void test_custom_take(void) {
    // take_ok succeeds when holding ok
    int code = 42;
    my_result_t r = my_result_from_ok(&code);
    int out_ok = 0;
    assert(my_result_take_ok(&r, &out_ok));
    assert(out_ok == 42);
    assert(my_result_is_none(&r));

    // take_ok fails when holding err
    hstring_t msg = hstring_make("bad");
    r = my_result_from_err(&msg);
    int out_ok2 = 0;
    assert(!my_result_take_ok(&r, &out_ok2));
    assert(my_result_is_err(&r));  // unchanged

    // take_err succeeds when holding err
    hstring_t out_err;
    memset(&out_err, 0, sizeof(out_err));
    assert(my_result_take_err(&r, &out_err));
    assert(strcmp(out_err.ptr, "bad") == 0);
    assert(my_result_is_none(&r));
    hstring_destroy(&out_err);

    // take_err fails when holding NONE
    assert(!my_result_take_err(&r, &out_err));
    printf("  test_custom_take               OK\n");
}

// ─── main ─────────────────────────────────────────────────────────────────────

static void test_tri(void) {
    // from_i32
    int i = 7;
    my_tri_t v = my_tri_from_i32(&i);
    assert(my_tri_is_i32(&v));
    assert(!my_tri_is_f64(&v));
    assert(!my_tri_is_str(&v));
    assert(my_tri_tag(&v) == my_tri_tag_i32);
    assert(*my_tri_get_i32(&v) == 7);
    my_tri_destroy(&v);
    assert(my_tri_is_none(&v));

    // from_f64
    double d = 3.14;
    v = my_tri_from_f64(&d);
    assert(my_tri_is_f64(&v));
    assert(my_tri_tag(&v) == my_tri_tag_f64);
    assert(*my_tri_get_f64(&v) == 3.14);
    my_tri_destroy(&v);

    // from_str + take_str
    hstring_t s = hstring_make("hello tri");
    v = my_tri_from_str(&s);
    assert(s.ptr == NULL);  // moved
    assert(my_tri_is_str(&v));
    assert(my_tri_tag(&v) == my_tri_tag_str);

    hstring_t out;
    memset(&out, 0, sizeof(out));
    assert(!my_tri_take_i32(&v, &i));  // wrong arm
    assert(my_tri_is_str(&v));         // unchanged
    assert(my_tri_take_str(&v, &out));
    assert(strcmp(out.ptr, "hello tri") == 0);
    assert(my_tri_is_none(&v));
    hstring_destroy(&out);

    // move between arms
    hstring_t s2 = hstring_make("move me");
    my_tri_t src = my_tri_from_str(&s2);
    int x2 = 99;
    my_tri_t dst = my_tri_from_i32(&x2);
    my_tri_move(&dst, &src);
    assert(my_tri_is_none(&src));
    assert(my_tri_is_str(&dst));
    assert(strcmp(my_tri_get_str(&dst)->ptr, "move me") == 0);
    my_tri_destroy(&dst);

    printf("  test_tri                       OK\n");
}

static void test_quad(void) {
    // from_a (int)
    int i = 1;
    my_quad_t v = my_quad_from_1(&i);
    assert(my_quad_is_1(&v));
    assert(my_quad_tag(&v) == my_quad_tag_1);
    assert(*my_quad_get_1(&v) == 1);
    my_quad_destroy(&v);

    // from_b (double)
    double d = 2.5;
    v = my_quad_from_2(&d);
    assert(my_quad_is_2(&v));
    assert(my_quad_tag(&v) == my_quad_tag_2);
    my_quad_destroy(&v);

    // from_c (float)
    float f = 3.0f;
    v = my_quad_from_3(&f);
    assert(my_quad_is_3(&v));
    assert(my_quad_tag(&v) == my_quad_tag_3);
    my_quad_destroy(&v);

    // from_d (hstring_t) + take_d
    hstring_t s = hstring_make("quad string");
    v = my_quad_from_4(&s);
    assert(s.ptr == NULL);
    assert(my_quad_is_4(&v));
    assert(my_quad_tag(&v) == my_quad_tag_4);

    hstring_t out;
    memset(&out, 0, sizeof(out));
    assert(!my_quad_take_1(&v, &i));  // wrong arm
    assert(my_quad_is_4(&v));
    assert(my_quad_take_4(&v, &out));
    assert(strcmp(out.ptr, "quad string") == 0);
    assert(my_quad_is_none(&v));
    hstring_destroy(&out);

    printf("  test_quad                      OK\n");
}

// ─── Tests for _ZP_VARIANT_VISIT ─────────────────────────────────────────────

// --- 2-type: my_variant ---

static int g_visit_called_arm = 0;  // 1=A, 2=B

static void visit2_a(int *v) {
    (void)v;
    g_visit_called_arm = 1;
}
static void visit2_b(hstring_t *v) {
    (void)v;
    g_visit_called_arm = 2;
}

static void test_visit_2type(void) {
    // Visits A arm
    int x = 10;
    my_variant_t v = my_variant_from_1(&x);
    g_visit_called_arm = 0;
    _ZP_VARIANT_VISIT(&v, visit2_a, visit2_b);
    assert(g_visit_called_arm == 1);
    my_variant_destroy(&v);

    // Visits B arm
    hstring_t s = hstring_make("visit-b");
    v = my_variant_from_2(&s);
    g_visit_called_arm = 0;
    _ZP_VARIANT_VISIT(&v, visit2_a, visit2_b);
    assert(g_visit_called_arm == 2);
    my_variant_destroy(&v);

    // NONE: no arm called (tag == 0, switch falls through)
    v = my_variant_none();
    g_visit_called_arm = -1;
    _ZP_VARIANT_VISIT(&v, visit2_a, visit2_b);
    assert(g_visit_called_arm == -1);  // unchanged
    my_variant_destroy(&v);

    printf("  test_visit_2type               OK\n");
}

// --- 3-type: my_tri ---

static int g_visit3_called = 0;  // 1=i32, 2=f64, 3=str

static void visit3_i32(int *v) {
    (void)v;
    g_visit3_called = 1;
}
static void visit3_f64(double *v) {
    (void)v;
    g_visit3_called = 2;
}
static void visit3_str(hstring_t *v) {
    (void)v;
    g_visit3_called = 3;
}

static void test_visit_3type(void) {
    int i = 5;
    my_tri_t v = my_tri_from_i32(&i);
    g_visit3_called = 0;
    _ZP_VARIANT_VISIT(&v, visit3_i32, visit3_f64, visit3_str);
    assert(g_visit3_called == 1);
    my_tri_destroy(&v);

    double d = 1.5;
    v = my_tri_from_f64(&d);
    g_visit3_called = 0;
    _ZP_VARIANT_VISIT(&v, visit3_i32, visit3_f64, visit3_str);
    assert(g_visit3_called == 2);
    my_tri_destroy(&v);

    hstring_t s = hstring_make("tri-str");
    v = my_tri_from_str(&s);
    g_visit3_called = 0;
    _ZP_VARIANT_VISIT(&v, visit3_i32, visit3_f64, visit3_str);
    assert(g_visit3_called == 3);
    my_tri_destroy(&v);

    // NONE: no arm called
    v = my_tri_none();
    g_visit3_called = -1;
    _ZP_VARIANT_VISIT(&v, visit3_i32, visit3_f64, visit3_str);
    assert(g_visit3_called == -1);

    printf("  test_visit_3type               OK\n");
}

// --- 4-type: my_quad ---

static int g_visit4_called = 0;  // 1=a, 2=b, 3=c, 4=d

static void visit4_a(int *v) {
    (void)v;
    g_visit4_called = 1;
}
static void visit4_b(double *v) {
    (void)v;
    g_visit4_called = 2;
}
static void visit4_c(float *v) {
    (void)v;
    g_visit4_called = 3;
}
static void visit4_d(hstring_t *v) {
    (void)v;
    g_visit4_called = 4;
}

static void test_visit_4type(void) {
    int i = 1;
    my_quad_t v = my_quad_from_1(&i);
    g_visit4_called = 0;
    _ZP_VARIANT_VISIT(&v, visit4_a, visit4_b, visit4_c, visit4_d);
    assert(g_visit4_called == 1);
    my_quad_destroy(&v);

    double d = 2.0;
    v = my_quad_from_2(&d);
    g_visit4_called = 0;
    _ZP_VARIANT_VISIT(&v, visit4_a, visit4_b, visit4_c, visit4_d);
    assert(g_visit4_called == 2);
    my_quad_destroy(&v);

    float f = 3.0f;
    v = my_quad_from_3(&f);
    g_visit4_called = 0;
    _ZP_VARIANT_VISIT(&v, visit4_a, visit4_b, visit4_c, visit4_d);
    assert(g_visit4_called == 3);
    my_quad_destroy(&v);

    hstring_t s = hstring_make("quad-d");
    v = my_quad_from_4(&s);
    g_visit4_called = 0;
    _ZP_VARIANT_VISIT(&v, visit4_a, visit4_b, visit4_c, visit4_d);
    assert(g_visit4_called == 4);
    my_quad_destroy(&v);

    // NONE: no arm called
    v = my_quad_none();
    g_visit4_called = -1;
    _ZP_VARIANT_VISIT(&v, visit4_a, visit4_b, visit4_c, visit4_d);
    assert(g_visit4_called == -1);

    printf("  test_visit_4type               OK\n");
}

static void test_visit_generic(void) {
    my_tri_t v[3] = {my_tri_from_f64(&(double){3.5}), my_tri_from_i32(&(int){42}),
                     my_tri_from_str(&(hstring_t){.ptr = "1234"})};

    int out[3] = {3, 42, 1234};

#define DOUBLE_TO_INT(d) \
    { r = (int)(*d); }
#define IDENTITY(x) \
    { r = *x; }
#define STR_TO_INT(s) \
    { r = atoi((s)->ptr); }
    for (int i = 0; i < 3; i++) {
        int r = 0;
        _ZP_VARIANT_VISIT(&v[i], DOUBLE_TO_INT, IDENTITY, STR_TO_INT);
        assert(r == out[i]);
    }

    printf("  test_visit_generic             OK\n");
}

int main(void) {
    printf("variant_template tests\n");
    printf("── default names (1 / 2) ────\n");
    test_default_none();
    test_default_from_a();
    test_default_from_b();
    test_default_move();
    test_default_take();
    printf("── custom names (ok / err) ──\n");
    test_custom_none();
    test_custom_from_ok();
    test_custom_from_err();
    test_custom_move_ok_to_err();
    test_custom_take();
    printf("── 3-type (i32 / f64 / str) ─\n");
    test_tri();
    printf("── 4-type (1 / 2 / 3 / 4) ───\n");
    test_quad();
    printf("── _ZP_VARIANT_VISIT macro ──\n");
    test_visit_2type();
    test_visit_3type();
    test_visit_4type();
    test_visit_generic();
    printf("All variant_template tests passed.\n");
    return 0;
}
