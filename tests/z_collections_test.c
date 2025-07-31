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
#include "zenoh-pico/collections/sortedmap.h"
#include "zenoh-pico/collections/string.h"

#undef NDEBUG
#include <assert.h>

char *a = "a";
char *b = "b";
char *c = "c";
char *d = "d";

// RING
_Z_RING_DEFINE(_z_str, char)

// SORTED MAP
_Z_SORTEDMAP_DEFINE(_z_str, _z_str, char, char)

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

void ring_iterator_test(void) {
#define TEST_RING(ring, values, n)                                                              \
    {                                                                                           \
        _z_str_ring_iterator_t iter = _z_str_ring_iterator_make(&ring);                         \
        _z_str_ring_reverse_iterator_t reverse_iter = _z_str_ring_reverse_iterator_make(&ring); \
                                                                                                \
        for (int i = 0; i < n; i++) {                                                           \
            assert(_z_str_ring_iterator_next(&iter));                                           \
            assert(strcmp(_z_str_ring_iterator_value(&iter), values[i]) == 0);                  \
        }                                                                                       \
                                                                                                \
        for (int i = n - 1; i >= 0; i--) {                                                      \
            assert(_z_str_ring_reverse_iterator_next(&reverse_iter));                           \
            assert(strcmp(_z_str_ring_reverse_iterator_value(&reverse_iter), values[i]) == 0);  \
        }                                                                                       \
                                                                                                \
        assert(!_z_str_ring_iterator_next(&iter));                                              \
        assert(!_z_str_ring_reverse_iterator_next(&reverse_iter));                              \
    }

    _z_str_ring_t ring = _z_str_ring_make(4);

    char *values[] = {"A", "B", "C", "D", "E", "F"};

    assert(_z_str_ring_push(&ring, values[0]) == NULL);
    // { "A" }
    TEST_RING(ring, values, 1);

    assert(_z_str_ring_push(&ring, values[1]) == NULL);
    // { "A", "B" }
    TEST_RING(ring, values, 2);

    assert(_z_str_ring_push(&ring, values[2]) == NULL);
    // { "A", "B", "C" }
    TEST_RING(ring, values, 3);

    assert(_z_str_ring_push(&ring, values[3]) == NULL);
    // { "A", "B", "C", "D" }
    TEST_RING(ring, values, 4);

    assert(_z_str_ring_pull(&ring) != NULL);
    // { "B", "C", "D" }
    TEST_RING(ring, (values + 1), 3);
    assert(_z_str_ring_push(&ring, values[4]) == NULL);
    // { "B", "C", "D", "E" }
    TEST_RING(ring, (values + 1), 4);

    assert(_z_str_ring_pull(&ring) != NULL);
    // { "C", "D", "E" }
    TEST_RING(ring, (values + 2), 3);
    assert(_z_str_ring_push(&ring, values[5]) == NULL);
    // { "C", "D", "E", "F" }
    TEST_RING(ring, (values + 2), 4);

    assert(_z_str_ring_pull(&ring) != NULL);
    // { "D", "E", "F" }
    TEST_RING(ring, (values + 3), 3);

    assert(_z_str_ring_pull(&ring) != NULL);
    // { "E", "F" }
    TEST_RING(ring, (values + 4), 2);

    assert(_z_str_ring_pull(&ring) != NULL);
    // { "F" }
    TEST_RING(ring, (values + 5), 1);

    assert(_z_str_ring_pull(&ring) != NULL);
    // {}
    TEST_RING(ring, values, 0);

    _z_str_ring_clear(&ring);

#undef TEST_RING
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

void int_map_iterator_test(void) {
    _z_str_intmap_t map;

    map = _z_str_intmap_make();
    _z_str_intmap_insert(&map, 10, _z_str_clone("A"));
    _z_str_intmap_insert(&map, 20, _z_str_clone("B"));
    _z_str_intmap_insert(&map, 30, _z_str_clone("C"));
    _z_str_intmap_insert(&map, 40, _z_str_clone("D"));

#define TEST_MAP(map)                                                      \
    {                                                                      \
        _z_str_intmap_iterator_t iter = _z_str_intmap_iterator_make(&map); \
        assert(_z_str_intmap_iterator_next(&iter));                        \
        assert(_z_str_intmap_iterator_key(&iter) == 20);                   \
        assert(strcmp(_z_str_intmap_iterator_value(&iter), "B") == 0);     \
                                                                           \
        assert(_z_str_intmap_iterator_next(&iter));                        \
        assert(_z_str_intmap_iterator_key(&iter) == 40);                   \
        assert(strcmp(_z_str_intmap_iterator_value(&iter), "D") == 0);     \
                                                                           \
        assert(_z_str_intmap_iterator_next(&iter));                        \
        assert(_z_str_intmap_iterator_key(&iter) == 10);                   \
        assert(strcmp(_z_str_intmap_iterator_value(&iter), "A") == 0);     \
                                                                           \
        assert(_z_str_intmap_iterator_next(&iter));                        \
        assert(_z_str_intmap_iterator_key(&iter) == 30);                   \
        assert(strcmp(_z_str_intmap_iterator_value(&iter), "C") == 0);     \
                                                                           \
        assert(!_z_str_intmap_iterator_next(&iter));                       \
    }

    TEST_MAP(map);

    _z_str_intmap_t map2 = _z_str_intmap_clone(&map);

    TEST_MAP(map2);

    _z_str_intmap_clear(&map);
    _z_str_intmap_clear(&map2);

#undef TEST_MAP
}

void int_map_iterator_deletion_test(void) {
    _z_str_intmap_t map;

    map = _z_str_intmap_make();
    _z_str_intmap_insert(&map, 10, _z_str_clone("A"));
    _z_str_intmap_insert(&map, 20, _z_str_clone("B"));
    _z_str_intmap_insert(&map, 30, _z_str_clone("C"));
    _z_str_intmap_insert(&map, 40, _z_str_clone("D"));

    _z_str_intmap_iterator_t iter = _z_str_intmap_iterator_make(&map);
    _z_str_intmap_iterator_next(&iter);
    for (size_t s = 4; s != 0; s--) {
        assert(s == _z_str_intmap_len(&map));
        size_t key = _z_str_intmap_iterator_key(&iter);
        assert(strlen(_z_str_intmap_iterator_value(&iter)) == 1);
        _z_str_intmap_iterator_next(&iter);
        _z_str_intmap_remove(&map, key);
    }
    _z_str_intmap_clear(&map);
}

static bool slist_eq_f(const void *left, const void *right) { return strcmp((char *)left, (char *)right) == 0; }

void slist_test(void) {
    char *values[] = {"test1", "test2", "test3"};
    _z_slist_t *slist = NULL;
    // Test empty
    assert(_z_slist_is_empty(slist));
    assert(_z_slist_len(slist) == 0);

    // Add value test
    slist = _z_slist_push(slist, values[0], strlen(values[0]) + 1, _z_noop_copy, false);
    assert(!_z_slist_is_empty(slist));
    assert(_z_slist_len(slist) == 1);
    assert(strcmp(values[0], (char *)_z_slist_value(slist)) == 0);
    assert(_z_slist_next(slist) == NULL);

    slist = _z_slist_push(slist, values[1], strlen(values[1]) + 1, _z_noop_copy, false);
    assert(_z_slist_len(slist) == 2);
    assert(strcmp(values[1], (char *)_z_slist_value(slist)) == 0);
    assert(strcmp(values[0], (char *)_z_slist_value(_z_slist_next(slist))) == 0);

    // Push back test
    slist = _z_slist_push_back(slist, values[2], strlen(values[2]) + 1, _z_noop_copy, false);
    assert(_z_slist_len(slist) == 3);
    assert(strcmp(values[2], (char *)_z_slist_value(_z_slist_next(_z_slist_next(slist)))) == 0);

    // Pop test
    slist = _z_slist_pop(slist, _z_noop_clear);
    assert(_z_slist_len(slist) == 2);
    assert(strcmp(values[0], (char *)_z_slist_value(slist)) == 0);
    assert(strcmp(values[2], (char *)_z_slist_value(_z_slist_next(slist))) == 0);
    slist = _z_slist_pop(slist, _z_noop_clear);
    assert(_z_slist_len(slist) == 1);
    assert(strcmp(values[2], (char *)_z_slist_value(slist)) == 0);
    assert(strlen(values[2]) == 5);
    slist = _z_slist_pop(slist, _z_noop_clear);
    assert(_z_slist_is_empty(slist));

    // Drop element test
    for (size_t i = 0; i < _ZP_ARRAY_SIZE(values); i++) {
        slist = _z_slist_push(slist, values[i], strlen(values[i]) + 1, _z_noop_copy, false);
    }
    // Drop second element
    slist = _z_slist_drop_element(slist, slist, _z_noop_clear);
    assert(_z_slist_len(slist) == 2);
    assert(strcmp(values[2], (char *)_z_slist_value(slist)) == 0);
    assert(strcmp(values[0], (char *)_z_slist_value(_z_slist_next(slist))) == 0);

    // Free test
    _z_slist_free(&slist, _z_noop_clear);
    assert(_z_slist_is_empty(slist));

    // Push empty test
    slist = _z_slist_push_empty(slist, strlen(values[0]) + 1);
    assert(_z_slist_len(slist) == 1);
    char *val = (char *)_z_slist_value(slist);
    strcpy(val, values[0]);
    assert(strcmp(values[0], (char *)_z_slist_value(slist)) == 0);
    _z_slist_free(&slist, _z_noop_clear);

    // Drop filter test
    for (size_t i = 0; i < _ZP_ARRAY_SIZE(values); i++) {
        slist = _z_slist_push(slist, values[i], strlen(values[i]) + 1, _z_noop_copy, false);
    }
    slist = _z_slist_drop_filter(slist, _z_noop_clear, slist_eq_f, values[1]);
    assert(_z_slist_len(slist) == 2);
    assert(strcmp(values[2], (char *)_z_slist_value(slist)) == 0);
    assert(strcmp(values[0], (char *)_z_slist_value(_z_slist_next(slist))) == 0);
    _z_slist_free(&slist, _z_noop_clear);

    // Find test
    for (size_t i = 0; i < _ZP_ARRAY_SIZE(values); i++) {
        slist = _z_slist_push(slist, values[i], strlen(values[i]) + 1, _z_noop_copy, false);
    }
    _z_slist_t *elem = NULL;
    elem = _z_slist_find(slist, slist_eq_f, "bob");
    assert(elem == NULL);
    elem = _z_slist_find(slist, slist_eq_f, values[2]);
    assert(elem != NULL);
    _z_slist_free(&slist, _z_noop_clear);

    // Clone test
    for (size_t i = 0; i < _ZP_ARRAY_SIZE(values); i++) {
        slist = _z_slist_push(slist, values[i], strlen(values[i]) + 1, _z_noop_copy, false);
    }
    _z_slist_t *clone = NULL;
    clone = _z_slist_clone(slist, strlen(values[0]), _z_noop_copy, false);
    assert(_z_slist_len(slist) == 3);
    assert(strcmp(values[2], (char *)_z_slist_value(slist)) == 0);
    assert(strcmp(values[1], (char *)_z_slist_value(_z_slist_next(slist))) == 0);
    assert(strcmp(values[0], (char *)_z_slist_value(_z_slist_next(_z_slist_next(slist)))) == 0);
    _z_slist_free(&slist, _z_noop_clear);
    _z_slist_free(&clone, _z_noop_clear);
}

void sorted_map_iterator_test(void) {
    _z_str__z_str_sortedmap_t map;

    map = _z_str__z_str_sortedmap_make();
    _z_str__z_str_sortedmap_insert(&map, _z_str_clone("4"), _z_str_clone("D"));
    _z_str__z_str_sortedmap_insert(&map, _z_str_clone("1"), _z_str_clone("A"));
    _z_str__z_str_sortedmap_insert(&map, _z_str_clone("3"), _z_str_clone("C"));
    _z_str__z_str_sortedmap_insert(&map, _z_str_clone("2"), _z_str_clone("B"));

#define TEST_SORTEDMAP(map)                                                                    \
    {                                                                                          \
        assert(_z_str__z_str_sortedmap_len(&map) == 4);                                        \
        _z_str__z_str_sortedmap_iterator_t iter = _z_str__z_str_sortedmap_iterator_make(&map); \
                                                                                               \
        assert(_z_str__z_str_sortedmap_iterator_next(&iter));                                  \
        assert(strcmp(_z_str__z_str_sortedmap_iterator_key(&iter), "1") == 0);                 \
        assert(strcmp(_z_str__z_str_sortedmap_iterator_value(&iter), "A") == 0);               \
                                                                                               \
        assert(_z_str__z_str_sortedmap_iterator_next(&iter));                                  \
        assert(strcmp(_z_str__z_str_sortedmap_iterator_key(&iter), "2") == 0);                 \
        assert(strcmp(_z_str__z_str_sortedmap_iterator_value(&iter), "B") == 0);               \
                                                                                               \
        assert(_z_str__z_str_sortedmap_iterator_next(&iter));                                  \
        assert(strcmp(_z_str__z_str_sortedmap_iterator_key(&iter), "3") == 0);                 \
        assert(strcmp(_z_str__z_str_sortedmap_iterator_value(&iter), "C") == 0);               \
                                                                                               \
        assert(_z_str__z_str_sortedmap_iterator_next(&iter));                                  \
        assert(strcmp(_z_str__z_str_sortedmap_iterator_key(&iter), "4") == 0);                 \
        assert(strcmp(_z_str__z_str_sortedmap_iterator_value(&iter), "D") == 0);               \
                                                                                               \
        assert(!_z_str__z_str_sortedmap_iterator_next(&iter));                                 \
    }

    TEST_SORTEDMAP(map);

    _z_str__z_str_sortedmap_t map2 = _z_str__z_str_sortedmap_clone(&map);

    TEST_SORTEDMAP(map2);

    _z_str__z_str_sortedmap_clear(&map);
    _z_str__z_str_sortedmap_clear(&map2);

#undef TEST_SORTEDMAP
}

void sorted_map_iterator_deletion_test(void) {
    _z_str__z_str_sortedmap_t map;

    map = _z_str__z_str_sortedmap_make();
    _z_str__z_str_sortedmap_insert(&map, _z_str_clone("4"), _z_str_clone("D"));
    _z_str__z_str_sortedmap_insert(&map, _z_str_clone("1"), _z_str_clone("A"));
    _z_str__z_str_sortedmap_insert(&map, _z_str_clone("3"), _z_str_clone("C"));
    _z_str__z_str_sortedmap_insert(&map, _z_str_clone("2"), _z_str_clone("B"));

    _z_str__z_str_sortedmap_iterator_t iter = _z_str__z_str_sortedmap_iterator_make(&map);
    _z_str__z_str_sortedmap_iterator_next(&iter);
    for (size_t s = 4; s != 0; s--) {
        assert(s == _z_str__z_str_sortedmap_len(&map));
        char *key = _z_str__z_str_sortedmap_iterator_key(&iter);
        // SAFETY: returned _z_str_t should be null-terminated.
        // Flawfinder: ignore [CWE-126]
        assert(strlen(_z_str__z_str_sortedmap_iterator_value(&iter)) == 1);
        _z_str__z_str_sortedmap_iterator_next(&iter);
        _z_str__z_str_sortedmap_remove(&map, key);
    }
    _z_str__z_str_sortedmap_clear(&map);
}

void sorted_map_replace_test(void) {
    _z_str__z_str_sortedmap_t map = _z_str__z_str_sortedmap_make();
    _z_str__z_str_sortedmap_insert(&map, _z_str_clone("key"), _z_str_clone("old"));
    assert(strcmp(_z_str__z_str_sortedmap_get(&map, "key"), "old") == 0);

    _z_str__z_str_sortedmap_insert(&map, _z_str_clone("key"), _z_str_clone("new"));
    assert(strcmp(_z_str__z_str_sortedmap_get(&map, "key"), "new") == 0);

    _z_str__z_str_sortedmap_clear(&map);
}

void sorted_map_missing_key_test(void) {
    _z_str__z_str_sortedmap_t map = _z_str__z_str_sortedmap_make();
    assert(_z_str__z_str_sortedmap_get(&map, "absent") == NULL);
    _z_str__z_str_sortedmap_clear(&map);
}

void sorted_map_empty_test(void) {
    _z_str__z_str_sortedmap_t map = _z_str__z_str_sortedmap_make();
    assert(_z_str__z_str_sortedmap_is_empty(&map));
    assert(_z_str__z_str_sortedmap_len(&map) == 0);

    _z_str__z_str_sortedmap_iterator_t iter = _z_str__z_str_sortedmap_iterator_make(&map);
    assert(!_z_str__z_str_sortedmap_iterator_next(&iter));

    _z_str__z_str_sortedmap_clear(&map);
}

void sorted_map_pop_first_test(void) {
    _z_str__z_str_sortedmap_t map;
    _z_str__z_str_sortedmap_init(&map);
    _z_str__z_str_sortedmap_insert(&map, _z_str_clone("3"), _z_str_clone("three"));
    _z_str__z_str_sortedmap_insert(&map, _z_str_clone("1"), _z_str_clone("one"));
    _z_str__z_str_sortedmap_insert(&map, _z_str_clone("2"), _z_str_clone("two"));

    _z_str__z_str_sortedmap_entry_t *entry = _z_str__z_str_sortedmap_pop_first(&map);
    assert(entry != NULL);
    assert(strcmp(_z_str__z_str_sortedmap_entry_key(entry), "1") == 0);
    assert(strcmp(_z_str__z_str_sortedmap_entry_val(entry), "one") == 0);
    _z_str__z_str_sortedmap_entry_free(&entry);

    assert(_z_str__z_str_sortedmap_len(&map) == 2);

    _z_str__z_str_sortedmap_clear(&map);
}

void sorted_map_copy_move_test(void) {
    _z_str__z_str_sortedmap_t src = _z_str__z_str_sortedmap_make();
    _z_str__z_str_sortedmap_insert(&src, _z_str_clone("key"), _z_str_clone("value"));

    _z_str__z_str_sortedmap_t dst = _z_str__z_str_sortedmap_make();
    assert(_z_str__z_str_sortedmap_copy(&dst, &src) == _Z_RES_OK);
    assert(strcmp(_z_str__z_str_sortedmap_get(&dst, "key"), "value") == 0);

    _z_str__z_str_sortedmap_clear(&src);
    assert(_z_str__z_str_sortedmap_move(&src, &dst) == _Z_RES_OK);
    assert(strcmp(_z_str__z_str_sortedmap_get(&src, "key"), "value") == 0);

    _z_str__z_str_sortedmap_clear(&src);
}

void sorted_map_free_test(void) {
    _z_str__z_str_sortedmap_t *map_ptr = z_malloc(sizeof(_z_str__z_str_sortedmap_t));
    assert(map_ptr != NULL);

    *map_ptr = _z_str__z_str_sortedmap_make();
    _z_str__z_str_sortedmap_insert(map_ptr, _z_str_clone("K"), _z_str_clone("V"));

    _z_str__z_str_sortedmap_free(&map_ptr);
    assert(map_ptr == NULL);  // should be nullified after free
}

void sorted_map_stress_test(void) {
    _z_str__z_str_sortedmap_t map = _z_str__z_str_sortedmap_make();
    char key[16], val[16];
    for (int i = 100; i >= 1; i--) {
        snprintf(key, sizeof(key), "%03d", i);
        snprintf(val, sizeof(val), "val%d", i);
        _z_str__z_str_sortedmap_insert(&map, _z_str_clone(key), _z_str_clone(val));
    }

    _z_str__z_str_sortedmap_iterator_t iter = _z_str__z_str_sortedmap_iterator_make(&map);
    int expected = 1;
    while (_z_str__z_str_sortedmap_iterator_next(&iter)) {
        char expected_key[16];
        snprintf(expected_key, sizeof(expected_key), "%03d", expected);
        assert(strcmp(_z_str__z_str_sortedmap_iterator_key(&iter), expected_key) == 0);
        expected++;
    }
    assert(expected == 101);  // 1..100
    _z_str__z_str_sortedmap_clear(&map);
}

int main(void) {
    ring_test();
    ring_test_init_free();
    lifo_test();
    lifo_test_init_free();
    fifo_test();
    fifo_test_init_free();

    int_map_iterator_test();
    int_map_iterator_deletion_test();
    ring_iterator_test();

    slist_test();

    sorted_map_iterator_test();
    sorted_map_iterator_deletion_test();
    sorted_map_replace_test();
    sorted_map_missing_key_test();
    sorted_map_empty_test();
    sorted_map_pop_first_test();
    sorted_map_copy_move_test();
    sorted_map_free_test();
    sorted_map_stress_test();
}
