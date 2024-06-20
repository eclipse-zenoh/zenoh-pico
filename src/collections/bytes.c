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
#include <string.h>
#include <stdio.h> 

#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/result.h"
#include "zenoh-pico/protocol/codec/core.h"

/*-------- Bytes --------*/
_Bool _zz_bytes_check(const _zz_bytes_t *bytes) { 
    return !_zz_bytes_is_empty(bytes);
}

_zz_bytes_t _zz_bytes_null(void) {
    _zz_bytes_t b;
    b._slices = _z_arc_slice_svec_make(0);
    return b;
}

int8_t _zz_bytes_copy(_zz_bytes_t *dst, const _zz_bytes_t *src) {
    _z_arc_slice_svec_copy(&dst->_slices, &src->_slices);
    if (_z_arc_slice_svec_len(&dst->_slices) == _z_arc_slice_svec_len(&dst->_slices)) {
        return _Z_RES_OK;
    } else {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
}

_zz_bytes_t _zz_bytes_duplicate(const _zz_bytes_t *src) {
    _zz_bytes_t dst = _zz_bytes_null();
    _zz_bytes_copy(&dst, src);
    return dst;
}

size_t _zz_bytes_len(const _zz_bytes_t *bs) {
    size_t len = 0;
    for (size_t i = 0; i < _z_arc_slice_svec_len(&bs->_slices); i++) {
        const _z_arc_slice_t *s = _z_arc_slice_svec_get(&bs->_slices, i);
        len += _z_arc_slice_len(s);
    }
    return len;
}

_Bool _zz_bytes_is_empty(const _zz_bytes_t *bs) {
    for (size_t i = 0; i < _z_arc_slice_svec_len(&bs->_slices); i++) {
        const _z_arc_slice_t *s = _z_arc_slice_svec_get(&bs->_slices, i);
        if  ( _z_arc_slice_len(s) > 0) return false;
    }
    return true;
}

size_t _zz_bytes_num_slices(const _zz_bytes_t *bs) {
    return _z_arc_slice_svec_len(&bs->_slices);
}

_z_arc_slice_t* _zz_bytes_get_slice(const _zz_bytes_t *bs, size_t i) {
    if (i >= _zz_bytes_num_slices(bs)) return NULL;
    return _z_arc_slice_svec_get(&bs->_slices, i);
}

void _zz_bytes_clear(_zz_bytes_t *bytes) { _z_arc_slice_svec_clear(&bytes->_slices); }

void _zz_bytes_free(_zz_bytes_t **bs) {
    _zz_bytes_t *ptr = *bs;

    if (ptr != NULL) {
        _zz_bytes_clear(ptr);

        z_free(ptr);
        *bs = NULL;
    }
}

size_t _zz_bytes_to_buf(const _zz_bytes_t *bytes, uint8_t* dst, size_t len) {
    uint8_t* start = dst;

    for (size_t i = 0; i < _zz_bytes_num_slices(bytes) && len > 0; ++i) {
        // Recopy data
        _z_arc_slice_t *s = _zz_bytes_get_slice(bytes, i);
        size_t s_len = _z_arc_slice_len(s);
        size_t len_to_copy = len >= s_len ? s_len : len;
        memcpy(start, _z_arc_slice_data(s), len_to_copy);
        start += s_len;
        len -= len_to_copy;
    }

    return len;
}
_zz_bytes_t _zz_bytes_from_slice(_z_slice_t s) {
    _zz_bytes_t b = _zz_bytes_null();
    _z_arc_slice_t arc_s = _z_arc_slice_wrap(s, 0, s.len);
    _z_arc_slice_svec_append(&b._slices, &arc_s);
    return b;
}

_zz_bytes_t _zz_bytes_from_buf(uint8_t* src, size_t len) {
    _z_slice_t s = _z_slice_wrap(src, len);
    return _zz_bytes_from_slice(s);
}

_z_slice_t _zz_bytes_to_slice(const _zz_bytes_t *bytes) {
    // Allocate slice
    _z_slice_t ret = _z_slice_make(_zz_bytes_len(bytes));
    if (!_z_slice_check(ret)) {
        return ret;
    }
    uint8_t* start = (uint8_t*)ret.start;
    for (size_t i = 0; i < _zz_bytes_num_slices(bytes); ++i) {
        // Recopy data
        _z_arc_slice_t* s = _zz_bytes_get_slice(bytes, i);
        size_t s_len = _z_arc_slice_len(s);
        memcpy(start, _z_arc_slice_data(s), s_len);
        start += s_len;
    }

    return ret;
}

int8_t _zz_bytes_append_slice(_zz_bytes_t* dst, _z_arc_slice_t* s) {
    return _z_arc_slice_svec_append(&dst->_slices, s);
}

int8_t _zz_bytes_append_inner(_zz_bytes_t* dst, _zz_bytes_t* src) {
    _Bool success = true;
    for (size_t i = 0; i < _zz_bytes_num_slices(src); ++i) {
        _z_arc_slice_t *s = _zz_bytes_get_slice(src, i);
        success = success && _z_arc_slice_svec_append(&dst->_slices, s);
    }
    if (!success) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    _z_svec_release(&src->_slices);
    return _Z_RES_OK;
}

int8_t _zz_bytes_append(_zz_bytes_t* dst, _zz_bytes_t* src) {
    uint8_t l_buf[16]; 
    size_t l_len = _z_zsize_encode_buf(l_buf, _zz_bytes_len(src));
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
    int8_t res = _zz_bytes_append_inner(dst, src);
    if (res != _Z_RES_OK) {
        return res;
    }
    return _Z_RES_OK;
}




_zz_bytes_t _zz_bytes_serialize_from_pair(_zz_bytes_t* first, _zz_bytes_t* second) {
    _zz_bytes_t out = _zz_bytes_null();

    if (_zz_bytes_append(&out, first) != _Z_RES_OK) {
        _zz_bytes_clear(&out);
        _zz_bytes_clear(first);
    } else if (_zz_bytes_append(&out, second) != _Z_RES_OK) {
        _zz_bytes_clear(&out);
        _zz_bytes_clear(second);
    }

    return out;
}


int8_t _zz_bytes_deserialize_into_pair(const _zz_bytes_t* bs, _zz_bytes_t* first_out, _zz_bytes_t* second_out) {
    _zz_bytes_reader_t reader = _zz_bytes_get_reader(bs);
    int8_t res = _zz_bytes_reader_read_next(&reader, first_out);
    if (res != _Z_RES_OK) return res;
    res = _zz_bytes_reader_read_next(&reader, second_out);
    if (res != _Z_RES_OK) {
       _zz_bytes_clear(first_out);
    };
    return res;
}



uint8_t _zz_bytes_to_uint8(const _zz_bytes_t *bs) {
    uint8_t val = 0;
    _zz_bytes_to_buf(bs, &val, sizeof(val));
    return val;
}

// FIXME: int16+ endianness, Issue #420
uint16_t _zz_bytes_to_uint16(const _zz_bytes_t *bs) {
    uint16_t val = 0;
    _zz_bytes_to_buf(bs, (uint8_t*)&val, sizeof(val));
    return val;
}

uint32_t _zz_bytes_to_uint32(const _zz_bytes_t *bs) {
    uint32_t val = 0;
    _zz_bytes_to_buf(bs, (uint8_t*)&val, sizeof(val));
    return val;
}

uint64_t _zz_bytes_to_uint64(const _zz_bytes_t *bs) {
    uint64_t val = 0;
    _zz_bytes_to_buf(bs, (uint8_t*)&val, sizeof(val));
    return val;
}

float _zz_bytes_to_float(const _zz_bytes_t *bs) {
    float val = 0;
    _zz_bytes_to_buf(bs, (uint8_t*)&val, sizeof(val));
    return val;
}

double _zz_bytes_to_double(const _zz_bytes_t *bs) {
    double val = 0;
    _zz_bytes_to_buf(bs, (uint8_t*)&val, sizeof(val));
    return val;
}

_zz_bytes_t _zz_bytes_from_uint8(uint8_t val) {
    return _zz_bytes_from_buf(&val, sizeof(val));
}

_zz_bytes_t _zz_bytes_from_uint16(uint16_t val) {
    return _zz_bytes_from_buf((uint8_t*)&val, sizeof(val));
}

_zz_bytes_t _zz_bytes_from_uint32(uint32_t val) {
    return _zz_bytes_from_buf((uint8_t*)&val, sizeof(val));
}

_zz_bytes_t _zz_bytes_from_uint64(uint64_t val) {
    return _zz_bytes_from_buf((uint8_t*)&val, sizeof(val));
}

_zz_bytes_t _zz_bytes_from_float(float val) {
    return _zz_bytes_from_buf((uint8_t*)&val, sizeof(val));
}

_zz_bytes_t _zz_bytes_from_double(double val) {
    return _zz_bytes_from_buf((uint8_t*)&val, sizeof(val));
}

_zz_bytes_reader_t _zz_bytes_get_reader(const _zz_bytes_t *bytes) {
    _zz_bytes_reader_t r;
    r.bytes = bytes;
    r.byte_idx = 0;
    r.in_slice_idx = 0;
    return r;
}

int8_t _zz_bytes_reader_seek_forward(_zz_bytes_reader_t *reader, size_t offset) {
    size_t start_slice = reader->slice_idx;
    for (size_t i = start_slice; i < _zz_bytes_num_slices(reader->bytes); ++i) {
        _z_arc_slice_t* s = _zz_bytes_get_slice(reader->bytes, i);
        size_t remaining =  _z_arc_slice_len(s) - reader->in_slice_idx;
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

int8_t _zz_bytes_reader_seek_backward(_zz_bytes_reader_t *reader, size_t offset) {
    while (offset != 0) {
        if (reader->in_slice_idx == 0) {
            if (reader->slice_idx == 0) return _Z_ERR_DID_NOT_READ;
            reader->slice_idx--;
            _z_arc_slice_t* s = _zz_bytes_get_slice(reader->bytes, reader->slice_idx);
            s = _zz_bytes_get_slice(reader->bytes, reader->slice_idx);
            reader->in_slice_idx = _z_arc_slice_len(s);
        }
        
        if (offset > reader->in_slice_idx) {
            offset -= reader->in_slice_idx;
            reader->byte_idx -= reader->in_slice_idx;
            reader->in_slice_idx = 0;
        } else {
            offset = 0;
            reader->byte_idx -= offset;
            reader->in_slice_idx -= offset;
        }
    }
    return _Z_RES_OK;
}


int8_t _zz_bytes_reader_seek(_zz_bytes_reader_t *reader, int64_t offset, int origin) {
    switch (origin) {
        case SEEK_SET: {
            reader->byte_idx = 0;
            reader->in_slice_idx = 0;
            reader->slice_idx = 0;
            if (offset < 0) return _Z_ERR_DID_NOT_READ; 
            return _zz_bytes_reader_seek_forward(reader, offset);
        }
        case SEEK_CUR: {
            if (offset >= 0) return _zz_bytes_reader_seek_forward(reader, offset); 
            else return _zz_bytes_reader_seek_backward(reader, -offset);
        }
        case SEEK_END: {
            if (offset > 0) return _Z_ERR_DID_NOT_READ;
            else return _zz_bytes_reader_seek_backward(reader, -offset);
        }
        default: return _Z_ERR_GENERIC;
    }
}

int64_t _zz_bytes_reader_tell(const _zz_bytes_reader_t *reader) {
    return reader->byte_idx;
}

int8_t _zz_bytes_reader_read(_zz_bytes_reader_t *reader, uint8_t* buf, size_t len) {
    uint8_t* buf_start = buf;
    
    for (size_t i = reader->slice_idx; i < _zz_bytes_num_slices(reader->bytes); ++i) {
        _z_arc_slice_t* s = _zz_bytes_get_slice(reader->bytes, i);
        size_t remaining =  _z_arc_slice_len(s) - reader->in_slice_idx;
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


int8_t __read_single_byte(uint8_t* b, void* context) {
    _zz_bytes_reader_t* reader = (_zz_bytes_reader_t*)context;
    return _zz_bytes_reader_read(reader, b, 1);
}

int8_t _zz_bytes_reader_read_zint(_zz_bytes_reader_t *reader, _z_zint_t *zint) {
    return _z_zsize_decode_with_reader(zint, __read_single_byte, reader);
}

int8_t _zz_bytes_reader_read_slices(_zz_bytes_reader_t* reader, size_t len, _zz_bytes_t* out) {
    *out = _zz_bytes_null();
    int8_t res = _Z_RES_OK;
    
    for (size_t i = reader->slice_idx; i < _zz_bytes_num_slices(reader->bytes) && len > 0; ++i) {
        _z_arc_slice_t* s = _zz_bytes_get_slice(reader->bytes, i);
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
    if (res != _Z_RES_OK) _zz_bytes_clear(out);

    return res;
    
}

int8_t _zz_bytes_reader_read_next(_zz_bytes_reader_t* reader, _zz_bytes_t* out) {
    *out = _zz_bytes_null();
    _z_zint_t len;
    if (_zz_bytes_reader_read_zint(reader, &len) != _Z_RES_OK) {
        return _Z_ERR_DID_NOT_READ;
    }
    size_t offset = _zz_bytes_reader_tell(reader);

    return _zz_bytes_reader_read_slices(reader, len, out);
}