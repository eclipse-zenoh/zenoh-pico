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
#include "zenoh-pico/utils/properties.h"
#include "zenoh-pico/utils/collections.h"

zn_properties_t zn_properties_make()
{
    return z_i_map_make(_Z_DEFAULT_I_MAP_CAPACITY);
}

int zn_properties_init(zn_properties_t *ps)
{
    return z_i_map_init(ps, _Z_DEFAULT_I_MAP_CAPACITY);
}

int zn_properties_insert(zn_properties_t *ps, unsigned int key, z_string_t value)
{
    return z_i_map_set(ps, key, (z_str_t)value.val);
}

z_string_t zn_properties_get(const zn_properties_t *ps, unsigned int key)
{
    z_string_t s;
    z_str_t p = z_i_map_get(ps, key);
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

size_t zn_properties_len(const zn_properties_t *ps)
{
    return z_i_map_len(ps);
}

int zn_properties_is_empty(const zn_properties_t *ps)
{
    return z_i_map_is_empty(ps);
}

void zn_properties_clear(zn_properties_t *ps)
{
    z_i_map_clear(ps);
}

void zn_properties_free(zn_properties_t **ps)
{
    z_i_map_free(ps);
}

zn_properties_result_t zn_properties_from_str(const z_str_t s, int argc, zn_property_mapping_t argv[])
{
    zn_properties_result_t res;
    res.value.properties = zn_properties_make();

    res.tag = _z_res_t_OK;

    // Check the string contains only the right
    z_str_t start = s;
    z_str_t end = &s[strlen(s)];
    while (start != end)
    {
        z_str_t p_key_start = start;
        z_str_t p_key_end = strchr(s, '=');

        if (p_key_end == NULL)
            goto ERR;

        size_t key_len = p_key_end - p_key_start;
        int found = 0;
        for (unsigned int i = 0; i < argc; i++)
        {
            if (key_len != strlen(argv[i].str))
                continue;
            if (strncmp(p_key_start, argv[i].str, key_len) != 0)
                continue;

            found = 1;
            break;
        }

        if (!found)
            goto ERR;

        z_str_t p_value_end = strchr(p_key_end, ';');
        if (p_value_end == NULL)
            start = end;
        else
            start = p_value_end + 1;
    }

    // Parse the string keys into integer keys
    for (unsigned int i = 0; i < argc; i++)
    {
        z_str_t p_key_init = strpbrk(s, argv[i].str);
        if (p_key_init == NULL)
            continue;

        z_str_t p_equal = p_key_init + strlen(argv[i].str);
        if (p_equal[0] != '=')
            goto ERR;

        z_str_t p_value_init = p_equal + 1;
        z_str_t p_value_end = strchr(p_value_init, ';');
        if (p_value_end == NULL)
            p_value_end = &s[strlen(s)];

        int len = p_value_end - p_value_init;
        z_str_t p_value = (z_str_t)malloc((len + 1) * sizeof(char));
        strncpy(p_value, p_value_init, len);
        p_value[len] = '\0';

        z_string_t value;
        _z_string_move_str(&value, p_value);
        zn_properties_insert(&res.value.properties, argv[i].key, value);
    }

    return res;

ERR:
    zn_properties_clear(&res.value.properties);
    res.tag = _z_res_t_ERR;
    res.value.error = _z_err_t_PARSE_STRING;
    return res;
}
