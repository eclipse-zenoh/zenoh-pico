//
// Copyright (c) 2024 ZettaScale Technology
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

#ifndef ZENOH_PICO_COLLECTIONS_BYTES_H
#define ZENOH_PICO_COLLECTIONS_BYTES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "vec.h"
#include "arc_slice.h"
#include "zenoh-pico/protocol/iobuf.h"



inline size_t _z_arc_slice_size(const _z_arc_slice_t *s) {
    (void)s;
    return sizeof(_z_arc_slice_size); 
}
static inline void _z_arc_slice_elem_move(void *dst, void *src) {
    _z_arc_slice_move((_z_arc_slice_t *)dst, (_z_arc_slice_t*)src);
}
_Z_ELEM_DEFINE(_z_arc_slice, _z_arc_slice_t, _z_arc_slice_size, _z_arc_slice_drop, _z_arc_slice_copy)
_Z_SVEC_DEFINE(_z_arc_slice, _z_arc_slice_t)


/*-------- Bytes --------*/
/**
 * A container for slices.
 *
 * Members:
 *   _z_slice_vec_t _slices: contents of the container.
 */

typedef struct {
    _z_arc_slice_svec_t _slices;
} _zz_bytes_t;


_Bool _zz_bytes_check(const _zz_bytes_t *bytes);
_zz_bytes_t _zz_bytes_null(void);
int8_t _zz_bytes_append(_zz_bytes_t* dst, _zz_bytes_t* src);
int8_t _zz_bytes_append_slice(_zz_bytes_t* dst, _z_arc_slice_t* s);
int8_t _zz_bytes_copy(_zz_bytes_t *dst, const _zz_bytes_t *src);
_zz_bytes_t _zz_bytes_duplicate(const _zz_bytes_t *src);
void _zz_bytes_move(_zz_bytes_t *dst, _zz_bytes_t *src);
void _zz_bytes_drop(_zz_bytes_t *bytes);
void _zz_bytes_free(_zz_bytes_t **bs);
size_t _zz_bytes_num_slices(const _zz_bytes_t *bs);
_z_arc_slice_t* _zz_bytes_get_slice(const _zz_bytes_t *bs, size_t i);
size_t _zz_bytes_len(const _zz_bytes_t *bs);
_Bool _zz_bytes_is_empty(const _zz_bytes_t *bs);
uint8_t _zz_bytes_to_uint8(const _zz_bytes_t *bs);
uint16_t _zz_bytes_to_uint16(const _zz_bytes_t *bs);
uint32_t _zz_bytes_to_uint32(const _zz_bytes_t *bs);
uint64_t _zz_bytes_to_uint64(const _zz_bytes_t *bs);
float _zz_bytes_to_float(const _zz_bytes_t *bs);
double _zz_bytes_to_double(const _zz_bytes_t *bs);
_z_slice_t _zz_bytes_to_slice(const _zz_bytes_t *bytes);
_zz_bytes_t _zz_bytes_from_slice(_z_slice_t s);
_zz_bytes_t _zz_bytes_from_uint8(uint8_t val);
_zz_bytes_t _zz_bytes_from_uint16(uint16_t val);
_zz_bytes_t _zz_bytes_from_uint32(uint32_t val);
_zz_bytes_t _zz_bytes_from_uint64(uint64_t val);
_zz_bytes_t _zz_bytes_from_float(float val);
_zz_bytes_t _zz_bytes_from_double(double val);
size_t _zz_bytes_to_buf(const _zz_bytes_t *bytes, uint8_t* dst, size_t len);
_zz_bytes_t _zz_bytes_from_buf(uint8_t* src, size_t len);


typedef struct {
    size_t slice_idx;
    size_t in_slice_idx;
    size_t byte_idx;
    const _zz_bytes_t* bytes;
} _zz_bytes_reader_t;



_zz_bytes_reader_t _zz_bytes_get_reader(const _zz_bytes_t *bytes);
int8_t _zz_bytes_reader_seek(_zz_bytes_reader_t *reader, int64_t offset, int origin);
int64_t _zz_bytes_reader_tell(const _zz_bytes_reader_t *reader);
int8_t _zz_bytes_reader_read_slices(_zz_bytes_reader_t* reader, size_t len, _zz_bytes_t* out);
int8_t _zz_bytes_reader_read(_zz_bytes_reader_t *reader, uint8_t* buf, size_t len);
int8_t _zz_bytes_reader_read_next(_zz_bytes_reader_t* reader, _zz_bytes_t* out);

#endif /* ZENOH_PICO_COLLECTIONS_BYTES_H */