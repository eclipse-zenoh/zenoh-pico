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
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <string.h>
#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/utils/properties.h"

int zn_properties_init(zn_properties_t *ps)
{
    zn_int_str_map_init(ps);
    return 0;
}

zn_properties_t zn_properties_make()
{
    zn_properties_t ps;
    zn_properties_init(&ps);
    return ps;
}

int zn_properties_insert(zn_properties_t *ps, unsigned int key, z_string_t value)
{
    z_str_t res = zn_int_str_map_insert(ps, key, (z_str_t)value.val);
    return res == value.val ? 0 : -1;
}

z_string_t zn_properties_get(const zn_properties_t *ps, unsigned int key)
{
    z_string_t s;
    z_str_t p = zn_int_str_map_get(ps, key);
    if (p == NULL)
    {
        s.val = NULL;
        s.len = 0;
    }
    else
    {
        s.val = p;
        s.len = strlen(p);
    }
    return s;
}
