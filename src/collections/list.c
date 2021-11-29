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
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/list.h"

/*-------- Inner single-linked list --------*/
_z_list_t *_z_list_of(void *x)
{
    _z_list_t *xs = (_z_list_t *)malloc(sizeof(_z_list_t));
    xs->val = x;
    xs->tail = NULL;
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
    while (xs != NULL)
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

_z_list_t *_z_list_drop_head(_z_list_t *xs, z_element_free_f f)
{
    _z_list_t *head = xs;
    xs = head->tail;
    f(&head->val);
    free(head);

    return xs;
}

_z_list_t *_z_list_drop_pos(_z_list_t *xs, size_t pos, z_element_free_f f)
{
    _z_list_t *head = xs;

    if (pos == 0)
    {
        return _z_list_drop_head(xs, f);
    }
    else
    {
        _z_list_t *previous = NULL;

        size_t idx = 0;
        while (idx < pos)
        {
            previous = xs;
            xs = xs->tail;
            idx++;
        }

        previous->tail = xs->tail;
        f(&xs->val);
        free(xs);

        return head;
    }
}

_z_list_t *_z_list_drop_filter(_z_list_t *xs, z_list_predicate predicate, void *arg, z_element_free_f f)
{
    _z_list_t *previous = xs;
    _z_list_t *current = xs;

    while (current != NULL)
    {
        if (predicate(current->val, arg) == 1)
        {
            // head removal
            if (current == xs)
            {
                xs = xs->tail;
                f(&current->val);
                free(current);
            }
            // tail removal
            else if (current->tail == NULL)
            {
                previous->tail = NULL;
                f(current->val);
                free(current);
            }
            // middle removal
            else
            {
                previous->tail = current->tail;
                f(&current->val);
                free(current);
            }
            break;
        }

        previous = current;
        current = current->tail;
    }

    return xs;
}

int _z_list_cmp(const _z_list_t *right, const _z_list_t *left, z_element_cmp_f f)
{
    _z_list_t *l = (_z_list_t *)left;
    _z_list_t *r = (_z_list_t *)right;

    while (l != NULL && r != NULL)
    {
        int res = f(_z_list_head(l), _z_list_head(r));
        if (res != 0)
            return res;

        l = _z_list_pop(l);
        r = _z_list_pop(r);
    }

    // l and r should be both NULL
    return (l == r) ? 0 : -1;
}

_z_list_t *_z_list_dup(const _z_list_t *xs, z_element_dup_f f)
{
    _z_list_t *new = NULL;

    _z_list_t *head = (_z_list_t *)xs;
    while (head != NULL)
    {
        void *x = f(_z_list_head(head));
        new = _z_list_cons(new, x);
        head = _z_list_pop(head);
    }

    return new;
}

/**
 * Free the list in deep. This function frees
 * the inner void * to the element of the list.
 */
void _z_list_free(_z_list_t **xs, z_element_free_f f)
{
    _z_list_t *ptr = (_z_list_t *)*xs;
    while (ptr != NULL)
    {
        ptr = _z_list_drop_head(ptr, f);
    }
    *xs = NULL;
}
