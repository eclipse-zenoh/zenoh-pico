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

#include "zenoh-pico/collections/list.h"

#include <assert.h>
#include <stddef.h>

#include "zenoh-pico/utils/logging.h"

/*-------- Inner single-linked list --------*/
_z_list_t *_z_list_of(void *x) {
    _z_list_t *xs = (_z_list_t *)z_malloc(sizeof(_z_list_t));
    if (xs == NULL) {
        _Z_ERROR("Failed to allocate list element.");
        return NULL;
    }
    xs->_val = x;
    xs->_tail = NULL;
    return xs;
}

_z_list_t *_z_list_push(_z_list_t *xs, void *x) {
    _z_list_t *lst = _z_list_of(x);
    lst->_tail = xs;
    return lst;
}

_z_list_t *_z_list_push_back(_z_list_t *xs, void *x) {
    _z_list_t *l = (_z_list_t *)xs;
    while (l != NULL && l->_tail != NULL) {
        l = l->_tail;
    }
    if (l == NULL) {
        l = _z_list_of(x);
        return l;
    } else {
        l->_tail = _z_list_of(x);
        return xs;
    }
}

void *_z_list_head(const _z_list_t *xs) { return xs->_val; }

_z_list_t *_z_list_tail(const _z_list_t *xs) { return xs->_tail; }

size_t _z_list_len(const _z_list_t *xs) {
    size_t len = 0;
    _z_list_t *l = (_z_list_t *)xs;
    while (l != NULL) {
        len = len + (size_t)1;
        l = _z_list_tail(l);
    }
    return len;
}

_z_list_t *_z_list_pop(_z_list_t *xs, z_element_free_f f_f, void **x) {
    _z_list_t *l = (_z_list_t *)xs;
    _z_list_t *head = xs;
    l = head->_tail;
    if (x != NULL) {
        *x = head->_val;
    } else {
        f_f(&head->_val);
    }
    z_free(head);

    return l;
}

_z_list_t *_z_list_find(const _z_list_t *xs, z_element_eq_f c_f, const void *e) {
    _z_list_t *l = (_z_list_t *)xs;
    _z_list_t *ret = NULL;
    while (l != NULL) {
        void *head = _z_list_head(l);
        if (c_f(e, head) == true) {
            ret = l;
            break;
        }
        l = _z_list_tail(l);
    }
    return ret;
}

_z_list_t *_z_list_drop_element(_z_list_t *list, _z_list_t *prev, z_element_free_f f_f) {
    _z_list_t *dropped = NULL;
    if (prev == NULL) {  // Head removal
        dropped = list;
        list = list->_tail;
    } else {  // Other cases
        dropped = prev->_tail;
        if (dropped != NULL) {
            prev->_tail = dropped->_tail;
        }
    }
    if (dropped != NULL) {
        f_f(&dropped->_val);
        z_free(dropped);
    }
    return list;
}

_z_list_t *_z_list_drop_filter(_z_list_t *xs, z_element_free_f f_f, z_element_eq_f c_f, const void *left) {
    _z_list_t *l = (_z_list_t *)xs;
    _z_list_t *previous = xs;
    _z_list_t *current = xs;

    while (current != NULL) {
        if (c_f(left, current->_val)) {
            _z_list_t *this_ = current;

            // head removal
            if (this_ == l) {
                l = l->_tail;
            }
            // tail removal
            else if (this_->_tail == NULL) {
                previous->_tail = NULL;
            }
            // middle removal
            else {
                previous->_tail = this_->_tail;
            }

            f_f(&this_->_val);
            z_free(this_);
            break;
        } else {
            previous = current;
            current = current->_tail;
        }
    }

    return l;
}

_z_list_t *_z_list_clone(const _z_list_t *xs, z_element_clone_f d_f) {
    _z_list_t *new = NULL;

    _z_list_t *head = (_z_list_t *)xs;
    while (head != NULL) {
        void *x = d_f(_z_list_head(head));
        new = _z_list_push(new, x);
        head = _z_list_tail(head);
    }

    return new;
}

/**
 * Free the list in deep. This function frees
 * the inner void * to the element of the list.
 */
void _z_list_free(_z_list_t **xs, z_element_free_f f) {
    _z_list_t *ptr = *xs;
    while (ptr != NULL) {
        ptr = _z_list_pop(ptr, f, NULL);
    }

    *xs = NULL;
}
