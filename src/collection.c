/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
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
#include "zenoh-pico/private/collection.h"
#include "zenoh-pico/types.h"

/*-------- Vec --------*/
inline _z_vec_t _z_vec_make(size_t capacity)
{
    _z_vec_t v;
    v._capacity = capacity;
    v._len = 0;
    v._val = (void **)malloc(sizeof(void *) * capacity);
    return v;
}

_z_vec_t _z_vec_clone(const _z_vec_t *v)
{
    _z_vec_t u = _z_vec_make(v->_capacity);
    for (size_t i = 0; i < v->_len; ++i)
        _z_vec_append(&u, v->_val[i]);
    return u;
}

void _z_vec_free_inner(_z_vec_t *v)
{
    free(v->_val);
    v->_len = 0;
    v->_capacity = 0;
    v->_val = NULL;
}

void _z_vec_free(_z_vec_t *v)
{
    for (size_t i = 0; i < v->_len; ++i)
        free(v->_val[i]);
    _z_vec_free_inner(v);
}

inline size_t _z_vec_len(const _z_vec_t *v) { return v->_len; }

void _z_vec_append(_z_vec_t *v, void *e)
{
    if (v->_len == v->_capacity)
    {
        // Allocate a new vector
        size_t _capacity = 2 * v->_capacity;
        void **_val = (void **)malloc(_capacity * sizeof(void *));
        memcpy(_val, v->_val, v->_capacity * sizeof(void *));
        // Free the old vector
        free(v->_val);
        // Update the current vector
        v->_val = _val;
        v->_capacity = _capacity;
    }

    v->_val[v->_len] = e;
    v->_len++;
}

const void *_z_vec_get(const _z_vec_t *v, size_t i)
{
    assert(i < v->_len);
    return v->_val[i];
}

void _z_vec_set(_z_vec_t *v, size_t i, void *e)
{
    assert(i < v->_capacity);
    v->_val[i] = e;
}

/*-------- Linked List --------*/
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

void _z_list_free(_z_list_t **xs)
{
    _z_list_t *current = *xs;
    _z_list_t *tail;
    if (current != _z_list_empty)
    {
        tail = _z_list_tail(current);
        free(current);
        current = tail;
    }
    *xs = 0;
}

/*-------- Int Map --------*/
_z_i_map_t *_z_i_map_empty = NULL;

_z_i_map_t *_z_i_map_make(size_t capacity)
{
    _z_i_map_t *map = (_z_i_map_t *)malloc(sizeof(_z_i_map_t));
    map->vals = (_z_list_t **)malloc(capacity * sizeof(_z_list_t *));
    memset(map->vals, 0, capacity * sizeof(_z_list_t *));
    map->capacity = capacity;
    map->len = 0;

    return map;
}

size_t _z_i_map_capacity(_z_i_map_t *map)
{
    return map->capacity;
}

size_t _z_i_map_len(_z_i_map_t *map)
{
    return map->len;
}

void _z_i_map_set(_z_i_map_t *map, size_t k, void *v)
{
    _z_i_map_entry_t *entry = NULL;

    // Compute the hash
    size_t idx = k % map->capacity;
    // Get the list associated to the hash
    _z_list_t *xs = map->vals[idx];

    if (xs == _z_list_empty)
    {
        entry = (_z_i_map_entry_t *)malloc(sizeof(_z_i_map_entry_t));
        entry->key = k;
        entry->value = v;
        map->vals[idx] = _z_list_cons(_z_list_empty, entry);
        map->len++;
    }
    else
    {
        while (xs != _z_list_empty)
        {
            entry = (_z_i_map_entry_t *)xs->val;
            if (entry->key == k)
            {
                entry->value = v;
                break;
            }
            else
            {
                xs = xs->tail;
            }
        }

        if (xs == _z_list_empty)
        {
            entry = (_z_i_map_entry_t *)malloc(sizeof(_z_i_map_entry_t));
            entry->key = k;
            entry->value = v;
            map->vals[idx] = _z_list_cons(map->vals[idx], entry);
            map->len++;
        }
    }
}

