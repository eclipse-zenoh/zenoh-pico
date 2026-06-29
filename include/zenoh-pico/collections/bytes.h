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

#include "buf.h"
#include "slice.h"
#include "zenoh-pico/system/common/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

// _z_bytes_t stores its payload as either a single owning slice (the common case,
// avoiding any heap allocation for the slice vector) or, when it holds multiple
// slices, as a dynamically allocated vector of plain (owning) slices.
#define _ZP_VECTOR_TEMPLATE_ELEM_TYPE _z_slice_t
#define _ZP_VECTOR_TEMPLATE_NAME _z_slice_vec
#define _ZP_VECTOR_TEMPLATE_ELEM_DESTROY_FN(x) _z_slice_clear(x)
#define _ZP_VECTOR_TEMPLATE_ELEM_MOVE_FN(dst, src) (*(dst) = *(src), *(src) = _z_slice_null())
#define _ZP_VECTOR_TEMPLATE_ALLOC_FN z_malloc
#define _ZP_VECTOR_TEMPLATE_REALLOC_FN z_realloc
#define _ZP_VECTOR_TEMPLATE_FREE_FN z_free
#include "zenoh-pico/collections/vector_template.h"

// Variant holding either a single slice (alternative `slice`) or a vector of
// slices (alternative `vec`). The NONE state represents empty bytes.
#define _ZP_VARIANT_TEMPLATE_NAME _z_slice_single_or_vec
#define _ZP_VARIANT_TEMPLATE_1_TYPE _z_slice_t
#define _ZP_VARIANT_TEMPLATE_1_NAME slice
#define _ZP_VARIANT_TEMPLATE_1_DESTROY_FN(ptr) _z_slice_clear(ptr)
#define _ZP_VARIANT_TEMPLATE_1_MOVE_FN(dst, src) (*(dst) = *(src), *(src) = _z_slice_null())
#define _ZP_VARIANT_TEMPLATE_2_TYPE _z_slice_vec_t
#define _ZP_VARIANT_TEMPLATE_2_NAME vec
#define _ZP_VARIANT_TEMPLATE_2_DESTROY_FN(ptr) _z_slice_vec_destroy(ptr)
#define _ZP_VARIANT_TEMPLATE_2_MOVE_FN(dst, src) (*(dst) = *(src), _z_slice_vec_init(src))
#include "zenoh-pico/collections/variant_template.h"

/*-------- Bytes --------*/
/**
 * A container for slices, optimized for the common single-slice case.
 *
 * Members:
 *   _z_slice_single_or_vec_t _inner: either a single slice or a vector of slices.
 */

typedef struct {
    _z_slice_single_or_vec_t _inner;
} _z_bytes_t;

static inline _z_bytes_t _z_bytes_null(void) {
    _z_bytes_t b;
    b._inner = _z_slice_single_or_vec_none();
    return b;
}

static inline _z_bytes_t _z_bytes_steal(_z_bytes_t *src) {
    _z_bytes_t b = *src;
    src->_inner = _z_slice_single_or_vec_none();
    return b;
}
static inline size_t _z_bytes_num_slices(const _z_bytes_t *bs) {
    _ZP_VARIANT_CONST_VISIT(_z_slice_single_or_vec, &bs->_inner,
        (slice, return 1),
        (vec, return _z_slice_vec_size(_)),
        (none, return 0)
    );
    return 0;  // Unreachable, but avoids compiler warning.
}

static inline const _z_slice_t *_z_bytes_get_slice(const _z_bytes_t *bs, size_t i) {
    _ZP_VARIANT_CONST_VISIT(_z_slice_single_or_vec, &bs->_inner,
        (slice, return (i == 0) ? _ : NULL),
        (vec, return _z_slice_vec_const_get(_, i)),
        (none, return NULL)
    );
    return NULL;  // Unreachable, but avoids compiler warning.
}
static inline void _z_bytes_clear(_z_bytes_t *bytes) { _z_slice_single_or_vec_destroy(&bytes->_inner); }

