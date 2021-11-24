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

#ifndef ZENOH_PICO_UTILS_COLLECTION_LIST_H
#define ZENOH_PICO_UTILS_COLLECTION_LIST_H

#include <stdint.h>
#include <stdio.h>

/*-------- Linked List --------*/
/**
 * A single-linked list.
 *
 *  Members:
 *   void *val: The pointer to the inner value.
 *   struct z_list *tail: A pointer to the next element in the list.
 */
typedef int (*z_list_predicate)(void *, void *);
typedef struct _z_list
{
    void *val;
    struct _z_list *tail;
} z_list_t;

extern z_list_t *z_list_empty;

z_list_t *z_list_of(void *x);
z_list_t *z_list_cons(z_list_t *xs, void *x);

void *z_list_head(z_list_t *xs);
z_list_t *z_list_tail(z_list_t *xs);

size_t z_list_len(z_list_t *xs);
z_list_t *z_list_remove(z_list_t *xs, z_list_predicate p, void *arg);

z_list_t *z_list_pop(z_list_t *xs);
z_list_t *z_list_drop_val(z_list_t *xs, size_t position);
void z_list_free(z_list_t *xs);
void z_list_free_deep(z_list_t *xs);

#endif /* ZENOH_PICO_UTILS_COLLECTION_LIST_H */
