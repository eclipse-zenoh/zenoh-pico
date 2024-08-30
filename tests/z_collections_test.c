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

#include "zenoh-pico/collections/fifo.h"
#include "zenoh-pico/collections/lifo.h"
#include "zenoh-pico/collections/ring.h"
#include "zenoh-pico/collections/string.h"

#undef NDEBUG
#include <assert.h>

char *a = "a";
char *b = "b";
char *c = "c";
char *d = "d";

// RING
_Z_RING_DEFINE(_z_str, char)

void print_ring(_z_str_ring_t *r) {
    printf("Ring { capacity: %zu, r_idx: %zu, w_idx: %zu, len: %zu }\n", _z_str_ring_capacity(r), r->_r_idx, r->_w_idx,
           _z_str_ring_len(r));
}

void ring_test(void) {
    _z_str_ring_t r = _z_str_ring_make(3);
    print_ring(&r);
    assert(_z_str_ring_is_empty(&r));

    // One
    char *s = _z_str_ring_push(&r, a);
    print_ring(&r);
    assert(s == NULL);
    assert(_z_str_ring_len(&r) == 1);

    s = _z_str_ring_pull(&r);
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
    printf("%s == %s\n", d, s);
    assert(strcmp(d, s) == 0);
    assert(_z_str_ring_len(&r) == 3);
    assert(_z_str_ring_is_full(&r));

    s = _z_str_ring_push_force(&r, d);
    print_ring(&r);
    printf("%s == %s\n", a, s);
    assert(strcmp(a, s) == 0);
    assert(_z_str_ring_len(&r) == 3);
    assert(_z_str_ring_is_full(&r));

    s = _z_str_ring_push_force(&r, d);
    print_ring(&r);
    printf("%s == %s\n", b, s);
    assert(strcmp(b, s) == 0);
    assert(_z_str_ring_len(&r) == 3);
    assert(_z_str_ring_is_full(&r));

    s = _z_str_ring_push_force(&r, d);
    print_ring(&r);
    printf("%s == %s\n", c, s);
    assert(strcmp(c, s) == 0);
    assert(_z_str_ring_len(&r) == 3);
    assert(_z_str_ring_is_full(&r));

    s = _z_str_ring_pull(&r);
    print_ring(&r);
    printf("%s == %s\n", d, s);
    assert(strcmp(d, s) == 0);
    assert(_z_str_ring_len(&r) == 2);

    s = _z_str_ring_pull(&r);
    print_ring(&r);
    printf("%s == %s\n", d, s);
    assert(strcmp(d, s) == 0);
    assert(_z_str_ring_len(&r) == 1);

    s = _z_str_ring_pull(&r);
    print_ring(&r);
    printf("%s == %s\n", d, s);
    assert(strcmp(d, s) == 0);
    assert(_z_str_ring_is_empty(&r));

    _z_str_ring_clear(&r);
}

void ring_test_init_free(void) {
    _z_str_ring_t *r = (_z_str_ring_t *)malloc(sizeof(_z_str_ring_t));
    _z_str_ring_init(r, 1);
    assert(r != NULL);

    char *str = (char *)calloc(1, sizeof(char));
    _z_str_ring_push_force_drop(r, str);

    _z_str_ring_free(&r);
    assert(r == NULL);
}

// LIFO
_Z_LIFO_DEFINE(_z_str, char)

void print_lifo(_z_str_lifo_t *r) {
    printf("Lifo { capacity: %zu, len: %zu }\n", _z_str_lifo_capacity(r), _z_str_lifo_len(r));
}

void lifo_test(void) {
    _z_str_lifo_t r = _z_str_lifo_make(3);
    print_lifo(&r);
    assert(_z_str_lifo_is_empty(&r));

    // One
    char *s = _z_str_lifo_push(&r, a);
    print_lifo(&r);
    assert(s == NULL);
    assert(_z_str_lifo_len(&r) == 1);

    s = _z_str_lifo_pull(&r);
    print_lifo(&r);
    printf("%s == %s\n", a, s);
    assert(strcmp(a, s) == 0);
    assert(_z_str_lifo_is_empty(&r));

    s = _z_str_lifo_pull(&r);
    print_lifo(&r);
    assert(s == NULL);
    assert(_z_str_lifo_is_empty(&r));

    // Two
    s = _z_str_lifo_push(&r, a);
    print_lifo(&r);
    assert(s == NULL);
    assert(_z_str_lifo_len(&r) == 1);

    s = _z_str_lifo_push(&r, b);
    print_lifo(&r);
    assert(s == NULL);
    assert(_z_str_lifo_len(&r) == 2);

    s = _z_str_lifo_push(&r, c);
    print_lifo(&r);
    assert(s == NULL);
    assert(_z_str_lifo_len(&r) == 3);
    assert(_z_str_lifo_is_full(&r));

    s = _z_str_lifo_push(&r, d);
    print_lifo(&r);
    printf("%s == %s\n", d, s);
    assert(strcmp(d, s) == 0);
    assert(_z_str_lifo_len(&r) == 3);
    assert(_z_str_lifo_is_full(&r));

    s = _z_str_lifo_pull(&r);
    print_lifo(&r);
    printf("%s == %s\n", c, s);
    assert(strcmp(c, s) == 0);
    assert(_z_str_lifo_len(&r) == 2);

    s = _z_str_lifo_pull(&r);
    print_lifo(&r);
    printf("%s == %s\n", b, s);
    assert(strcmp(b, s) == 0);
    assert(_z_str_lifo_len(&r) == 1);

    s = _z_str_lifo_pull(&r);
    print_lifo(&r);
    printf("%s == %s\n", a, s);
    assert(strcmp(a, s) == 0);
    assert(_z_str_lifo_is_empty(&r));

    _z_str_lifo_clear(&r);
}