bool _z_bytes_check(const _z_bytes_t *bytes);
z_result_t _z_bytes_append_bytes(_z_bytes_t *dst, _z_bytes_t *src);
z_result_t _z_bytes_append_slice(_z_bytes_t *dst, _z_slice_t *s);
z_result_t _z_bytes_copy(_z_bytes_t *dst, const _z_bytes_t *src);
z_result_t _z_bytes_move(_z_bytes_t *dst, _z_bytes_t *src);
void _z_bytes_free(_z_bytes_t **bs);
size_t _z_bytes_len(const _z_bytes_t *bs);
bool _z_bytes_is_empty(const _z_bytes_t *bs);
z_result_t _z_bytes_to_slice(const _z_bytes_t *bytes, _z_slice_t *s);
z_result_t _z_bytes_from_slice(_z_bytes_t *b, _z_slice_t *s);
size_t _z_bytes_to_buf(const _z_bytes_t *bytes, uint8_t *dst, size_t len);
z_result_t _z_bytes_copy_from_buf(_z_bytes_t *b, const uint8_t *src, size_t len);

typedef struct {
    size_t slice_idx;
    size_t in_slice_idx;
    size_t byte_idx;
    const _z_bytes_t *bytes;
} _z_bytes_reader_t;

_z_bytes_reader_t _z_bytes_get_reader(const _z_bytes_t *bytes);
z_result_t _z_bytes_reader_seek(_z_bytes_reader_t *reader, int64_t offset, int origin);
int64_t _z_bytes_reader_tell(const _z_bytes_reader_t *reader);
z_result_t _z_bytes_reader_read_slices(_z_bytes_reader_t *reader, size_t len, _z_bytes_t *out);
size_t _z_bytes_reader_read(_z_bytes_reader_t *reader, uint8_t *buf, size_t len);

typedef struct {
    _z_write_buf_t cache;  // Contiguous write cache; flushed into `bytes` as a single slice on append/finish.
    _z_bytes_t bytes;
} _z_bytes_writer_t;

bool _z_bytes_writer_is_empty(const _z_bytes_writer_t *writer);
bool _z_bytes_writer_check(const _z_bytes_writer_t *writer);
_z_bytes_writer_t _z_bytes_writer_from_bytes(_z_bytes_t *bytes);
_z_bytes_writer_t _z_bytes_writer_empty(void);
z_result_t _z_bytes_writer_write_all(_z_bytes_writer_t *writer, const uint8_t *src, size_t len);
z_result_t _z_bytes_writer_append_z_bytes(_z_bytes_writer_t *writer, _z_bytes_t *bytes);
z_result_t _z_bytes_writer_append_slice(_z_bytes_writer_t *writer, _z_slice_t *bytes);
_z_bytes_t _z_bytes_writer_finish(_z_bytes_writer_t *writer);
void _z_bytes_writer_clear(_z_bytes_writer_t *writer);
z_result_t _z_bytes_writer_move(_z_bytes_writer_t *dst, _z_bytes_writer_t *src);
size_t _z_bytes_reader_remaining(const _z_bytes_reader_t *reader);

// A view into bytes that does not own the underlying buffer and slices. It is the caller's responsibility to ensure
// that the viewed bytes outlive the view.
typedef struct _z_bytes_view_t {
    _z_bytes_t _target;
} _z_bytes_view_t;

static inline _z_bytes_view_t _z_bytes_view_null(void) {
    _z_bytes_view_t view;
    view._target = _z_bytes_null();
    return view;
}
static inline bool _z_bytes_view_check(const _z_bytes_view_t *bytes) { return _z_bytes_check(&bytes->_target); }

static inline _z_bytes_view_t _z_bytes_view_from_slice(const _z_slice_t *slice) {
    _z_bytes_view_t view_bytes;
    view_bytes._target = _z_bytes_null();
    _z_slice_t s = *slice;
    // append non-owning slice to the view bytes; the view will alias the slice's buffer without taking ownership of it.
    _z_bytes_append_slice(&view_bytes._target, &s);
    return view_bytes;
}

static inline _z_bytes_view_t _z_bytes_view_from_bytes(const _z_bytes_t *bytes) {
    _z_bytes_view_t view_bytes;
    if (bytes == NULL) {
        view_bytes._target = _z_bytes_null();
    } else {
        view_bytes._target = *bytes;
    }
    return view_bytes;
}

static inline const _z_bytes_t *_z_bytes_view_deref(const _z_bytes_view_t *bytes) { return &bytes->_target; }

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_COLLECTIONS_BYTES_H */
