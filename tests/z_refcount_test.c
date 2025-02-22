//
// Copyright (c) 2024 ZettaScale Technology
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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/collections/refcount.h"
#include "zenoh-pico/system/common/platform.h"

#undef NDEBUG
#include <assert.h>

#define FOO_CLEARED_VALUE -1

typedef struct _dummy_t {
    int foo;
} _dummy_t;

void _dummy_clear(_dummy_t *val) {
    val->foo = FOO_CLEARED_VALUE;
    return;
}

_Z_REFCOUNT_DEFINE(_dummy, _dummy)

typedef struct {
    unsigned int _strong_cnt;
    unsigned int _weak_cnt;
} _dummy_inner_rc_t;

void test_rc_null(void) {
    _dummy_rc_t drc = _dummy_rc_null();
    assert(drc._cnt == NULL);
    assert(drc._val == NULL);
}

void test_rc_size(void) { assert(_dummy_rc_size(NULL) == sizeof(_dummy_rc_t)); }

void test_rc_drop(void) {
    _dummy_rc_t drc = _dummy_rc_null();
    assert(!_dummy_rc_drop(NULL));
    assert(!_dummy_rc_drop(&drc));
}

void test_rc_new(void) {
    _dummy_t *val = z_malloc(sizeof(_dummy_t));
    val->foo = 42;
    _dummy_rc_t drc = _dummy_rc_new(val);
    assert(!_Z_RC_IS_NULL(&drc));
    assert(_z_rc_strong_count(drc._cnt) == 1);
    assert(_z_rc_weak_count(drc._cnt) == 1);
    assert(drc._val->foo == 42);
    drc._val->foo = 0;
    assert(val->foo == 0);
    assert(_dummy_rc_drop(&drc));
}

void test_rc_new_from_val(void) {
    _dummy_t val = {.foo = 42};
    _dummy_rc_t drc = _dummy_rc_new_from_val(&val);
    assert(!_Z_RC_IS_NULL(&drc));
    assert(_z_rc_strong_count(drc._cnt) == 1);
    assert(_z_rc_weak_count(drc._cnt) == 1);
    assert(drc._val->foo == 42);
    drc._val->foo = 0;
    assert(val.foo == 42);
    assert(_dummy_rc_drop(&drc));
}

void test_rc_clone(void) {
    _dummy_t val = {.foo = 42};
    _dummy_rc_t drc1 = _dummy_rc_new_from_val(&val);
    assert(_z_rc_strong_count(drc1._cnt) == 1);
    assert(_z_rc_weak_count(drc1._cnt) == 1);

    _dummy_rc_t drc2 = _dummy_rc_clone(&drc1);
    assert(!_Z_RC_IS_NULL(&drc2));
    assert(_z_rc_strong_count(drc2._cnt) == 2);
    assert(_z_rc_weak_count(drc2._cnt) == 2);
    assert(_z_rc_strong_count(drc2._cnt) == _z_rc_strong_count(drc1._cnt));
    assert(_z_rc_weak_count(drc2._cnt) == _z_rc_weak_count(drc1._cnt));
    assert(drc2._val->foo == drc1._val->foo);

    assert(!_dummy_rc_drop(&drc1));
    assert(_z_rc_strong_count(drc2._cnt) == 1);
    assert(_z_rc_weak_count(drc2._cnt) == 1);
    assert(drc2._val->foo == 42);
    assert(_dummy_rc_drop(&drc2));
}

void test_rc_eq(void) {
    _dummy_t val = {.foo = 42};
    _dummy_rc_t drc1 = _dummy_rc_new_from_val(&val);
    _dummy_rc_t drc2 = _dummy_rc_clone(&drc1);
    assert(_dummy_rc_eq(&drc1, &drc2));
    assert(!_dummy_rc_drop(&drc1));
    assert(_dummy_rc_drop(&drc2));
}

void test_rc_clone_as_ptr(void) {
    _dummy_t val = {.foo = 42};
    _dummy_rc_t drc1 = _dummy_rc_new_from_val(&val);
    _dummy_rc_t *drc2 = _dummy_rc_clone_as_ptr(&drc1);
    assert(drc2->_val != NULL);
    assert(!_Z_RC_IS_NULL(drc2));
    assert(_z_rc_strong_count(drc2->_cnt) == 2);
    assert(_z_rc_weak_count(drc2->_cnt) == 2);
    assert(_dummy_rc_eq(&drc1, drc2));
    assert(!_dummy_rc_drop(&drc1));
    assert(_dummy_rc_drop(drc2));
    z_free(drc2);
}

