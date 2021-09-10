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
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <string.h>
#include "zenoh-pico/net/property.h"
#include "zenoh-pico/private/collection.h"

zn_properties_t *zn_properties_make()
{
    return _z_i_map_make(_Z_DEFAULT_I_MAP_CAPACITY);
}

zn_properties_t *zn_properties_insert(zn_properties_t *ps, unsigned int key, z_string_t value)
{
    _z_i_map_set(ps, key, (z_str_t)value.val);
    return ps;
}

z_string_t zn_properties_get(zn_properties_t *ps, unsigned int key)
{
    z_string_t s;
    z_str_t p = _z_i_map_get(ps, key);
    if (p)
    {
        s.val = p;
        s.len = strlen(p);
    }
    else
    {
        s.val = NULL;
        s.len = 0;
    }
    return s;
}

unsigned int zn_properties_len(zn_properties_t *ps)
{
    return _z_i_map_len(ps);
}

void zn_properties_free(zn_properties_t *ps)
{
    _z_i_map_free(ps);
}
