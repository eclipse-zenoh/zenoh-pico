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

/*------------------ int-string map ------------------*/
zn_int_str_map_result_t zn_int_str_map_from_strn(const z_str_t s, unsigned int argc, zn_int_str_mapping_t argv[], size_t n)
{
    zn_int_str_map_result_t res;

    res.tag = _z_res_t_OK;
    res.value.int_str_map = zn_int_str_map_make();

    // Check the string contains only the right
    z_str_t start = s;
    z_str_t end = &s[n];
    while (start < end)
    {
        z_str_t p_key_start = start;
        z_str_t p_key_end = strchr(p_key_start, INT_STR_MAP_KEYVALUE_SEPARATOR);

        if (p_key_end == NULL)
            goto ERR;

        // Verify the key is valid based on the provided mapping
        size_t p_key_len = p_key_end - p_key_start;
        int found = 0;
        unsigned int key;
        for (unsigned int i = 0; i < argc; i++)
        {
            if (p_key_len != strlen(argv[i].str))
                continue;
            if (strncmp(p_key_start, argv[i].str, p_key_len) != 0)
                continue;

            found = 1;
            key = argv[i].key;
            break;
        }

        if (!found)
            goto ERR;

        // Read and populate the value
        z_str_t p_value_start = p_key_end + 1;
        z_str_t p_value_end = strchr(p_key_end, INT_STR_MAP_LIST_SEPARATOR);
        if (p_value_end == NULL)
            p_value_end = end;

        size_t p_value_len = p_value_end - p_value_start;
        z_str_t p_value = (z_str_t)malloc((p_value_len + 1) * sizeof(char));
        strncpy(p_value, p_value_start, p_value_len);
        p_value[p_value_len] = '\0';

        printf("\nInserting: %s\n", p_value);

        zn_int_str_map_insert(&res.value.int_str_map, key, p_value);

        // Process next key value
        start = p_value_end + 1;
    }

    return res;

ERR:
    zn_int_str_map_clear(&res.value.int_str_map);
    res.tag = _z_res_t_ERR;
    res.value.error = _z_err_t_PARSE_STRING;
    return res;
}

zn_int_str_map_result_t zn_int_str_map_from_str(const z_str_t s, unsigned int argc, zn_int_str_mapping_t argv[])
{
    return zn_int_str_map_from_strn(s, argc, argv, strlen(s));
}

size_t zn_int_str_map_strlen(const zn_int_str_map_t *s, unsigned int argc, zn_int_str_mapping_t argv[])
{
    // Calculate the string length to allocate
    size_t len = 0;
    for (size_t i = 0; i < argc; i++)
    {
        z_str_t v = zn_int_str_map_get(s, argv[i].key);
        if (v != NULL)
        {
            if (len != 0)
                len += 1;               // List separator
            len += strlen(argv[i].str); // Key
            len += 1;                   // KeyValue separator
            len += strlen(v);           // Value
        }
    }

    return len;
}

void zn_int_str_map_onto_str(z_str_t dst, const zn_int_str_map_t *s, unsigned int argc, zn_int_str_mapping_t argv[])
{
    // Build the string
    dst[0] = '\0';

    const char lsep = INT_STR_MAP_LIST_SEPARATOR;
    const char ksep = INT_STR_MAP_KEYVALUE_SEPARATOR;
    for (size_t i = 0; i < argc; i++)
    {
        z_str_t v = zn_int_str_map_get(s, argv[i].key);
        if (v != NULL)
        {
            if (strlen(dst) != 0)
                strncat(dst, &lsep, 1); // List separator
            strcat(dst, argv[i].str);   // Key
            strncat(dst, &ksep, 1);     // KeyValue separator
            strcat(dst, v);             // Value
        }
    }
}

z_str_t zn_int_str_map_to_str(const zn_int_str_map_t *s, unsigned int argc, zn_int_str_mapping_t argv[])
{
    // Calculate the string length to allocate
    size_t len = zn_int_str_map_strlen(s, argc, argv);
    // Build the string
    z_str_t dst = (z_str_t)malloc(len + 1);
    zn_int_str_map_onto_str(dst, s, argc, argv);
    return dst;
}
