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

#include <stdlib.h>
#include "zenoh-pico/utils/collections.h"
#include "zenoh-pico/utils/types.h"

/*-------- intmap --------*/
z_i_map_t *z_i_map_empty = NULL;

z_i_map_t *z_i_map_make(size_t capacity)
{
    z_i_map_t *map = (z_i_map_t *)malloc(sizeof(z_i_map_t));
    map->capacity = capacity;
    map->len = 0;
    map->vals = (z_list_t **)malloc(capacity * sizeof(z_list_t *));
    for (size_t i = 0; i < map->capacity; i++)
        map->vals[i] = z_list_empty;

    return map;
}

size_t z_i_map_capacity(z_i_map_t *map)
{
    return map->capacity;
}

size_t z_i_map_len(z_i_map_t *map)
{
    return map->len;
}

void z_i_map_set(z_i_map_t *map, size_t k, void *v)
{
    z_i_map_entry_t *entry = NULL;

    // Compute the hash
    size_t idx = k % map->capacity;
    // Get the list associated to the hash
    z_list_t *xs = map->vals[idx];

    if (xs == z_list_empty)
    {
        entry = (z_i_map_entry_t *)malloc(sizeof(z_i_map_entry_t));
        entry->key = k;
        entry->value = v;
        map->vals[idx] = z_list_cons(z_list_empty, entry);
        map->len++;
    }
    else
    {
        while (xs != z_list_empty)
        {
            entry = (z_i_map_entry_t *)xs->val;
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

        if (xs == z_list_empty)
        {
            entry = (z_i_map_entry_t *)malloc(sizeof(z_i_map_entry_t));
            entry->key = k;
            entry->value = v;
            map->vals[idx] = z_list_cons(map->vals[idx], entry);
            map->len++;
        }
    }
}

void *z_i_map_get(z_i_map_t *map, size_t k)
{
    z_i_map_entry_t *entry = NULL;
    size_t idx = k % map->capacity;
    z_list_t *xs = map->vals[idx];

    while (xs != z_list_empty)
    {
        entry = (z_i_map_entry_t *)xs->val;
        if (entry->key == k)
            return entry->value;
        xs = xs->tail;
    }

    return NULL;
}

int z_i_map_key_predicate(void *current, void *desired)
{
    z_i_map_entry_t *c = (z_i_map_entry_t *)current;
    z_i_map_entry_t *d = (z_i_map_entry_t *)desired;
    if (c->key == d->key)
        return 1;
    return 0;
}

void z_i_map_remove(z_i_map_t *map, size_t k)
{
    size_t idx = k % map->capacity;
    z_i_map_entry_t e;
    e.key = k;
    size_t l = z_list_len(map->vals[idx]);
    map->vals[idx] = z_list_remove(map->vals[idx], z_i_map_key_predicate, &e);
    map->len -= l - z_list_len(map->vals[idx]);
}

void z_i_map_free(z_i_map_t *map)
{
    if (map != z_i_map_empty)
    {
        for (size_t i = 0; i < map->capacity; i++)
        {
            while (map->vals[i])
            {
                z_i_map_entry_t *e = (z_i_map_entry_t *)z_list_head(map->vals[i]);
                free(e->value);
                free(e);
                map->vals[i] = z_list_pop(map->vals[i]);
            }
        }
        free(map->vals);
        free(map);
    }
}
