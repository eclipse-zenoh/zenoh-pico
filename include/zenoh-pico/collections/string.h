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

#ifndef ZENOH_PICO_COLLECTIONS_STRING_H
#define ZENOH_PICO_COLLECTIONS_STRING_H

#include <stdint.h>
#include <stdio.h>
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/collections/vec.h"

/*-------- str --------*/
/**
 * A string with null terminator.
 */
typedef char *z_str_t;

void _z_str_clear(z_str_t src);
z_str_t _z_str_clone(const z_str_t src);
int _z_str_cmp(const z_str_t left, const z_str_t right);

_Z_ELEM_DEFINE(_z_str, char, _z_str_clear, _z_str_clone, _z_str_cmp)
_Z_VEC_DEFINE(_z_str, char)
_Z_LIST_DEFINE(_z_str, char)
_Z_INT_MAP_DEFINE(_z_str, char)

#define INT_STR_MAP_KEYVALUE_SEPARATOR '='
#define INT_STR_MAP_LIST_SEPARATOR ';'

typedef struct
{
    unsigned int key;
    z_str_t str;
} _z_str_intmapping_t;
_Z_RESULT_DECLARE(_z_str_intmap_t, str_intmap)

size_t _z_str_intmap_strlen(const _z_str_intmap_t *s, unsigned int argc, _z_str_intmapping_t argv[]);

void _z_str_intmap_onto_str(z_str_t dst, const _z_str_intmap_t *s, unsigned int argc, _z_str_intmapping_t argv[]);
z_str_t _z_str_intmap_to_str(const _z_str_intmap_t *s, unsigned int argc, _z_str_intmapping_t argv[]);

_z_str_intmap_result_t _z_str_intmap_from_str(const z_str_t s, unsigned int argc, _z_str_intmapping_t argv[]);
_z_str_intmap_result_t _z_str_intmap_from_strn(const z_str_t s, unsigned int argc, _z_str_intmapping_t argv[], size_t n);

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
void _z_string_copy(z_string_t *dst, const z_string_t *src);
void _z_string_move(z_string_t *dst, z_string_t *src);
void _z_string_move_str(z_string_t *dst, z_str_t src);
void _z_string_clear(z_string_t *s);
void _z_string_free(z_string_t **s);
void _z_string_reset(z_string_t *s);
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

#endif /* ZENOH_PICO_COLLECTIONS_STRING_H */
