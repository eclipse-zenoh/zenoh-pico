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
#include "zenoh-pico/system/platform-common.h"

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

void test_rc_null(void) {
    _dummy_rc_t drc = _dummy_rc_null();
    assert(drc.in == NULL);
}

void test_rc_size(void) { assert(_dummy_rc_size(NULL) == sizeof(_dummy_rc_t)); }

void test_rc_drop(void) {
    _dummy_rc_t drc = _dummy_rc_null();
    assert(!_dummy_rc_drop(NULL));
    assert(!_dummy_rc_drop(&drc));
}

void test_rc_new(void) {
    _dummy_rc_t drc = _dummy_rc_new();
    assert(drc.in != NULL);
    assert(drc.in->_strong_cnt == 1);
    assert(drc.in->_weak_cnt == 1);
    assert(drc.in->val.foo == 0);
    assert(_dummy_rc_drop(&drc));
}

void test_rc_new_from_val(void) {
    _dummy_t val = {.foo = 42};
    _dummy_rc_t drc = _dummy_rc_new_from_val(val);
    assert(drc.in != NULL);
    assert(drc.in->_strong_cnt == 1);
    assert(drc.in->_weak_cnt == 1);
    assert(drc.in->val.foo == 42);
    assert(_dummy_rc_drop(&drc));
}

void test_rc_clone(void) {
    _dummy_t val = {.foo = 42};
    _dummy_rc_t drc1 = _dummy_rc_new_from_val(val);
    assert(drc1.in->_strong_cnt == 1);
    assert(drc1.in->_weak_cnt == 1);

    _dummy_rc_t drc2 = _dummy_rc_clone(&drc1);
    assert(drc2.in != NULL);
    assert(drc2.in->_strong_cnt == 2);
    assert(drc2.in->_weak_cnt == 2);
    assert(drc2.in->_strong_cnt == drc1.in->_strong_cnt);
    assert(drc2.in->_weak_cnt == drc1.in->_weak_cnt);
    assert(drc2.in->val.foo == drc1.in->val.foo);

    assert(!_dummy_rc_drop(&drc1));
    assert(drc2.in->_strong_cnt == 1);
    assert(drc2.in->_weak_cnt == 1);
    assert(drc2.in->val.foo == 42);
    assert(_dummy_rc_drop(&drc2));
}

void test_rc_eq(void) {
    _dummy_rc_t drc1 = _dummy_rc_new();
    _dummy_rc_t drc2 = _dummy_rc_clone(&drc1);
    assert(_dummy_rc_eq(&drc1, &drc2));
    assert(!_dummy_rc_drop(&drc1));
    assert(_dummy_rc_drop(&drc2));
}

void test_rc_clone_as_ptr(void) {
    _dummy_rc_t drc1 = _dummy_rc_new();
    _dummy_rc_t *drc2 = _dummy_rc_clone_as_ptr(&drc1);
    assert(drc2 != NULL);
    assert(drc2->in != NULL);
    assert(drc2->in->_strong_cnt == 2);
    assert(drc2->in->_weak_cnt == 2);
    assert(_dummy_rc_eq(&drc1, drc2));
    assert(!_dummy_rc_drop(&drc1));
    assert(_dummy_rc_drop(drc2));
    z_free(drc2);
}

void test_rc_copy(void) {
    _dummy_rc_t drc1 = _dummy_rc_new();
    _dummy_rc_t drc2 = _dummy_rc_null();
    assert(!_dummy_rc_eq(&drc1, &drc2));
    _dummy_rc_copy(&drc2, &drc1);
    assert(drc2.in->_strong_cnt == 2);
    assert(drc2.in->_weak_cnt == 2);
    assert(_dummy_rc_eq(&drc1, &drc2));
    assert(!_dummy_rc_drop(&drc2));
    assert(_dummy_rc_drop(&drc1));
}

