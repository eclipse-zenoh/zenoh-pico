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

#include "arc_slice.h"
#include "vec.h"
#include "zenoh-pico/protocol/iobuf.h"

inline size_t _z_arc_slice_size(const _z_arc_slice_t *s) {
    (void)s;
    return sizeof(_z_arc_slice_t);
}
static inline void _z_arc_slice_elem_move(void *dst, void *src) {
    _z_arc_slice_move((_z_arc_slice_t *)dst, (_z_arc_slice_t *)src);
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
} _z_bytes_t;

_Bool _z_bytes_check(const _z_bytes_t *bytes);
_z_bytes_t _z_bytes_null(void);
int8_t _z_bytes_append_bytes(_z_bytes_t *dst, _z_bytes_t *src);
int8_t _z_bytes_append_slice(_z_bytes_t *dst, _z_arc_slice_t *s);
int8_t _z_bytes_copy(_z_bytes_t *dst, const _z_bytes_t *src);
_z_bytes_t _z_bytes_duplicate(const _z_bytes_t *src);
void _z_bytes_move(_z_bytes_t *dst, _z_bytes_t *src);
void _z_bytes_drop(_z_bytes_t *bytes);
void _z_bytes_free(_z_bytes_t **bs);
size_t _z_bytes_num_slices(const _z_bytes_t *bs);
_z_arc_slice_t *_z_bytes_get_slice(const _z_bytes_t *bs, size_t i);
size_t _z_bytes_len(const _z_bytes_t *bs);
_Bool _z_bytes_is_empty(const _z_bytes_t *bs);
int8_t _z_bytes_to_uint8(const _z_bytes_t *bs, uint8_t *u);
int8_t _z_bytes_to_uint16(const _z_bytes_t *bs, uint16_t *u);
int8_t _z_bytes_to_uint32(const _z_bytes_t *bs, uint32_t *u);
int8_t _z_bytes_to_uint64(const _z_bytes_t *bs, uint64_t *u);
int8_t _z_bytes_to_float(const _z_bytes_t *bs, float *f);
int8_t _z_bytes_to_double(const _z_bytes_t *bs, double *d);
int8_t _z_bytes_to_slice(const _z_bytes_t *bytes, _z_slice_t *s);
int8_t _z_bytes_from_slice(_z_bytes_t *b, _z_slice_t s);
int8_t _z_bytes_from_uint8(_z_bytes_t *b, uint8_t val);
int8_t _z_bytes_from_uint16(_z_bytes_t *b, uint16_t val);
int8_t _z_bytes_from_uint32(_z_bytes_t *b, uint32_t val);
int8_t _z_bytes_from_uint64(_z_bytes_t *b, uint64_t val);
int8_t _z_bytes_from_float(_z_bytes_t *b, float val);
int8_t _z_bytes_from_double(_z_bytes_t *b, double val);
size_t _z_bytes_to_buf(const _z_bytes_t *bytes, uint8_t *dst, size_t len);
int8_t _z_bytes_from_buf(_z_bytes_t *b, const uint8_t *src, size_t len);
int8_t _z_bytes_serialize_from_pair(_z_bytes_t *out, _z_bytes_t *first, _z_bytes_t *second);
int8_t _z_bytes_deserialize_into_pair(const _z_bytes_t *bs, _z_bytes_t *first_out, _z_bytes_t *second_out);
_z_slice_t _z_bytes_try_get_contiguous(const _z_bytes_t *bs);

typedef struct {
    size_t slice_idx;
    size_t in_slice_idx;
    size_t byte_idx;
    const _z_bytes_t *bytes;
} _z_bytes_reader_t;

_z_bytes_reader_t _z_bytes_get_reader(const _z_bytes_t *bytes);
int8_t _z_bytes_reader_seek(_z_bytes_reader_t *reader, int64_t offset, int origin);
int64_t _z_bytes_reader_tell(const _z_bytes_reader_t *reader);
int8_t _z_bytes_reader_read_slices(_z_bytes_reader_t *reader, size_t len, _z_bytes_t *out);
size_t _z_bytes_reader_read(_z_bytes_reader_t *reader, uint8_t *buf, size_t len);

typedef struct {
    _z_bytes_reader_t _reader;
} _z_bytes_iterator_t;

_z_bytes_iterator_t _z_bytes_get_iterator(const _z_bytes_t *bytes);
int8_t _z_bytes_iterator_next(_z_bytes_iterator_t *iter, _z_bytes_t *b);

typedef struct {
    uint8_t *cache;
    size_t cache_size;
    _z_bytes_t *bytes;
} _z_bytes_writer_t;

_z_bytes_writer_t _z_bytes_get_writer(_z_bytes_t *bytes, size_t cache_size);
int8_t _z_bytes_writer_write(_z_bytes_writer_t *writer, const uint8_t *src, size_t len);
int8_t _z_bytes_writer_ensure_cache(_z_bytes_writer_t *writer);

typedef struct {
    _z_bytes_writer_t writer;
} _z_bytes_iterator_writer_t;

_z_bytes_iterator_writer_t _z_bytes_get_iterator_writer(_z_bytes_t *bytes);
int8_t _z_bytes_iterator_writer_write(_z_bytes_iterator_writer_t *writer, _z_bytes_t *bytes);
#endif /* ZENOH_PICO_COLLECTIONS_BYTES_H */
