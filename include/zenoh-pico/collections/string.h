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

#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/collections/vec.h"

/*-------- str --------*/
/**
 * A string with null terminator.
 */
typedef char *_z_str_t;

_z_str_t _z_str_clone(const _z_str_t src);
void _z_str_clear(_z_str_t src);
void _z_str_free(_z_str_t *src);
int _z_str_eq(const _z_str_t left, const _z_str_t right);

size_t _z_str_size(const _z_str_t src);
void _z_str_copy(_z_str_t dst, const _z_str_t src);
_Z_ELEM_DEFINE(_z_str, char, _z_str_size, _z_noop_clear, _z_str_copy)
_Z_VEC_DEFINE(_z_str, char)
_Z_LIST_DEFINE(_z_str, char)
_Z_INT_MAP_DEFINE(_z_str, char)

#define INT_STR_MAP_KEYVALUE_SEPARATOR '='
#define INT_STR_MAP_LIST_SEPARATOR ';'

typedef struct
{
    unsigned int key;
    _z_str_t str;
} _z_str_intmapping_t;
_Z_RESULT_DECLARE(_z_str_intmap_t, str_intmap)

size_t _z_str_intmap_strlen(const _z_str_intmap_t *s, unsigned int argc, _z_str_intmapping_t argv[]);

void _z_str_intmap_onto_str(_z_str_t dst, const _z_str_intmap_t *s, unsigned int argc, _z_str_intmapping_t argv[]);
_z_str_t _z_str_intmap_to_str(const _z_str_intmap_t *s, unsigned int argc, _z_str_intmapping_t argv[]);

_z_str_intmap_result_t _z_str_intmap_from_str(const _z_str_t s, unsigned int argc, _z_str_intmapping_t argv[]);
_z_str_intmap_result_t _z_str_intmap_from_strn(const _z_str_t s, unsigned int argc, _z_str_intmapping_t argv[], size_t n);

/*-------- string --------*/
/**
 * A string with no terminator.
 *
 * Members:
 *   size_t len: The length of the string.
 *   const _z_str_t val: A pointer to the string.
 */
typedef struct
{
    _z_str_t val;
    size_t len;
} _z_string_t;

_z_string_t z_string_make(const _z_str_t value);
void _z_string_append(_z_string_t *dst, const _z_string_t *src);
void _z_string_copy(_z_string_t *dst, const _z_string_t *src);
void _z_string_move(_z_string_t *dst, _z_string_t *src);
void _z_string_move_str(_z_string_t *dst, _z_str_t src);
void _z_string_clear(_z_string_t *s);
void _z_string_free(_z_string_t **s);
void _z_string_reset(_z_string_t *s);
_z_string_t _z_string_from_bytes(_z_bytes_t *bs);

/*-------- str_array --------*/
/**
 * An array of NULL terminated strings.
 *
 * Members:
 *   size_t len: The length of the array.
 *   _z_str_t *val: A pointer to the array.
 */
typedef struct
{
    _z_str_t *val;
    size_t len;
} _z_str_array_t;

_z_str_array_t _z_str_array_make(size_t len);
void _z_str_array_init(_z_str_array_t *sa, size_t len);
void _z_str_array_copy(_z_str_array_t *dst, const _z_str_array_t *src);
void _z_str_array_move(_z_str_array_t *dst, _z_str_array_t *src);
void _z_str_array_free(_z_str_array_t *sa);

#endif /* ZENOH_PICO_COLLECTIONS_STRING_H */
