//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>

#ifndef ZENOH_PICO_COLLECTIONS_STRING_H
#define ZENOH_PICO_COLLECTIONS_STRING_H

#include "zenoh-pico/collections/array.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/collections/vec.h"

/*-------- str --------*/
typedef char *_z_str_t;

char *_z_str_clone(const char *src);
void _z_str_clear(char *src);
void _z_str_free(char **src);
_Bool _z_str_eq(const char *left, const char *right);

size_t _z_str_size(const char *src);
void _z_str_copy(char *dst, const char *src);
void _z_str_n_copy(char *dst, const char *src, size_t size);
_Z_ELEM_DEFINE(_z_str, char, _z_str_size, _z_noop_clear, _z_str_copy)
// _Z_ARRAY_DEFINE(_z_str, char *)
// This is here for reference on why
// the _z_str_array_t was not defined using this macro
// but instead manually as find below
_Z_VEC_DEFINE(_z_str, char)
_Z_LIST_DEFINE(_z_str, char)
_Z_INT_MAP_DEFINE(_z_str, char)

#define INT_STR_MAP_KEYVALUE_SEPARATOR '='
#define INT_STR_MAP_LIST_SEPARATOR ';'

typedef struct {
    char *_str;
    uint8_t _key;
} _z_str_intmapping_t;

size_t _z_str_intmap_strlen(const _z_str_intmap_t *s, uint8_t argc, _z_str_intmapping_t argv[]);

void _z_str_intmap_onto_str(char *dst, size_t dst_len, const _z_str_intmap_t *s, uint8_t argc,
                            _z_str_intmapping_t argv[]);
char *_z_str_intmap_to_str(const _z_str_intmap_t *s, uint8_t argc, _z_str_intmapping_t argv[]);

int8_t _z_str_intmap_from_str(_z_str_intmap_t *strint, const char *s, uint8_t argc, _z_str_intmapping_t argv[]);
int8_t _z_str_intmap_from_strn(_z_str_intmap_t *strint, const char *s, uint8_t argc, _z_str_intmapping_t argv[],
                               size_t n);

/*-------- string --------*/
/**
 * A string with no terminator.
 *
 * Members:
 *   size_t len: The length of the string.
 *   const char *val: A pointer to the string.
 */
typedef struct {
    size_t len;
    char *val;
} _z_string_t;

_z_string_t _z_string_make(const char *value);
void _z_string_copy(_z_string_t *dst, const _z_string_t *src);
void _z_string_move(_z_string_t *dst, _z_string_t *src);
void _z_string_move_str(_z_string_t *dst, char *src);
void _z_string_clear(_z_string_t *s);
void _z_string_free(_z_string_t **s);
void _z_string_reset(_z_string_t *s);
_z_string_t _z_string_from_bytes(const _z_bytes_t *bs);

/*-------- str_array --------*/
/**
 * An array of NULL terminated strings.
 *
 * Members:
 *   size_t len: The length of the array.
 *   char **_val: A pointer to the array.
 */
typedef struct {
    size_t len;
    char **val;
} _z_str_array_t;

_z_str_array_t _z_str_array_empty(void);
_z_str_array_t _z_str_array_make(size_t len);
void _z_str_array_init(_z_str_array_t *sa, size_t len);
char **_z_str_array_get(const _z_str_array_t *sa, size_t pos);
size_t _z_str_array_len(const _z_str_array_t *sa);
_Bool _z_str_array_is_empty(const _z_str_array_t *sa);
void _z_str_array_copy(_z_str_array_t *dst, const _z_str_array_t *src);
void _z_str_array_move(_z_str_array_t *dst, _z_str_array_t *src);
void _z_str_array_clear(_z_str_array_t *sa);
void _z_str_array_free(_z_str_array_t **sa);

#endif /* ZENOH_PICO_COLLECTIONS_STRING_H */