void test_rc_copy(void) {
    _dummy_t val = {.foo = 42};
    _dummy_rc_t drc1 = _dummy_rc_new_from_val(&val);
    _dummy_rc_t drc2 = _dummy_rc_null();
    assert(!_dummy_rc_eq(&drc1, &drc2));
    _dummy_rc_copy(&drc2, &drc1);
    assert(_z_rc_strong_count(drc2._cnt) == 2);
    assert(_z_rc_weak_count(drc2._cnt) == 2);
    assert(_dummy_rc_eq(&drc1, &drc2));
    assert(!_dummy_rc_drop(&drc2));
    assert(_dummy_rc_drop(&drc1));
}

void test_rc_clone_as_weak(void) {
    _dummy_t val = {.foo = 42};
    _dummy_rc_t drc1 = _dummy_rc_new_from_val(&val);
    _dummy_weak_t dwk1 = _dummy_rc_clone_as_weak(&drc1);
    assert(!_Z_RC_IS_NULL(&dwk1));
    assert(_z_rc_strong_count(dwk1._cnt) == 1);
    assert(_z_rc_weak_count(dwk1._cnt) == 2);

    assert(dwk1._val->foo == 42);
    assert(_dummy_rc_drop(&drc1));
    assert(_z_rc_strong_count(dwk1._cnt) == 0);
    assert(_z_rc_weak_count(dwk1._cnt) == 1);
    assert(_dummy_weak_drop(&dwk1));
}

void test_rc_clone_as_weak_ptr(void) {
    _dummy_t val = {.foo = 42};
    _dummy_rc_t drc1 = _dummy_rc_new_from_val(&val);
    _dummy_weak_t *dwk1 = _dummy_rc_clone_as_weak_ptr(&drc1);
    assert(dwk1 != NULL);
    assert(!_Z_RC_IS_NULL(dwk1));
    assert(_z_rc_strong_count(dwk1->_cnt) == 1);
    assert(_z_rc_weak_count(dwk1->_cnt) == 2);

    assert(_dummy_rc_drop(&drc1));
    assert(_z_rc_strong_count(dwk1->_cnt) == 0);
    assert(_z_rc_weak_count(dwk1->_cnt) == 1);
    assert(_dummy_weak_drop(dwk1));
    z_free(dwk1);
}

void test_weak_null(void) {
    _dummy_weak_t dwk = _dummy_weak_null();
    assert(dwk._val == NULL);
    assert(dwk._cnt == NULL);
}

void test_weak_clone(void) {
    _dummy_t val = {.foo = 42};
    _dummy_rc_t drc1 = _dummy_rc_new_from_val(&val);
    _dummy_weak_t dwk1 = _dummy_rc_clone_as_weak(&drc1);
    assert(_z_rc_strong_count(dwk1._cnt) == 1);
    assert(_z_rc_weak_count(dwk1._cnt) == 2);

    _dummy_weak_t dwk2 = _dummy_weak_clone(&dwk1);
    assert(_z_rc_strong_count(dwk2._cnt) == 1);
    assert(_z_rc_weak_count(dwk2._cnt) == 3);

    assert(_dummy_rc_drop(&drc1));
    assert(_z_rc_strong_count(dwk2._cnt) == 0);
    assert(_z_rc_weak_count(dwk2._cnt) == 2);

    assert(_dummy_weak_eq(&dwk1, &dwk2));
    assert(!_dummy_weak_drop(&dwk2));
    assert(_dummy_weak_drop(&dwk1));
}

void test_weak_copy(void) {
    _dummy_t val = {.foo = 42};
    _dummy_rc_t drc1 = _dummy_rc_new_from_val(&val);
    _dummy_weak_t dwk1 = _dummy_rc_clone_as_weak(&drc1);
    _dummy_weak_t dwk2 = _dummy_weak_null();
    assert(!_dummy_weak_eq(&dwk1, &dwk2));

    _dummy_weak_copy(&dwk2, &dwk1);
    assert(_dummy_weak_eq(&dwk1, &dwk2));
    assert(_z_rc_strong_count(dwk2._cnt) == 1);
    assert(_z_rc_weak_count(dwk2._cnt) == 3);

    assert(!_dummy_weak_drop(&dwk1));
    assert(!_dummy_weak_drop(&dwk2));
    assert(_dummy_rc_drop(&drc1));
}

