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

#include "zenoh-pico/api/olv_macros.h"
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
    if (_z_arc_slice_svec_len(&dst->_slices) == _z_arc_slice_svec_len(&src->_slices)) {
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
    size_t remaining = len;
    for (size_t i = 0; i < _z_bytes_num_slices(bytes) && remaining > 0; ++i) {
        // Recopy data
        _z_arc_slice_t *s = _z_bytes_get_slice(bytes, i);
        size_t s_len = _z_arc_slice_len(s);
        size_t len_to_copy = remaining >= s_len ? s_len : remaining;
        memcpy(start, _z_arc_slice_data(s), len_to_copy);
        start += len_to_copy;
        remaining -= len_to_copy;
    }

    return len - remaining;
}
int8_t _z_bytes_from_slice(_z_bytes_t *b, _z_slice_t s) {
    *b = _z_bytes_null();
    _z_arc_slice_t arc_s = _z_arc_slice_wrap(s, 0, s.len);
    if (_z_arc_slice_len(&arc_s) != s.len) return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    return _z_arc_slice_svec_append(&b->_slices, &arc_s) ? _Z_RES_OK : _Z_ERR_SYSTEM_OUT_OF_MEMORY;
}

int8_t _z_bytes_from_buf(_z_bytes_t *b, const uint8_t *src, size_t len) {
    *b = _z_bytes_null();
    _z_slice_t s = _z_slice_wrap_copy(src, len);
    if (s.len != len) return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    return _z_bytes_from_slice(b, s);
}

int8_t _z_bytes_to_slice(const _z_bytes_t *bytes, _z_slice_t *s) {
    // Allocate slice
    size_t len = _z_bytes_len(bytes);
    *s = _z_slice_make(len);
    if (!_z_slice_check(s) && len > 0) {
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

int8_t _z_bytes_append_slice(_z_bytes_t *dst, _z_arc_slice_t *s) {
    _Z_CLEAN_RETURN_IF_ERR(_z_arc_slice_svec_append(&dst->_slices, s) ? _Z_RES_OK : _Z_ERR_SYSTEM_OUT_OF_MEMORY,
                           _z_arc_slice_drop(s));
    return _Z_RES_OK;
}

int8_t _z_bytes_append_bytes(_z_bytes_t *dst, _z_bytes_t *src) {
    int8_t res = _Z_RES_OK;
    for (size_t i = 0; i < _z_bytes_num_slices(src); ++i) {
        _z_arc_slice_t s;
        _z_arc_slice_move(&s, _z_bytes_get_slice(src, i));
        res = _z_bytes_append_slice(dst, &s);
        if (res != _Z_RES_OK) {
            break;
        }
    }

    _z_bytes_drop(src);
    return res;
}

int8_t _z_bytes_serialize_from_pair(_z_bytes_t *out, _z_bytes_t *first, _z_bytes_t *second) {
    *out = _z_bytes_null();
    _z_bytes_iterator_writer_t writer = _z_bytes_get_iterator_writer(out);
    _Z_CLEAN_RETURN_IF_ERR(_z_bytes_iterator_writer_write(&writer, first), _z_bytes_drop(second));
    _Z_CLEAN_RETURN_IF_ERR(_z_bytes_iterator_writer_write(&writer, second), _z_bytes_drop(out));

    return _Z_RES_OK;
}

int8_t _z_bytes_deserialize_into_pair(const _z_bytes_t *bs, _z_bytes_t *first_out, _z_bytes_t *second_out) {
    _z_bytes_iterator_t iter = _z_bytes_get_iterator(bs);
    int8_t res = _z_bytes_iterator_next(&iter, first_out);
    if (res != _Z_RES_OK) return res;
    res = _z_bytes_iterator_next(&iter, second_out);
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
            return _z_bytes_reader_seek_forward(reader, (size_t)offset);
        }
        case SEEK_CUR: {
            if (offset >= 0)
                return _z_bytes_reader_seek_forward(reader, (size_t)offset);
            else
                return _z_bytes_reader_seek_backward(reader, (size_t)(-offset));
        }
        case SEEK_END: {
            reader->byte_idx = _z_bytes_len(reader->bytes);
            reader->in_slice_idx = 0;
            reader->slice_idx = _z_bytes_num_slices(reader->bytes);
            if (offset > 0)
                return _Z_ERR_DID_NOT_READ;
            else
                return _z_bytes_reader_seek_backward(reader, (size_t)(-offset));
        }
        default:
            return _Z_ERR_GENERIC;
    }
}

int64_t _z_bytes_reader_tell(const _z_bytes_reader_t *reader) { return (int64_t)reader->byte_idx; }

size_t _z_bytes_reader_read(_z_bytes_reader_t *reader, uint8_t *buf, size_t len) {
    uint8_t *buf_start = buf;
    size_t to_read = len;
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

    return to_read - len;
}

