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
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "zenoh-pico/collections/string.h"
// aa
#include "zenoh-pico/collections/ring.h"

_Z_RING_DEFINE(_z_str, char)

#undef NDEBUG
#include <assert.h>

void print_ring(_z_ring_t *r) {
    printf("Ring { capacity: %zu, r_idx: %zu, w_idx: %zu, len: %zu }\n", _z_ring_capacity(r), r->_r_idx, r->_w_idx,
           _z_ring_len(r));
}

void ring_test(void) {
    char *a = "a";
    char *b = "b";
    char *c = "c";
    char *d = "d";

    _z_str_ring_t r = _z_str_ring_make(3);
    print_ring(&r);
    assert(_z_str_ring_is_empty(&r));

    // One
    _z_str_ring_push(&r, a);
    print_ring(&r);
    assert(_z_str_ring_len(&r) == 1);

    char *s = _z_str_ring_pull(&r);
    print_ring(&r);
    assert(strcmp(a, s) == 0);
    assert(_z_str_ring_is_empty(&r));

    s = _z_str_ring_pull(&r);
    print_ring(&r);
    assert(s == NULL);
    assert(_z_str_ring_is_empty(&r));

    // Two
    s = _z_str_ring_push(&r, a);
    print_ring(&r);
    assert(s == NULL);
    assert(_z_str_ring_len(&r) == 1);

    s = _z_str_ring_push(&r, b);
    print_ring(&r);
    assert(s == NULL);
    assert(_z_str_ring_len(&r) == 2);

    s = _z_str_ring_push(&r, c);
    print_ring(&r);
    assert(s == NULL);
    assert(_z_str_ring_len(&r) == 3);
    assert(_z_str_ring_is_full(&r));

    s = _z_str_ring_push(&r, d);
    print_ring(&r);
    assert(strcmp(d, s) == 0);
    assert(_z_str_ring_len(&r) == 3);
    assert(_z_str_ring_is_full(&r));

    s = _z_str_ring_push_force(&r, d);
    print_ring(&r);
    assert(strcmp(a, s) == 0);
    assert(_z_str_ring_len(&r) == 3);
    assert(_z_str_ring_is_full(&r));

    s = _z_str_ring_push_force(&r, d);
    print_ring(&r);
    assert(strcmp(b, s) == 0);
    assert(_z_str_ring_len(&r) == 3);
    assert(_z_str_ring_is_full(&r));

    s = _z_str_ring_push_force(&r, d);
    print_ring(&r);
    assert(strcmp(c, s) == 0);
    assert(_z_str_ring_len(&r) == 3);
    assert(_z_str_ring_is_full(&r));

    s = _z_str_ring_pull(&r);
    print_ring(&r);
    assert(strcmp(d, s) == 0);
    assert(_z_str_ring_len(&r) == 2);

    s = _z_str_ring_pull(&r);
    print_ring(&r);
    assert(strcmp(d, s) == 0);
    assert(_z_str_ring_len(&r) == 1);

    s = _z_str_ring_pull(&r);
    print_ring(&r);
    assert(strcmp(d, s) == 0);
    assert(_z_str_ring_is_empty(&r));
}

int main(void) { ring_test(); }