void test_weak_upgrade(void) {
    _dummy_t val = {.foo = 42};
    _dummy_rc_t drc1 = _dummy_rc_new_from_val(&val);
    _dummy_weak_t dwk1 = _dummy_rc_clone_as_weak(&drc1);

    // Valid upgrade
    _dummy_rc_t drc2 = _dummy_weak_upgrade(&dwk1);
    assert(!_Z_RC_IS_NULL(&drc2));
    assert(_z_rc_strong_count(drc2._cnt) == 2);
    assert(_z_rc_weak_count(drc2._cnt) == 3);
    assert(!_dummy_rc_drop(&drc1));
    assert(_dummy_rc_drop(&drc2));

    // Failed upgrade
    _dummy_rc_t drc3 = _dummy_weak_upgrade(&dwk1);
    assert(_Z_RC_IS_NULL(&drc3));
    assert(_z_rc_strong_count(dwk1._cnt) == 0);
    assert(_z_rc_weak_count(dwk1._cnt) == 1);
    assert(_dummy_weak_drop(&dwk1));
}

void test_overflow(void) {
    _dummy_t val = {.foo = 42};
    _dummy_rc_t drc1 = _dummy_rc_new_from_val(&val);
    // Artificially set weak count to max value
    _dummy_inner_rc_t *dcnt = (_dummy_inner_rc_t *)drc1._cnt;
    dcnt->_strong_cnt = INT32_MAX;
    dcnt->_weak_cnt = INT32_MAX;

    _dummy_rc_t drc2 = _dummy_rc_clone(&drc1);
    assert(_Z_RC_IS_NULL(&drc2));

    _dummy_weak_t dwk1 = _dummy_rc_clone_as_weak(&drc1);
    assert(_Z_RC_IS_NULL(&dwk1));

    // Manual free to make asan happy, without long decresing
    free(drc1._val);
    free(drc1._cnt);
}

void test_decr(void) {
    _dummy_t val = {.foo = 42};
    _dummy_rc_t drc1 = _dummy_rc_new_from_val(&val);
    _dummy_rc_t drc2 = _dummy_rc_clone(&drc1);
    assert(!_dummy_rc_decr(&drc2));
    assert(_dummy_rc_decr(&drc1));
    free(drc1._val);
}

_Z_SIMPLE_REFCOUNT_DEFINE(_dummy, _dummy)

void test_simple_rc_null(void) {
    _dummy_simple_rc_t drc = _dummy_simple_rc_null();
    assert(drc._cnt == NULL);
    assert(drc._val == NULL);
}

void test_simple_rc_size(void) { assert(_dummy_simple_rc_size(NULL) == sizeof(_dummy_simple_rc_t)); }

void test_simple_rc_drop(void) {
    _dummy_simple_rc_t drc = _dummy_simple_rc_null();
    assert(!_dummy_simple_rc_drop(NULL));
    assert(!_dummy_simple_rc_drop(&drc));
}

void test_simple_rc_new(void) {
    _dummy_t *val = z_malloc(sizeof(_dummy_t));
    val->foo = 42;
    _dummy_simple_rc_t drc = _dummy_simple_rc_new(val);
    assert(!_Z_RC_IS_NULL(&drc));
    assert(_z_simple_rc_strong_count(drc._cnt) == 1);
    assert(drc._val->foo == 42);
    drc._val->foo = 0;
    assert(val->foo == 0);
    assert(_dummy_simple_rc_drop(&drc));
}

void test_simple_rc_new_from_val(void) {
    _dummy_t val = {.foo = 42};
    _dummy_simple_rc_t drc = _dummy_simple_rc_new_from_val(&val);
    assert(!_Z_RC_IS_NULL(&drc));
    assert(_z_simple_rc_strong_count(drc._cnt) == 1);
    assert(drc._val->foo == 42);
    drc._val->foo = 0;
    assert(val.foo == 42);
    assert(_dummy_simple_rc_drop(&drc));
}