void *_z_i_map_get(_z_i_map_t *map, size_t k)
{
    _z_i_map_entry_t *entry = NULL;
    size_t idx = k % map->capacity;
    _z_list_t *xs = map->vals[idx];

    while (xs != _z_list_empty)
    {
        entry = (_z_i_map_entry_t *)xs->val;
        if (entry->key == k)
            return entry->value;
        xs = xs->tail;
    }

    return NULL;
}

int _z_i_map_key_predicate(void *current, void *desired)
{
    _z_i_map_entry_t *c = (_z_i_map_entry_t *)current;
    _z_i_map_entry_t *d = (_z_i_map_entry_t *)desired;
    if (c->key == d->key)
        return 1;
    return 0;
}

void _z_i_map_remove(_z_i_map_t *map, size_t k)
{
    size_t idx = k % map->capacity;
    _z_i_map_entry_t e;
    e.key = k;
    size_t l = _z_list_len(map->vals[idx]);
    map->vals[idx] = _z_list_remove(map->vals[idx], _z_i_map_key_predicate, &e);
    map->len -= l - _z_list_len(map->vals[idx]);
}

void _z_i_map_free(_z_i_map_t *map)
{
    if (map != _z_i_map_empty)
    {
        for (size_t i = 0; i < map->capacity; i++)
        {
            if (map->vals[i] != _z_list_empty)
                _z_list_free(&map->vals[i]);
        }
        free(map->vals);
        free(map);
    }
}

/*-------- bytes --------*/
void _z_bytes_init(z_bytes_t *bs, size_t capacity)
{
    bs->val = (uint8_t *)malloc(capacity * sizeof(uint8_t));
    bs->len = capacity;
}

z_bytes_t _z_bytes_make(size_t capacity)
{
    z_bytes_t bs;
    _z_bytes_init(&bs, capacity);
    return bs;
}

void _z_bytes_free(z_bytes_t *bs)
{
    free((uint8_t *)bs->val);
}

void _z_bytes_copy(z_bytes_t *dst, const z_bytes_t *src)
{
    _z_bytes_init(dst, src->len);
    memcpy((uint8_t *)dst->val, src->val, src->len);
}

void _z_bytes_move(z_bytes_t *dst, z_bytes_t *src)
{
    dst->val = src->val;
    dst->len = src->len;

    src->val = NULL;
    src->len = 0;
}

void _z_bytes_reset(z_bytes_t *bs)
{
    bs->val = NULL;
    bs->len = 0;
}

/*-------- string --------*/
void _z_string_reset(z_string_t *str)
{
    str->val = NULL;
    str->len = 0;
}

z_string_t _z_string_from_bytes(z_bytes_t *bs)
{
    z_string_t s;
    s.len = 2 * bs->len;
    char *s_val = (char *)malloc(s.len * sizeof(char) + 1);

    char c[] = "0123456789ABCDEF";
    for (size_t i = 0; i < bs->len; i++)
    {
        s_val[2 * i] = c[(bs->val[i] & 0xF0) >> 4];
        s_val[2 * i + 1] = c[bs->val[i] & 0x0F];
    }
    s_val[s.len] = '\0';
    s.val = s_val;

    return s;
}

/*-------- str_array --------*/
void _z_str_array_init(z_str_array_t *sa, size_t len)
{
    z_str_t **val = (z_str_t **)&sa->val;
    *val = (z_str_t *)malloc(len * sizeof(z_str_t));
    sa->len = len;
}

z_str_array_t _z_str_array_make(size_t len)
{
    z_str_array_t sa;
    _z_str_array_init(&sa, len);
    return sa;
}

void _z_str_array_free(z_str_array_t *sa)
{
    for (size_t i = 0; i < sa->len; i++)
        free((z_str_t)sa->val[i]);
    free((z_str_t *)sa->val);
}

void _z_str_array_copy(z_str_array_t *dst, const z_str_array_t *src)
{
    _z_str_array_init(dst, src->len);
    for (size_t i = 0; i < src->len; i++)
        ((z_str_t *)dst->val)[i] = strdup(src->val[i]);
    dst->len = src->len;
}

void _z_str_array_move(z_str_array_t *dst, z_str_array_t *src)
{
    dst->val = src->val;
    dst->len = src->len;

    src->val = NULL;
    src->len = 0;
}