int8_t __read_single_byte(uint8_t *b, void *context) {
    _z_bytes_reader_t *reader = (_z_bytes_reader_t *)context;
    return _z_bytes_reader_read(reader, b, 1) == 1 ? _Z_RES_OK : _Z_ERR_DID_NOT_READ;
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

        if (_Z_RC_IS_NULL(&ss.slice)) {
            res = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
            break;
        }

        res = _z_bytes_append_slice(out, &ss);
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

_z_bytes_iterator_t _z_bytes_get_iterator(const _z_bytes_t *bytes) {
    return (_z_bytes_iterator_t){._reader = _z_bytes_get_reader(bytes)};
}

int8_t _z_bytes_iterator_next(_z_bytes_iterator_t *iter, _z_bytes_t *b) {
    *b = _z_bytes_null();
    _z_zint_t len;
    if (_z_bytes_reader_read_zint(&iter->_reader, &len) != _Z_RES_OK) {
        return _Z_ERR_DID_NOT_READ;
    }
    return _z_bytes_reader_read_slices(&iter->_reader, len, b);
}

_z_bytes_writer_t _z_bytes_get_writer(_z_bytes_t *bytes, size_t cache_size) {
    return (_z_bytes_writer_t){.cache = NULL, .cache_size = cache_size, .bytes = bytes};
}

int8_t _z_bytes_writer_ensure_cache(_z_bytes_writer_t *writer) {
    // first we check if cache stayed untouched since previous write operation
    if (writer->cache != NULL) {
        _z_arc_slice_t *arc_s = _z_bytes_get_slice(writer->bytes, _z_bytes_num_slices(writer->bytes) - 1);
        if (_Z_RC_IN_VAL(&arc_s->slice)->start + arc_s->len == writer->cache) {
            size_t remaining_in_cache = _Z_RC_IN_VAL(&arc_s->slice)->len - arc_s->len;
            if (remaining_in_cache > 0) return _Z_RES_OK;
        }
    }
    // otherwise we allocate a new cache
    assert(writer->cache_size > 0);
    _z_slice_t s = _z_slice_make(writer->cache_size);
    if (s.start == NULL) return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    _z_arc_slice_t cache = _z_arc_slice_wrap(s, 0, 0);
    if (_Z_RC_IS_NULL(&cache.slice)) {
        _z_slice_clear(&s);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    _Z_CLEAN_RETURN_IF_ERR(_z_bytes_append_slice(writer->bytes, &cache), _z_arc_slice_drop(&cache));
    writer->cache = (uint8_t *)_Z_RC_IN_VAL(&cache.slice)->start;
    return _Z_RES_OK;
}

int8_t _z_bytes_writer_write(_z_bytes_writer_t *writer, const uint8_t *src, size_t len) {
    if (writer->cache_size == 0) {  // no cache - append data as a single slice
        _z_slice_t s = _z_slice_wrap_copy(src, len);
        if (s.len != len) return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
        _z_arc_slice_t arc_s = _z_arc_slice_wrap(s, 0, len);
        if _Z_RC_IS_NULL (&arc_s.slice) {
            _z_slice_clear(&s);
            return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
        }
        return _z_bytes_append_slice(writer->bytes, &arc_s);
    }

    while (len > 0) {
        _Z_RETURN_IF_ERR(_z_bytes_writer_ensure_cache(writer));
        _z_arc_slice_t *arc_s = _z_bytes_get_slice(writer->bytes, _z_bytes_num_slices(writer->bytes) - 1);
        size_t remaining_in_cache = _Z_RC_IN_VAL(&arc_s->slice)->len - arc_s->len;
        size_t to_copy = remaining_in_cache < len ? remaining_in_cache : len;
        memcpy(writer->cache, src, to_copy);
        len -= to_copy;
        arc_s->len += to_copy;
        src += to_copy;
        writer->cache += to_copy;
    }
    return _Z_RES_OK;
}

_z_bytes_iterator_writer_t _z_bytes_get_iterator_writer(_z_bytes_t *bytes) {
    return (_z_bytes_iterator_writer_t){.writer = _z_bytes_get_writer(bytes, 0)};
}
int8_t _z_bytes_iterator_writer_write(_z_bytes_iterator_writer_t *writer, _z_bytes_t *src) {
    uint8_t l_buf[16];
    size_t l_len = _z_zsize_encode_buf(l_buf, _z_bytes_len(src));
    _Z_CLEAN_RETURN_IF_ERR(_z_bytes_writer_write(&writer->writer, l_buf, l_len), _z_bytes_drop(src));
    _Z_CLEAN_RETURN_IF_ERR(_z_bytes_append_bytes(writer->writer.bytes, src), _z_bytes_drop(src));

    return _Z_RES_OK;
}
