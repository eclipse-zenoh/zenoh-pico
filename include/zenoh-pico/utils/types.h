
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

#ifndef _ZENOH_PICO_UTILS_TYPES_H
#define _ZENOH_PICO_UTILS_TYPES_H

#include <stdint.h>
#include <string.h>

/*------------------ Collection ------------------*/
typedef struct
{
    size_t _capacity;
    size_t _len;
    void **_val;
} _z_vec_t;

typedef int (*_z_list_predicate)(void *, void *);
typedef struct z_list
{
    void *val;
    struct z_list *tail;
} _z_list_t;

typedef struct
{
    size_t key;
    void *value;
} _z_i_map_entry_t;

typedef struct
{
    _z_list_t **vals;
    size_t capacity;
    size_t len;
} _z_i_map_t;

/*------------------ Zenoh ------------------*/
/**
 * A string with null terminator.
 */
typedef char *z_str_t;

/**
 * A string with no terminator.
 *
 * Members:
 *   size_t len: The length of the string.
 *   const char *val: A pointer to the string.
 */
typedef struct z_string_t
{
    const char *val;
    size_t len;
} z_string_t;

/**
 * An array of bytes.
 *
 * Members:
 *   size_t len: The length of the bytes array.
 *   const uint8_t *val: A pointer to the bytes array.
 */
typedef struct z_bytes_t
{
    const uint8_t *val;
    size_t len;
} z_bytes_t;

/**
 * An array of NULL terminated strings.
 *
 * Members:
 *   size_t len: The length of the array.
 *   char *const *val: A pointer to the array.
 */
typedef struct
{
    const char *const *val;
    size_t len;
} z_str_array_t;

/**
 * A zenoh-net property.
 *
 * Members:
 *   unsiggned int id: The property ID.
 *   z_string_t value: The property value as a string.
 */
typedef struct
{
    unsigned int id;
    z_string_t value;
} zn_property_t;

/**
 * Zenoh-net properties are represented as int-string map.
 */
typedef _z_i_map_t zn_properties_t;

#endif /* _ZENOH_PICO_UTILS_TYPES_H */
