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
#include <string.h>

#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

/*-------- Inner single-linked list --------*/
static _z_list_t *_z_list_new(void *x) {
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
    _z_list_t *lst = _z_list_new(x);
    if (lst == NULL) {
        return xs;
    }
    lst->_tail = xs;
    return lst;
}

_z_list_t *_z_list_push_back(_z_list_t *xs, void *x) {
    if (xs == NULL) {
        return _z_list_new(x);
    }
    _z_list_t *l = xs;
    while (l->_tail != NULL) {
        l = l->_tail;
    }
    l->_tail = _z_list_new(x);
    return xs;
}

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
    if (xs == NULL) {
        return xs;
    }
    _z_list_t *head = xs;
    _z_list_t *l = head->_tail;
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
        if (c_f(e, head)) {
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

/*-------- Inner sized single-linked list --------*/
typedef struct _z_slist_node_data_t {
    _z_slist_t *next;
} _z_slist_node_data_t;

#define NODE_DATA_SIZE sizeof(_z_slist_t *)

static inline _z_slist_node_data_t *_z_slist_node_data(const _z_slist_t *list) { return (_z_slist_node_data_t *)list; }

static inline void *_z_slist_node_value(const _z_slist_t *node) {
    return (void *)_z_ptr_u8_offset((uint8_t *)node, (ptrdiff_t)NODE_DATA_SIZE);
}

static _z_slist_t *_z_slist_new(void *value, size_t value_size, z_element_copy_f d_f, bool use_elem_f) {
    size_t node_size = NODE_DATA_SIZE + value_size;
    _z_slist_t *node = (_z_slist_t *)z_malloc(node_size);
    if (node == NULL) {
        _Z_ERROR("Failed to allocate list element.");
        return node;
    }
    memset(node, 0, NODE_DATA_SIZE);
    if (use_elem_f) {
        d_f(_z_slist_node_value(node), value);
    } else {
        memcpy(_z_slist_node_value(node), value, value_size);
    }
    return node;
}

_z_slist_t *_z_slist_push(_z_slist_t *node, void *value, size_t value_size, z_element_copy_f d_f, bool use_elem_f) {
    _z_slist_t *new_node = _z_slist_new(value, value_size, d_f, use_elem_f);
    if (new_node == NULL) {
        return node;
    }
    _z_slist_node_data_t *node_data = _z_slist_node_data(new_node);
    node_data->next = node;
    return new_node;
}

_z_slist_t *_z_slist_push_back(_z_slist_t *node, void *value, size_t value_size, z_element_copy_f d_f,
                               bool use_elem_f) {
    if (node == NULL) {
        return _z_slist_new(value, value_size, d_f, use_elem_f);
    }
    _z_slist_node_data_t *node_data = _z_slist_node_data(node);
    while (node_data->next != NULL) {
        node_data = _z_slist_node_data(node_data->next);
    }
    node_data->next = _z_slist_new(value, value_size, d_f, use_elem_f);
    return node;
}

void *_z_slist_value(const _z_slist_t *node) { return _z_slist_node_value(node); }

_z_slist_t *_z_slist_next(const _z_slist_t *node) {
    _z_slist_node_data_t *node_data = _z_slist_node_data(node);
    return _z_slist_node_data(node_data)->next;
}

size_t _z_slist_len(const _z_slist_t *node) {
    size_t len = 0;
    _z_slist_node_data_t *node_data = _z_slist_node_data(node);
    while (node_data != NULL) {
        len += (size_t)1;
        node_data = _z_slist_node_data(node_data->next);
    }
    return len;
}

_z_slist_t *_z_slist_pop(_z_slist_t *node, z_element_clear_f f_f) {
    if (node == NULL) {
        return node;
    }
    _z_slist_t *next_node = _z_slist_node_data(node)->next;
    f_f(_z_slist_node_value(node));
    z_free(node);
    return next_node;
}

_z_slist_t *_z_slist_find(const _z_slist_t *node, z_element_eq_f c_f, const void *target_val) {
    _z_slist_t *curr_node = (_z_slist_t *)node;
    _z_slist_node_data_t *node_data = _z_slist_node_data(curr_node);
    while (node_data != NULL) {
        void *value = _z_slist_node_value(curr_node);
        if (c_f(target_val, value)) {
            return curr_node;
        }
        curr_node = node_data->next;
        node_data = _z_slist_node_data(curr_node);
    }
    return NULL;
}

_z_slist_t *_z_slist_drop_element(_z_slist_t *list, _z_slist_t *prev, z_element_clear_f f_f) {
    _z_slist_t *dropped = NULL;
    if (prev == NULL) {  // Head removal
        dropped = list;
        list = _z_slist_node_data(list)->next;
    } else {  // Other cases
        dropped = _z_slist_node_data(prev)->next;
        if (dropped != NULL) {
            _z_slist_node_data(prev)->next = _z_slist_node_data(dropped)->next;
        }
    }
    if (dropped != NULL) {
        f_f(_z_slist_node_value(dropped));
        z_free(dropped);
    }
    return list;
}

_z_slist_t *_z_slist_drop_filter(_z_slist_t *head, z_element_clear_f f_f, z_element_eq_f c_f, const void *target_val) {
    _z_slist_t *previous = head;
    _z_slist_t *current = head;
    while (current != NULL) {
        if (c_f(target_val, _z_slist_node_value(current))) {
            _z_slist_t *this_ = current;
            if (this_ == head) {  // head removal
                head = _z_slist_node_data(head)->next;
            } else if (_z_slist_node_data(this_)->next == NULL) {  // tail removal
                _z_slist_node_data(previous)->next = NULL;
            } else {  // middle removal
                _z_slist_node_data(previous)->next = _z_slist_node_data(this_)->next;
            }
            f_f(_z_slist_node_data(this_));
            z_free(this_);
            break;
        } else {
            previous = current;
            current = _z_slist_node_data(current)->next;
        }
    }
    return head;
}

_z_slist_t *_z_slist_clone(const _z_slist_t *node, size_t value_size, z_element_copy_f d_f, bool use_elem_f) {
    _z_slist_t *new_node = NULL;
    _z_slist_t *curr_node = (_z_slist_t *)node;
    while (curr_node != NULL) {
        void *value = _z_slist_node_value(curr_node);
        new_node = _z_slist_push(new_node, value, value_size, d_f, use_elem_f);
        curr_node = _z_slist_node_data(curr_node)->next;
    }
    return new_node;
}

void _z_slist_free(_z_slist_t **node, z_element_clear_f f) {
    _z_slist_t *ptr = *node;
    while (ptr != NULL) {
        ptr = _z_slist_pop(ptr, f);
    }
    *node = NULL;
}