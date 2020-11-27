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

#ifndef ZENOH_PICO_TYPES_H
#define ZENOH_PICO_TYPES_H

#include <stdint.h>
#include <stdlib.h>

/**
 * A variable-length encoding unsigned integer. 
 */
typedef size_t z_zint_t;

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
 * A zenoh timestamp.
 */
typedef struct
{
    uint64_t time;
    z_bytes_t id;
} z_timestamp_t;

#endif /* ZENOH_PICO_TYPES_H */
