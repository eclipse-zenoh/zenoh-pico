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

/*-------- intmap --------*/
_z_i_map_t *_z_i_map_empty = NULL;

_z_i_map_t *_z_i_map_make(size_t capacity)
{
    _z_i_map_t *map = (_z_i_map_t *)malloc(sizeof(_z_i_map_t));
    map->capacity = capacity;
    map->len = 0;
    map->vals = (_z_list_t **)malloc(capacity * sizeof(_z_list_t *));
    for (size_t i = 0; i < map->capacity; i++)
        map->vals[i] = _z_list_empty;

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
            while (map->vals[i])
            {
                _z_i_map_entry_t *e = (_z_i_map_entry_t *)_z_list_head(map->vals[i]);
                free(e->value);
                free(e);
                map->vals[i] = _z_list_pop(map->vals[i]);
            }
        }
        free(map->vals);
        free(map);
    }
}
