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

_z_list_t *_z_list_push(_z_list_t *xs, void *x)
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

size_t _z_list_len(const _z_list_t *xs)
{
    size_t len = 0;
    _z_list_t *head = (_z_list_t *)xs;
    while (head != NULL)
    {
        len += 1;
        head = _z_list_tail(head);
    }
    return len;
}

_z_list_t *_z_list_pop(_z_list_t *xs, z_element_free_f f_f)
{
    _z_list_t *head = xs;
    xs = head->tail;
    f_f(&head->val);
    free(head);

    return xs;
}

_z_list_t *_z_list_drop_filter(_z_list_t *xs, z_element_free_f f_f, z_element_cmp_f c_f, void *left)
{
    _z_list_t *previous = xs;
    _z_list_t *current = xs;

    while (current != NULL)
    {
        if (c_f(left, current->val))
        {
            _z_list_t *this = current;

            // head removal
            if (this == xs)
            {
                xs = xs->tail;
                previous = xs;
            }
            // tail removal
            else if (this->tail == NULL)
            {
                previous->tail = NULL;
            }
            // middle removal
            else
            {
                previous->tail = this->tail;
            }

            current = this->tail;

            f_f(&this->val);
            free(this);
        }
        else
        {
            previous = current;
            current = current->tail;
        }
    }

    return xs;
}

int _z_list_cmp(const _z_list_t *right, const _z_list_t *left, z_element_cmp_f c_f)
{
    _z_list_t *l = (_z_list_t *)left;
    _z_list_t *r = (_z_list_t *)right;

    while (l != NULL && r != NULL)
    {
        int res = c_f(_z_list_head(l), _z_list_head(r));
        if (res != 0)
            return res;

        l = _z_list_tail(l);
        r = _z_list_tail(r);
    }

    // l and r should be both NULL
    return l == r;
}

_z_list_t *_z_list_clone(const _z_list_t *xs, z_element_clone_f f)
{
    _z_list_t *new = NULL;

    _z_list_t *head = (_z_list_t *)xs;
    while (head != NULL)
    {
        void *x = f(_z_list_head(head));
        new = _z_list_push(new, x);
        head = _z_list_tail(head);
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
        ptr = _z_list_pop(ptr, f);
    *xs = NULL;
}
