//
// Copyright (c) 2026 ZettaScale Technology
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

#ifndef ZENOH_PICO_COLLECTIONS_BUF_H
#define ZENOH_PICO_COLLECTIONS_BUF_H

#include <stdbool.h>
#include <stdint.h>

#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/result.h"

#define _ZP_VECTOR_TEMPLATE_ELEM_TYPE uint8_t
#define _ZP_VECTOR_TEMPLATE_NAME _z_uint8_vec
#define _ZP_VECTOR_TEMPLATE_ALLOC_FN z_malloc
#define _ZP_VECTOR_TEMPLATE_FREE_FN z_free
#define _ZP_VECTOR_TEMPLATE_REALLOC_FN z_realloc
#include "zenoh-pico/collections/vector_template.h"

typedef struct _z_write_buf_t {
    _z_uint8_vec_t _buffer;
} _z_write_buf_t;

static inline z_result_t _z_write_buf_init(_z_write_buf_t *buf, size_t initial_capacity) {
    return _z_uint8_vec_init_with_capacity(&buf->_buffer, initial_capacity) ? _Z_RES_OK : _Z_ERR_SYSTEM_OUT_OF_MEMORY;
}

static inline z_result_t _z_write_buf_ensure_space(_z_write_buf_t *buf, size_t space) {
    return _z_uint8_vec_reserve(&buf->_buffer, _z_uint8_vec_size(&buf->_buffer) + space) ? _Z_RES_OK
                                                                                         : _Z_ERR_SYSTEM_OUT_OF_MEMORY;
}

static inline z_result_t _z_write_buf_write_byte(_z_write_buf_t *buf, uint8_t b) {
    return _z_uint8_vec_push_back(&buf->_buffer, &b) ? _Z_RES_OK : _Z_ERR_SYSTEM_OUT_OF_MEMORY;
}

static inline z_result_t _z_write_buf_write_bytes(_z_write_buf_t *buf, const uint8_t *data, size_t len) {
    return _z_uint8_vec_append(&buf->_buffer, (uint8_t *)data, len) ? _Z_RES_OK : _Z_ERR_SYSTEM_OUT_OF_MEMORY;
}

static inline void _z_write_buf_destroy(_z_write_buf_t *buf) { _z_uint8_vec_destroy(&buf->_buffer); }

// Transfers ownership of the underlying buffer into an owned slice and resets the
// write buffer to an empty state. The returned slice owns the memory and must be
// cleared by its new owner. Returns a null slice if the write buffer is empty (any
// reserved-but-unused buffer is freed in that case).
static inline _z_slice_t _z_write_buf_into_slice(_z_write_buf_t *buf) {
    size_t len = _z_uint8_vec_size(&buf->_buffer);
    if (len == 0) {
        _z_uint8_vec_destroy(&buf->_buffer);
        return _z_slice_null();
    }
    _z_slice_t s = _z_slice_from_buf_custom_deleter(_z_uint8_vec_data(&buf->_buffer), len, _z_delete_context_default());
    _z_uint8_vec_init(&buf->_buffer);  // reset without freeing; ownership moved to the slice
    return s;
}

typedef struct _z_read_buf_t {
    _z_slice_view_t _buffer;
    size_t _read_pos;
} _z_read_buf_t;

static inline _z_read_buf_t _z_read_buf_new(const uint8_t *data, size_t len) {
    _z_read_buf_t buf;
    buf._buffer = _z_slice_view_make(data, len);
    buf._read_pos = 0;
    return buf;
}

static inline size_t _z_read_buf_remaining_len(_z_read_buf_t *buf) {
    return _z_slice_view_deref(&buf->_buffer)->len - buf->_read_pos;
}

static inline z_result_t _z_read_buf_read_byte(_z_read_buf_t *buf, uint8_t *out) {
    if (_z_read_buf_remaining_len(buf) == 0) {
        return _Z_ERR_DID_NOT_READ;
    }
    *out = _z_slice_view_deref(&buf->_buffer)->start[buf->_read_pos];
    buf->_read_pos++;
    return _Z_RES_OK;
}

static inline z_result_t _z_read_buf_read_bytes(_z_read_buf_t *buf, _z_slice_view_t *bytes, size_t len) {
    if (_z_read_buf_remaining_len(buf) < len) {
        return _Z_ERR_DID_NOT_READ;
    }
    *bytes = _z_slice_view_make(_z_slice_view_deref(&buf->_buffer)->start + buf->_read_pos, len);
    buf->_read_pos += len;
    return _Z_RES_OK;
}

static inline _z_read_buf_t _z_write_buf_into_read_buf(const _z_write_buf_t *wb) {
    return _z_read_buf_new(_z_uint8_vec_const_data(&wb->_buffer), _z_uint8_vec_size(&wb->_buffer));
}

#endif
