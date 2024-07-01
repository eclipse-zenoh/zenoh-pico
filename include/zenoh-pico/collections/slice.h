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
//

#ifndef ZENOH_PICO_COLLECTIONS_SLICE_H
#define ZENOH_PICO_COLLECTIONS_SLICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*-------- Slice --------*/
/**
 * An array of bytes.
 *
 * Members:
 *   size_t len: The length of the bytes array.
 *   uint8_t *start: A pointer to the bytes array.
 *   _Bool _is_alloc: Indicates if memory has been allocated by this module
 */
typedef struct {
    size_t len;
    const uint8_t *start;
    _Bool _is_alloc;
} _z_slice_t;

_z_slice_t _z_slice_empty(void);
inline static _Bool _z_slice_check(const _z_slice_t *slice) { return slice->start != NULL; }
int8_t _z_slice_init(_z_slice_t *bs, size_t capacity);
_z_slice_t _z_slice_make(size_t capacity);
_z_slice_t _z_slice_wrap(const uint8_t *bs, size_t len);
_z_slice_t _z_slice_wrap_copy(const uint8_t *bs, size_t len);
_z_slice_t _z_slice_steal(_z_slice_t *b);
int8_t _z_slice_copy(_z_slice_t *dst, const _z_slice_t *src);
_z_slice_t _z_slice_duplicate(const _z_slice_t *src);
void _z_slice_move(_z_slice_t *dst, _z_slice_t *src);
void _z_slice_reset(_z_slice_t *bs);
_Bool _z_slice_is_empty(const _z_slice_t *bs);
_Bool _z_slice_eq(const _z_slice_t *left, const _z_slice_t *right);
void _z_slice_clear(_z_slice_t *bs);
void _z_slice_free(_z_slice_t **bs);

#endif /* ZENOH_PICO_COLLECTIONS_SLICE_H */