void lifo_test_init_free(void) {
    _z_str_lifo_t *r = (_z_str_lifo_t *)malloc(sizeof(_z_str_lifo_t));
    _z_str_lifo_init(r, 1);
    assert(r != NULL);

    char *str = (char *)calloc(1, sizeof(char));
    _z_str_lifo_push_drop(r, str);

    _z_str_lifo_free(&r);
    assert(r == NULL);
}

// FIFO
_Z_FIFO_DEFINE(_z_str, char)

void print_fifo(_z_str_fifo_t *r) {
    printf("Fifo { capacity: %zu, len: %zu }\n", _z_str_fifo_capacity(r), _z_str_fifo_len(r));
}

void fifo_test(void) {
    _z_str_fifo_t r = _z_str_fifo_make(3);
    print_fifo(&r);
    assert(_z_str_fifo_is_empty(&r));

    // One
    char *s = _z_str_fifo_push(&r, a);
    print_fifo(&r);
    assert(s == NULL);
    assert(_z_str_fifo_len(&r) == 1);

    s = _z_str_fifo_pull(&r);
    print_fifo(&r);
    printf("%s == %s\n", a, s);
    assert(strcmp(a, s) == 0);
    assert(_z_str_fifo_is_empty(&r));

    s = _z_str_fifo_pull(&r);
    print_fifo(&r);
    assert(s == NULL);
    assert(_z_str_fifo_is_empty(&r));

    // Two
    s = _z_str_fifo_push(&r, a);
    print_fifo(&r);
    assert(s == NULL);
    assert(_z_str_fifo_len(&r) == 1);

    s = _z_str_fifo_push(&r, b);
    print_fifo(&r);
    assert(s == NULL);
    assert(_z_str_fifo_len(&r) == 2);

    s = _z_str_fifo_push(&r, c);
    print_fifo(&r);
    assert(s == NULL);
    assert(_z_str_fifo_len(&r) == 3);
    assert(_z_str_fifo_is_full(&r));

    s = _z_str_fifo_push(&r, d);
    print_fifo(&r);
    printf("%s == %s\n", d, s);
    assert(strcmp(d, s) == 0);
    assert(_z_str_fifo_len(&r) == 3);
    assert(_z_str_fifo_is_full(&r));

    s = _z_str_fifo_pull(&r);
    print_fifo(&r);
    printf("%s == %s\n", a, s);
    assert(strcmp(a, s) == 0);
    assert(_z_str_fifo_len(&r) == 2);

    s = _z_str_fifo_pull(&r);
    print_fifo(&r);
    printf("%s == %s\n", b, s);
    assert(strcmp(b, s) == 0);
    assert(_z_str_fifo_len(&r) == 1);

    s = _z_str_fifo_pull(&r);
    print_fifo(&r);
    printf("%s == %s\n", c, s);
    assert(strcmp(c, s) == 0);
    assert(_z_str_fifo_is_empty(&r));

    _z_str_fifo_clear(&r);
}

void fifo_test_init_free(void) {
    _z_str_fifo_t *r = (_z_str_fifo_t *)malloc(sizeof(_z_str_fifo_t));
    _z_str_fifo_init(r, 1);
    assert(r != NULL);

    char *str = (char *)calloc(1, sizeof(char));
    _z_str_fifo_push_drop(r, str);

    _z_str_fifo_free(&r);
    assert(r == NULL);
}

int main(void) {
    ring_test();
    ring_test_init_free();
    lifo_test();
    lifo_test_init_free();
    fifo_test();
    fifo_test_init_free();
}
