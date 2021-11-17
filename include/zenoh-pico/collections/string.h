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

#ifndef ZENOH_PICO_UTILS_COLLECTION_STRING_H
#define ZENOH_PICO_UTILS_COLLECTION_STRING_H

#include <stdint.h>
#include <stdio.h>
#include "zenoh-pico/collections/bytes.h"

/*-------- str --------*/
/**
 * A string with null terminator.
 */
typedef char *z_str_t;

z_str_t _z_str_dup(const z_str_t src);

/*-------- string --------*/
/**
 * A string with no terminator.
 *
 * Members:
 *   size_t len: The length of the string.
 *   const z_str_t val: A pointer to the string.
 */
typedef struct
{
    z_str_t val;
    size_t len;
} z_string_t;

z_string_t z_string_make(const z_str_t value);
void z_string_free(z_string_t *s);
void _z_string_copy(z_string_t *dst, const z_string_t *src);
void _z_string_move(z_string_t *dst, z_string_t *src);
void _z_string_move_str(z_string_t *dst, z_str_t src);
void _z_string_free(z_string_t *str);
void _z_string_reset(z_string_t *str);
z_string_t _z_string_from_bytes(z_bytes_t *bs);

/*-------- str_array --------*/
/**
 * An array of NULL terminated strings.
 *
 * Members:
 *   size_t len: The length of the array.
 *   z_str_t *val: A pointer to the array.
 */
typedef struct
{
    z_str_t *val;
    size_t len;
} z_str_array_t;

z_str_array_t _z_str_array_make(size_t len);
void _z_str_array_init(z_str_array_t *sa, size_t len);
void _z_str_array_copy(z_str_array_t *dst, const z_str_array_t *src);
void _z_str_array_move(z_str_array_t *dst, z_str_array_t *src);
void _z_str_array_free(z_str_array_t *sa);

#endif /* ZENOH_PICO_UTILS_COLLECTION_STRING_H */
