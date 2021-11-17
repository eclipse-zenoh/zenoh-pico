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
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/collections/intmap.h"

/*------------------ int-list map ------------------*/
void zn_int_list_map_entry_free(void **s)
{
    z_list_t *ptr = (z_list_t *)*s;
    // @TODO: update list
    s = NULL;
}

void *zn_int_list_map_entry_dup(const void *s)
{
    // @TODO: duplicate list
    return NULL;
}

int zn_int_list_map_entry_cmp(const void *left, const void *right)
{
    // @TODO: compare list
    return -1;
}

void zn_int_list_map_init(zn_int_list_map_t *ps)
{
    _z_int_void_map_entry_f f;
    f.free = zn_int_list_map_entry_free;
    f.dup = zn_int_list_map_entry_dup;
    f.cmp = zn_int_list_map_entry_cmp;

    _z_int_void_map_init(ps, _Z_DEFAULT_I_MAP_CAPACITY, f);
}

zn_int_list_map_t zn_int_list_map_make()
{
    zn_int_list_map_t ism;
    zn_int_list_map_init(&ism);
    return ism;
}

z_list_t *zn_int_list_map_insert(zn_int_list_map_t *ps, unsigned int key, z_list_t *value)
{
    return (z_list_t *)_z_int_void_map_insert(ps, key, (void *)value);
}

const z_list_t *zn_int_list_map_get(const zn_int_list_map_t *ps, unsigned int key)
{
    return (const z_list_t *)_z_int_void_map_get(ps, key);
}
