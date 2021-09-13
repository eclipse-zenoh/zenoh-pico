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
 *     ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "zenoh-pico/utils/collections.h"
#include "zenoh-pico/system/private/common.h"
#include "zenoh-pico/utils/types.h"

/*-------- linked list --------*/
_z_list_t *_z_list_empty = NULL;

_z_list_t *_z_list_of(void *x)
{
    _z_list_t *xs = (_z_list_t *)malloc(sizeof(_z_list_t));
    memset(xs, 0, sizeof(_z_list_t));
    xs->val = x;
    return xs;
}

_z_list_t *_z_list_cons(_z_list_t *xs, void *x)
{
    _z_list_t *lst = _z_list_of(x);
    lst->tail = xs;
    return lst;
}

void *_z_list_head(_z_list_t *xs)
{
    return xs->val;
}

_z_list_t *_z_list_tail(_z_list_t *xs)
{
    return xs->tail;
}

size_t _z_list_len(_z_list_t *xs)
{
    size_t len = 0;
    while (xs != _z_list_empty)
    {
        len += 1;
        xs = _z_list_tail(xs);
    }
    return len;
}

_z_list_t *_z_list_pop(_z_list_t *xs)
{
    _z_list_t *head = xs;
    xs = head->tail;
    free(head);
    return xs;
}

_z_list_t *_z_list_drop_val(_z_list_t *xs, size_t position)
{
    assert(position < _z_list_len(xs));
    _z_list_t *head = xs;
    _z_list_t *previous = 0;
    if (position == 0)
    {
        xs = head->tail;
        free(head);
        return xs;
    }

    size_t idx = 0;
    while (idx < position)
    {
        previous = xs;
        xs = xs->tail;
        idx++;
    }

    previous->tail = xs->tail;
    free(xs);
    return head;
}

_z_list_t *_z_list_remove(_z_list_t *xs, _z_list_predicate predicate, void *arg)
{
    _z_list_t *prev = xs;
    _z_list_t *current = xs;
    if (xs == _z_list_empty)
        return xs;

    while (current != _z_list_empty)
    {
        // head removal
        if (predicate(current->val, arg) == 1)
        {
            if (xs == current)
            {
                xs = xs->tail;
                free(current);
                return xs;
            }
            // tail removal
            else if (current->val == _z_list_empty)
            {
                prev->tail = _z_list_empty;
                free(current);
            }
            // middle removal
            else
            {
                prev->tail = current->tail;
                free(current);
            }
            break;
        }
        prev = current;
        current = current->tail;
    }
    return xs;
}

/**
 * Free the list. This function does not free
 * the inner void * to the element of the list.
 */
void _z_list_free(_z_list_t *xs)
{
    while (xs)
    {
        xs = _z_list_pop(xs);
    }
}

/**
 * Free the list in deep. This function frees
 * the inner void * to the element of the list.
 */
void _z_list_free_deep(_z_list_t *xs)
{
    while (xs)
    {
        void *x = _z_list_head(xs);
        free(x);
        xs = _z_list_pop(xs);
    }
}
