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
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/collections/vec.h"

/*-------- str --------*/
typedef char *_z_str_t;

char *_z_str_clone(const char *src);
char *_z_str_n_clone(const char *src, size_t len);
void _z_str_clear(char *src);
void _z_str_free(char **src);
_Bool _z_str_eq(const char *left, const char *right);

size_t _z_str_size(const char *src);
void _z_str_copy(char *dst, const char *src);
void _z_str_n_copy(char *dst, const char *src, size_t size);
_Z_ELEM_DEFINE(_z_str, char, _z_str_size, _z_noop_clear, _z_str_copy)

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
 */
typedef struct {
    _z_slice_t _slice;
} _z_string_t;

_z_string_t _z_string_null(void);
_Bool _z_string_check(const _z_string_t *value);
_z_string_t _z_string_copy_from_str(const char *value);
_z_string_t _z_string_copy_from_substr(const char *value, size_t len);
_z_string_t *_z_string_copy_from_str_as_ptr(const char *value);
_z_string_t _z_string_alias_str(const char *value);
_z_string_t _z_string_alias_substr(const char *value, size_t len);
_z_string_t _z_string_from_str_custom_deleter(char *value, _z_delete_context_t c);
_Bool _z_string_is_empty(const _z_string_t *s);
const char *_z_string_rchr(_z_string_t *str, char filter);
char *_z_string_pbrk(_z_string_t *str, const char *filter);

size_t _z_string_len(const _z_string_t *s);
const char *_z_string_data(const _z_string_t *s);
int8_t _z_string_copy(_z_string_t *dst, const _z_string_t *src);
int8_t _z_string_copy_substring(_z_string_t *dst, const _z_string_t *src, size_t offset, size_t len);
void _z_string_move(_z_string_t *dst, _z_string_t *src);
_z_string_t _z_string_steal(_z_string_t *str);
_z_string_t _z_string_alias(const _z_string_t *str);
void _z_string_move_str(_z_string_t *dst, char *src);
void _z_string_clear(_z_string_t *s);
void _z_string_free(_z_string_t **s);
void _z_string_reset(_z_string_t *s);
_Bool _z_string_equals(const _z_string_t *left, const _z_string_t *right);
_z_string_t _z_string_convert_bytes(const _z_slice_t *bs);
_z_string_t _z_string_preallocate(const size_t len);

_Z_ELEM_DEFINE(_z_string, _z_string_t, _z_string_len, _z_string_clear, _z_string_copy)

static inline void _z_string_elem_move(void *dst, void *src) { _z_string_move((_z_string_t *)dst, (_z_string_t *)src); }
_Z_SVEC_DEFINE(_z_string, _z_string_t)
_Z_LIST_DEFINE(_z_string, _z_string_t)
_Z_INT_MAP_DEFINE(_z_string, _z_string_t)

#endif /* ZENOH_PICO_COLLECTIONS_STRING_H */
