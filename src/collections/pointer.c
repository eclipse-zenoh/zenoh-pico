//
// Copyright (c) 2022 ZettaScale Technology
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
#include "zenoh-pico/collections/pointer.h"

#include <stdbool.h>

#include "zenoh-pico/collections/element.h"

_z_elem_sptr_t _z_elem_sptr_null(void) {
    _z_elem_sptr_t p;
    p._ptr = NULL;
    p._cnt = NULL;
    return p;
}

_z_elem_sptr_t _z_elem_sptr_new(void *val) {
    _z_elem_sptr_t p;
    p._ptr = val;
    p._cnt = (atomic_uint *)z_malloc(sizeof(atomic_uint));
    atomic_store_explicit(p._cnt, 1, memory_order_relaxed);
    return p;
}

void *_z_elem_sptr_get(_z_elem_sptr_t *p) {
    void *val = NULL;
    if (atomic_load_explicit(p->_cnt, memory_order_acquire) != 0) {
        val = p->_ptr;
    }
    return val;
}

_z_elem_sptr_t _z_elem_sptr_clone(const _z_elem_sptr_t *p) {
    _z_elem_sptr_t c;
    c._ptr = p->_ptr;
    c._cnt = p->_cnt;
    atomic_fetch_add_explicit(p->_cnt, 1, memory_order_relaxed);
    return c;
}

// static inline name##_sptr_t *name##_sptr_clone_as_ptr(const name##_sptr_t *p) {
//     name##_sptr_t *c = (name##_sptr_t *)z_malloc(sizeof(name##_sptr_t));
//     c->_cnt = p->_cnt;
//     c->ptr = p->ptr;
//     atomic_fetch_add_explicit(p->_cnt, 1, memory_order_relaxed);
//     return c;
// }

int _z_elem_sptr_eq(const _z_elem_sptr_t *left, const _z_elem_sptr_t *right) { return (left->_ptr == right->_ptr); }

_Bool _z_elem_sptr_drop(_z_elem_sptr_t *p, z_element_free_f f_f) {
    unsigned int c = atomic_fetch_sub_explicit(p->_cnt, 1, memory_order_release);
    _Bool dropped = c == 1;
    if (dropped) {
        atomic_thread_fence(memory_order_acquire);
        f_f(&p->_ptr);
        z_free(p->_cnt);
    }
    return dropped;
}