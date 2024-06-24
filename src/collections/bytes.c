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

#include "zenoh-pico/collections/bytes.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "zenoh-pico/protocol/codec/core.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/endianness.h"
#include "zenoh-pico/utils/result.h"

/*-------- Bytes --------*/
_Bool _z_bytes_check(const _z_bytes_t *bytes) { return !_z_bytes_is_empty(bytes); }

_z_bytes_t _z_bytes_null(void) {
    _z_bytes_t b;
    b._slices = _z_arc_slice_svec_make(0);
    return b;
}

int8_t _z_bytes_copy(_z_bytes_t *dst, const _z_bytes_t *src) {
    _z_arc_slice_svec_copy(&dst->_slices, &src->_slices);
    if (_z_arc_slice_svec_len(&dst->_slices) == _z_arc_slice_svec_len(&dst->_slices)) {
        return _Z_RES_OK;
    } else {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
}

_z_bytes_t _z_bytes_duplicate(const _z_bytes_t *src) {
    _z_bytes_t dst = _z_bytes_null();
    _z_bytes_copy(&dst, src);
    return dst;
}

size_t _z_bytes_len(const _z_bytes_t *bs) {
    size_t len = 0;
    for (size_t i = 0; i < _z_arc_slice_svec_len(&bs->_slices); ++i) {
        const _z_arc_slice_t *s = _z_arc_slice_svec_get(&bs->_slices, i);
        len += _z_arc_slice_len(s);
    }
    return len;
}

_Bool _z_bytes_is_empty(const _z_bytes_t *bs) {
    for (size_t i = 0; i < _z_arc_slice_svec_len(&bs->_slices); i++) {
        const _z_arc_slice_t *s = _z_arc_slice_svec_get(&bs->_slices, i);
        if (_z_arc_slice_len(s) > 0) return false;
    }
    return true;
}

size_t _z_bytes_num_slices(const _z_bytes_t *bs) { return _z_arc_slice_svec_len(&bs->_slices); }

_z_arc_slice_t *_z_bytes_get_slice(const _z_bytes_t *bs, size_t i) {
    if (i >= _z_bytes_num_slices(bs)) return NULL;
    return _z_arc_slice_svec_get(&bs->_slices, i);
}

void _z_bytes_drop(_z_bytes_t *bytes) { _z_arc_slice_svec_clear(&bytes->_slices); }

void _z_bytes_free(_z_bytes_t **bs) {
    _z_bytes_t *ptr = *bs;

    if (ptr != NULL) {
        _z_bytes_drop(ptr);

        z_free(ptr);
        *bs = NULL;
    }
}

size_t _z_bytes_to_buf(const _z_bytes_t *bytes, uint8_t *dst, size_t len) {
    uint8_t *start = dst;

    for (size_t i = 0; i < _z_bytes_num_slices(bytes) && len > 0; ++i) {
        // Recopy data
        _z_arc_slice_t *s = _z_bytes_get_slice(bytes, i);
        size_t s_len = _z_arc_slice_len(s);
        size_t len_to_copy = len >= s_len ? s_len : len;
        memcpy(start, _z_arc_slice_data(s), len_to_copy);
        start += s_len;
        len -= len_to_copy;
    }

    return len;
}
int8_t _z_bytes_from_slice(_z_bytes_t *b, _z_slice_t s) {
    *b = _z_bytes_null();
    _z_arc_slice_t arc_s = _z_arc_slice_wrap(s, 0, s.len);
    if (_z_arc_slice_len(&arc_s) != s.len) return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    return _z_arc_slice_svec_append(&b->_slices, &arc_s) ? _Z_RES_OK : _Z_ERR_SYSTEM_OUT_OF_MEMORY;
}

int8_t _z_bytes_from_buf(_z_bytes_t *b, uint8_t *src, size_t len) {
    *b = _z_bytes_null();
    _z_slice_t s = _z_slice_wrap_copy(src, len);
    if (s.len != len) return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    return _z_bytes_from_slice(b, s);
}

int8_t _z_bytes_to_slice(const _z_bytes_t *bytes, _z_slice_t *s) {
    // Allocate slice
    size_t len = _z_bytes_len(bytes);
    *s = _z_slice_make(len);
    if (!_z_slice_check(*s) && len > 0) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    uint8_t *start = (uint8_t *)s->start;
    for (size_t i = 0; i < _z_bytes_num_slices(bytes); ++i) {
        // Recopy data
        _z_arc_slice_t *arc_s = _z_bytes_get_slice(bytes, i);
        size_t s_len = _z_arc_slice_len(arc_s);
        memcpy(start, _z_arc_slice_data(arc_s), s_len);
        start += s_len;
    }

    return _Z_RES_OK;
}

int8_t _z_bytes_append_slice(_z_bytes_t *dst, _z_arc_slice_t *s) { return _z_arc_slice_svec_append(&dst->_slices, s); }

int8_t _z_bytes_append_inner(_z_bytes_t *dst, _z_bytes_t *src) {
    _Bool success = true;
    for (size_t i = 0; i < _z_bytes_num_slices(src); ++i) {
        _z_arc_slice_t *s = _z_bytes_get_slice(src, i);
        success = success && _z_arc_slice_svec_append(&dst->_slices, s);
    }
    if (!success) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    _z_svec_release(&src->_slices);
    return _Z_RES_OK;
}

int8_t _z_bytes_append(_z_bytes_t *dst, _z_bytes_t *src) {
    uint8_t l_buf[16];
    size_t l_len = _z_zsize_encode_buf(l_buf, _z_bytes_len(src));
    _z_slice_t s = _z_slice_wrap_copy(l_buf, l_len);
    if (!_z_slice_check(s)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    _z_arc_slice_t arc_s = _z_arc_slice_wrap(s, 0, l_len);
    _z_arc_slice_svec_append(&dst->_slices, &arc_s);

    if (dst->_slices._val == NULL) {
        _z_arc_slice_drop(&arc_s);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    int8_t res = _z_bytes_append_inner(dst, src);
    if (res != _Z_RES_OK) {
        return res;
    }
    return _Z_RES_OK;
}

int8_t _z_bytes_serialize_from_pair(_z_bytes_t *out, _z_bytes_t *first, _z_bytes_t *second) {
    int8_t res = _z_bytes_append(out, first);
    if (res != _Z_RES_OK) {
        _z_bytes_drop(out);
        _z_bytes_drop(first);
        return res;
    }
    res = _z_bytes_append(out, second);
    if (res != _Z_RES_OK) {
        _z_bytes_drop(out);
        _z_bytes_drop(second);
    }

    return res;
}

int8_t _z_bytes_deserialize_into_pair(const _z_bytes_t *bs, _z_bytes_t *first_out, _z_bytes_t *second_out) {
    _z_bytes_reader_t reader = _z_bytes_get_reader(bs);
    int8_t res = _z_bytes_reader_read_next(&reader, first_out);
    if (res != _Z_RES_OK) return res;
    res = _z_bytes_reader_read_next(&reader, second_out);
    if (res != _Z_RES_OK) {
        _z_bytes_drop(first_out);
    };
    return res;
}

int8_t _z_bytes_to_uint8(const _z_bytes_t *bs, uint8_t *val) {
    *val = 0;
    return _z_bytes_to_buf(bs, val, sizeof(uint8_t)) == sizeof(uint8_t) ? _Z_RES_OK : _Z_ERR_DID_NOT_READ;
}

int8_t _z_bytes_to_uint16(const _z_bytes_t *bs, uint16_t *val) {
    *val = 0;
    uint8_t buf[sizeof(uint16_t)];
    if (_z_bytes_to_buf(bs, buf, sizeof(uint16_t)) != sizeof(uint16_t)) {
        return _Z_ERR_DID_NOT_READ;
    }
    *val = _z_host_le_load16(buf);
    return _Z_RES_OK;
}

int8_t _z_bytes_to_uint32(const _z_bytes_t *bs, uint32_t *val) {
    *val = 0;
    uint8_t buf[sizeof(uint32_t)];
    if (_z_bytes_to_buf(bs, buf, sizeof(uint32_t)) != sizeof(uint32_t)) {
        return _Z_ERR_DID_NOT_READ;
    }
    *val = _z_host_le_load32(buf);
    return _Z_RES_OK;
}

int8_t _z_bytes_to_uint64(const _z_bytes_t *bs, uint64_t *val) {
    *val = 0;
    uint8_t buf[sizeof(uint64_t)];
    if (_z_bytes_to_buf(bs, buf, sizeof(uint64_t)) != sizeof(uint64_t)) {
        return _Z_ERR_DID_NOT_READ;
    }
    *val = _z_host_le_load64(buf);
    return _Z_RES_OK;
}

int8_t _z_bytes_to_float(const _z_bytes_t *bs, float *val) {
    *val = 0;
    return _z_bytes_to_buf(bs, (uint8_t *)val, sizeof(float)) == sizeof(float) ? _Z_RES_OK : _Z_ERR_DID_NOT_READ;
}

int8_t _z_bytes_to_double(const _z_bytes_t *bs, double *val) {
    *val = 0;
    return _z_bytes_to_buf(bs, (uint8_t *)val, sizeof(double)) == sizeof(double) ? _Z_RES_OK : _Z_ERR_DID_NOT_READ;
}

int8_t _z_bytes_from_uint8(_z_bytes_t *b, uint8_t val) { return _z_bytes_from_buf(b, &val, sizeof(val)); }

int8_t _z_bytes_from_uint16(_z_bytes_t *b, uint16_t val) {
    uint8_t buf[sizeof(uint16_t)];
    _z_host_le_store16(val, buf);
    return _z_bytes_from_buf(b, buf, sizeof(uint16_t));
}

int8_t _z_bytes_from_uint32(_z_bytes_t *b, uint32_t val) {
    uint8_t buf[sizeof(uint32_t)];
    _z_host_le_store32(val, buf);
    return _z_bytes_from_buf(b, buf, sizeof(uint32_t));
}

int8_t _z_bytes_from_uint64(_z_bytes_t *b, uint64_t val) {
    uint8_t buf[sizeof(uint64_t)];
    _z_host_le_store64(val, buf);
    return _z_bytes_from_buf(b, buf, sizeof(uint64_t));
}

int8_t _z_bytes_from_float(_z_bytes_t *b, float val) { return _z_bytes_from_buf(b, (uint8_t *)&val, sizeof(val)); }

int8_t _z_bytes_from_double(_z_bytes_t *b, double val) { return _z_bytes_from_buf(b, (uint8_t *)&val, sizeof(val)); }

_z_slice_t _z_bytes_try_get_contiguous(const _z_bytes_t *bs) {
    if (_z_bytes_num_slices(bs) == 1) {
        _z_arc_slice_t *arc_s = _z_bytes_get_slice(bs, 0);
        return _z_slice_wrap(_z_arc_slice_data(arc_s), _z_arc_slice_len(arc_s));
    }
    return _z_slice_empty();
}

void _z_bytes_move(_z_bytes_t *dst, _z_bytes_t *src) {
    dst->_slices = src->_slices;
    *src = _z_bytes_null();
}

_z_bytes_reader_t _z_bytes_get_reader(const _z_bytes_t *bytes) {
    _z_bytes_reader_t r;
    r.bytes = bytes;
    r.slice_idx = 0;
    r.byte_idx = 0;
    r.in_slice_idx = 0;
    return r;
}

int8_t _z_bytes_reader_seek_forward(_z_bytes_reader_t *reader, size_t offset) {
    size_t start_slice = reader->slice_idx;
    for (size_t i = start_slice; i < _z_bytes_num_slices(reader->bytes); ++i) {
        _z_arc_slice_t *s = _z_bytes_get_slice(reader->bytes, i);
        size_t remaining = _z_arc_slice_len(s) - reader->in_slice_idx;
        if (offset >= remaining) {
            reader->slice_idx += 1;
            reader->in_slice_idx = 0;
            reader->byte_idx += remaining;
            offset -= remaining;
        } else {
            reader->in_slice_idx += offset;
            reader->byte_idx += offset;
            offset = 0;
        }
        if (offset == 0) break;
    }

    if (offset > 0) return _Z_ERR_DID_NOT_READ;
    return _Z_RES_OK;
}

int8_t _z_bytes_reader_seek_backward(_z_bytes_reader_t *reader, size_t offset) {
    while (offset != 0) {
        if (reader->in_slice_idx == 0) {
            if (reader->slice_idx == 0) return _Z_ERR_DID_NOT_READ;
            reader->slice_idx--;
            _z_arc_slice_t *s = _z_bytes_get_slice(reader->bytes, reader->slice_idx);
            reader->in_slice_idx = _z_arc_slice_len(s);
        }

        if (offset > reader->in_slice_idx) {
            offset -= reader->in_slice_idx;
            reader->byte_idx -= reader->in_slice_idx;
            reader->in_slice_idx = 0;
        } else {
            reader->byte_idx -= offset;
            reader->in_slice_idx -= offset;
            offset = 0;
        }
    }
    return _Z_RES_OK;
}

int8_t _z_bytes_reader_seek(_z_bytes_reader_t *reader, int64_t offset, int origin) {
    switch (origin) {
        case SEEK_SET: {
            reader->byte_idx = 0;
            reader->in_slice_idx = 0;
            reader->slice_idx = 0;
            if (offset < 0) return _Z_ERR_DID_NOT_READ;
            return _z_bytes_reader_seek_forward(reader, offset);
        }
        case SEEK_CUR: {
            if (offset >= 0)
                return _z_bytes_reader_seek_forward(reader, offset);
            else
                return _z_bytes_reader_seek_backward(reader, -offset);
        }
        case SEEK_END: {
            reader->byte_idx = _z_bytes_len(reader->bytes);
            reader->in_slice_idx = 0;
            reader->slice_idx = _z_bytes_num_slices(reader->bytes);
            if (offset > 0)
                return _Z_ERR_DID_NOT_READ;
            else
                return _z_bytes_reader_seek_backward(reader, -offset);
        }
        default:
            return _Z_ERR_GENERIC;
    }
}

int64_t _z_bytes_reader_tell(const _z_bytes_reader_t *reader) { return reader->byte_idx; }

int8_t _z_bytes_reader_read(_z_bytes_reader_t *reader, uint8_t *buf, size_t len) {
    uint8_t *buf_start = buf;

    for (size_t i = reader->slice_idx; i < _z_bytes_num_slices(reader->bytes); ++i) {
        _z_arc_slice_t *s = _z_bytes_get_slice(reader->bytes, i);
        size_t remaining = _z_arc_slice_len(s) - reader->in_slice_idx;
        if (len >= remaining) {
            memcpy(buf_start, _z_arc_slice_data(s) + reader->in_slice_idx, remaining);
            reader->slice_idx += 1;
            reader->in_slice_idx = 0;
            reader->byte_idx += remaining;
            len -= remaining;
            buf_start += remaining;
        } else {
            memcpy(buf_start, _z_arc_slice_data(s) + reader->in_slice_idx, len);
            reader->in_slice_idx += len;
            reader->byte_idx += len;
            len = 0;
        }
        if (len == 0) break;
    }

    if (len > 0) return _Z_ERR_DID_NOT_READ;
    return _Z_RES_OK;
}

int8_t __read_single_byte(uint8_t *b, void *context) {
    _z_bytes_reader_t *reader = (_z_bytes_reader_t *)context;
    return _z_bytes_reader_read(reader, b, 1);
}

int8_t _z_bytes_reader_read_zint(_z_bytes_reader_t *reader, _z_zint_t *zint) {
    return _z_zsize_decode_with_reader(zint, __read_single_byte, reader);
}

int8_t _z_bytes_reader_read_slices(_z_bytes_reader_t *reader, size_t len, _z_bytes_t *out) {
    *out = _z_bytes_null();
    int8_t res = _Z_RES_OK;

    for (size_t i = reader->slice_idx; i < _z_bytes_num_slices(reader->bytes) && len > 0; ++i) {
        _z_arc_slice_t *s = _z_bytes_get_slice(reader->bytes, i);
        size_t s_len = _z_arc_slice_len(s);

        size_t remaining = s_len - reader->in_slice_idx;
        size_t len_to_copy = remaining > len ? len : remaining;
        _z_arc_slice_t ss = _z_arc_slice_get_subslice(s, reader->in_slice_idx, len_to_copy);
        reader->in_slice_idx += len_to_copy;
        reader->byte_idx += len_to_copy;
        if (reader->in_slice_idx == s_len) {
            reader->slice_idx++;
            reader->in_slice_idx = 0;
        }

        if (ss.slice.in == NULL) {
            res = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
            break;
        }
        res = _z_arc_slice_svec_append(&out->_slices, &ss);
        if (res != _Z_RES_OK) {
            _z_arc_slice_drop(&ss);
            break;
        }
        len -= len_to_copy;
    }

    if (len > 0 && res == _Z_RES_OK) res = _Z_ERR_DID_NOT_READ;
    if (res != _Z_RES_OK) _z_bytes_drop(out);

    return res;
}

int8_t _z_bytes_reader_read_next(_z_bytes_reader_t *reader, _z_bytes_t *out) {
    *out = _z_bytes_null();
    _z_zint_t len;
    if (_z_bytes_reader_read_zint(reader, &len) != _Z_RES_OK) {
        return _Z_ERR_DID_NOT_READ;
    }

    return _z_bytes_reader_read_slices(reader, len, out);
}

_z_bytes_iterator_t _z_bytes_get_iterator(const _z_bytes_t *bytes) {
    return (_z_bytes_iterator_t){._reader = _z_bytes_get_reader(bytes)};
}

_Bool _z_bytes_iterator_next(_z_bytes_iterator_t *iter, _z_bytes_t *b) {
    return _z_bytes_reader_read_next(&iter->_reader, b) == _Z_RES_OK;
}
