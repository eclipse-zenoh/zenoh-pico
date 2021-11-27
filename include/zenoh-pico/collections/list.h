/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#ifndef ZENOH_PICO_COLLECTIONS_LIST_H
#define ZENOH_PICO_COLLECTIONS_LIST_H

#include <stdint.h>
#include <stdio.h>
#include "zenoh-pico/collections/element.h"

/*-------- Single-linked List --------*/
/**
 * A single-linked list.
 *
 *  Members:
 *   void *val: The pointer to the inner value.
 *   struct z_list *tail: A pointer to the next element in the list.
 */
typedef int (*z_list_predicate)(void *, void *);

typedef struct _z_l_t
{
    void *val;
    struct _z_l_t *tail;
} _z_list_t;

_z_list_t *_z_list_of(void *x);
_z_list_t *_z_list_cons(_z_list_t *xs, void *x);

void *_z_list_head(_z_list_t *xs);
_z_list_t *_z_list_tail(_z_list_t *xs);

size_t _z_list_len(_z_list_t *xs);

_z_list_t *_z_list_pop(_z_list_t *xs);

_z_list_t *_z_list_drop_head(_z_list_t *xs, z_element_free_f f);
_z_list_t *_z_list_drop_pos(_z_list_t *xs, size_t pos, z_element_free_f f);
_z_list_t *_z_list_drop_filter(_z_list_t *xs, z_list_predicate p, void *arg, z_element_free_f f);

int _z_list_cmp(const _z_list_t *right, const _z_list_t *left, z_element_cmp_f f);
_z_list_t *_z_list_clone(const _z_list_t *xs, z_element_clone_f f);
void _z_list_free(_z_list_t **xs, z_element_free_f f);

/*-------- String list --------*/
typedef _z_list_t z_str_list_t;

#define z_str_list_make() NULL
#define z_str_list_push(list, elem) _z_list_cons(list, elem)
#define z_str_list_pop(list, elem) _z_list_pop(list, elem)
#define z_str_list_len(list) _z_list_len(list)
#define z_str_list_drop_head(list) (z_str_t) _z_list_drop_head(list, z_element_free_str)
#define z_str_list_drop_pos(list, pos) _z_list_drop_pos(list, pos, z_element_free_str)
#define z_str_list_drop_filter(list, pos) _z_list_drop_head(list, pos, z_element_free_str)
#define z_str_list_cmp(left, right) _z_list_cmp(left, right, z_element_cmp_str)
#define z_str_list_clone(list) _z_list_clone(list, z_element_clone_str)
#define z_str_list_free(list) _z_list_free(list, z_element_free_str)

#endif /* ZENOH_PICO_COLLECTIONS_LIST_H */