void test_simple_rc_clone(void) {
    _dummy_t val = {.foo = 42};
    _dummy_simple_rc_t drc1 = _dummy_simple_rc_new_from_val(&val);
    assert(_z_simple_rc_strong_count(drc1._cnt) == 1);

    _dummy_simple_rc_t drc2 = _dummy_simple_rc_clone(&drc1);
    assert(!_Z_RC_IS_NULL(&drc2));
    assert(_z_simple_rc_strong_count(drc2._cnt) == 2);
    assert(_z_simple_rc_strong_count(drc2._cnt) == _z_simple_rc_strong_count(drc1._cnt));
    assert(drc2._val->foo == drc1._val->foo);

    assert(!_dummy_simple_rc_drop(&drc1));
    assert(_z_simple_rc_strong_count(drc2._cnt) == 1);
    assert(drc2._val->foo == 42);
    assert(_dummy_simple_rc_drop(&drc2));
}

void test_simple_rc_eq(void) {
    _dummy_t val = {.foo = 42};
    _dummy_simple_rc_t drc1 = _dummy_simple_rc_new_from_val(&val);
    _dummy_simple_rc_t drc2 = _dummy_simple_rc_clone(&drc1);
    assert(_dummy_simple_rc_eq(&drc1, &drc2));
    assert(!_dummy_simple_rc_drop(&drc1));
    assert(_dummy_simple_rc_drop(&drc2));
}

void test_simple_rc_clone_as_ptr(void) {
    _dummy_t val = {.foo = 42};
    _dummy_simple_rc_t drc1 = _dummy_simple_rc_new_from_val(&val);
    _dummy_simple_rc_t *drc2 = _dummy_simple_rc_clone_as_ptr(&drc1);
    assert(drc2->_val != NULL);
    assert(!_Z_RC_IS_NULL(drc2));
    assert(_dummy_simple_rc_count(drc2) == 2);
    assert(_dummy_simple_rc_eq(&drc1, drc2));
    assert(!_dummy_simple_rc_drop(&drc1));
    assert(_dummy_simple_rc_count(drc2) == 1);
    assert(_dummy_simple_rc_drop(drc2));
    z_free(drc2);
}

void test_simple_rc_copy(void) {
    _dummy_t val = {.foo = 42};
    _dummy_simple_rc_t drc1 = _dummy_simple_rc_new_from_val(&val);
    _dummy_simple_rc_t drc2 = _dummy_simple_rc_null();
    assert(!_dummy_simple_rc_eq(&drc1, &drc2));
    _dummy_simple_rc_copy(&drc2, &drc1);
    assert(_dummy_simple_rc_count(&drc2) == 2);
    assert(_dummy_simple_rc_eq(&drc1, &drc2));
    assert(!_dummy_simple_rc_drop(&drc2));
    assert(_dummy_simple_rc_drop(&drc1));
}

void test_simple_rc_decr(void) {
    _dummy_t val = {.foo = 42};
    _dummy_simple_rc_t drc1 = _dummy_simple_rc_new_from_val(&val);
    _dummy_simple_rc_t drc2 = _dummy_simple_rc_clone(&drc1);
    assert(!_dummy_simple_rc_decr(&drc2));
    assert(_dummy_simple_rc_decr(&drc1));
    // free manualy, to make asan happy, because counter already zero
    z_free(drc1._val);
}

int main(void) {
    test_rc_null();
    test_rc_size();
    test_rc_drop();
    test_rc_new();
    test_rc_new_from_val();
    test_rc_clone();
    test_rc_eq();
    test_rc_clone_as_ptr();
    test_rc_copy();
    test_rc_clone_as_weak();
    test_rc_clone_as_weak_ptr();
    test_weak_null();
    test_weak_clone();
    test_weak_copy();
    test_weak_upgrade();
    test_overflow();
    test_decr();

    test_simple_rc_null();
    test_simple_rc_size();
    test_simple_rc_drop();
    test_simple_rc_new();
    test_simple_rc_new_from_val();
    test_simple_rc_clone();
    test_simple_rc_eq();
    test_simple_rc_clone_as_ptr();
    test_simple_rc_copy();
    test_simple_rc_decr();

    return 0;
}