void test_rc_clone_as_weak(void) {
    _dummy_rc_t drc1 = _dummy_rc_new();
    _dummy_weak_t dwk1 = _dummy_rc_clone_as_weak(&drc1);
    assert(dwk1.in != NULL);
    assert(dwk1.in->_strong_cnt == 1);
    assert(dwk1.in->_weak_cnt == 2);

    assert(dwk1.in->val.foo == 0);
    assert(!_dummy_rc_drop(&drc1));
    assert(dwk1.in->_strong_cnt == 0);
    assert(dwk1.in->_weak_cnt == 1);
    assert(dwk1.in->val.foo == FOO_CLEARED_VALUE);
    assert(_dummy_weak_drop(&dwk1));
}

void test_rc_clone_as_weak_ptr(void) {
    _dummy_rc_t drc1 = _dummy_rc_new();
    _dummy_weak_t *dwk1 = _dummy_rc_clone_as_weak_ptr(&drc1);
    assert(dwk1 != NULL);
    assert(dwk1->in != NULL);
    assert(dwk1->in->_strong_cnt == 1);
    assert(dwk1->in->_weak_cnt == 2);

    assert(!_dummy_rc_drop(&drc1));
    assert(dwk1->in->_strong_cnt == 0);
    assert(dwk1->in->_weak_cnt == 1);
    assert(_dummy_weak_drop(dwk1));
    z_free(dwk1);
}

void test_weak_null(void) {
    _dummy_weak_t dwk = _dummy_weak_null();
    assert(dwk.in == NULL);
}

void test_weak_clone(void) {
    _dummy_rc_t drc1 = _dummy_rc_new();
    _dummy_weak_t dwk1 = _dummy_rc_clone_as_weak(&drc1);
    assert(dwk1.in->_strong_cnt == 1);
    assert(dwk1.in->_weak_cnt == 2);

    _dummy_weak_t dwk2 = _dummy_weak_clone(&dwk1);
    assert(dwk2.in->_strong_cnt == 1);
    assert(dwk2.in->_weak_cnt == 3);

    assert(!_dummy_rc_drop(&drc1));
    assert(dwk2.in->_strong_cnt == 0);
    assert(dwk2.in->_weak_cnt == 2);

    assert(_dummy_weak_eq(&dwk1, &dwk2));
    assert(!_dummy_weak_drop(&dwk2));
    assert(_dummy_weak_drop(&dwk1));
}

void test_weak_copy(void) {
    _dummy_rc_t drc1 = _dummy_rc_new();
    _dummy_weak_t dwk1 = _dummy_rc_clone_as_weak(&drc1);
    _dummy_weak_t dwk2 = _dummy_weak_null();
    assert(!_dummy_weak_eq(&dwk1, &dwk2));

    _dummy_weak_copy(&dwk2, &dwk1);
    assert(_dummy_weak_eq(&dwk1, &dwk2));
    assert(dwk2.in->_strong_cnt == 1);
    assert(dwk2.in->_weak_cnt == 3);

    assert(!_dummy_weak_drop(&dwk1));
    assert(!_dummy_weak_drop(&dwk2));
    assert(_dummy_rc_drop(&drc1));
}

void test_weak_upgrade(void) {
    _dummy_rc_t drc1 = _dummy_rc_new();
    _dummy_weak_t dwk1 = _dummy_rc_clone_as_weak(&drc1);

    // Valid upgrade
    _dummy_rc_t drc2 = _dummy_weak_upgrade(&dwk1);
    assert(drc2.in != NULL);
    assert(drc2.in->_strong_cnt == 2);
    assert(drc2.in->_weak_cnt == 3);
    assert(!_dummy_rc_drop(&drc1));
    assert(!_dummy_rc_drop(&drc2));

    // Failed upgrade
    _dummy_rc_t drc3 = _dummy_weak_upgrade(&dwk1);
    assert(drc3.in == NULL);
    assert(_dummy_weak_drop(&dwk1));
}

void test_overflow(void) {
    _dummy_rc_t drc1 = _dummy_rc_new();
    // Artificially set weak count to max value
    drc1.in->_weak_cnt = INT32_MAX;

    _dummy_rc_t drc2 = _dummy_rc_clone(&drc1);
    assert(drc2.in == NULL);

    _dummy_weak_t dwk1 = _dummy_rc_clone_as_weak(&drc1);
    assert(dwk1.in == NULL);
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
    return 0;
}
