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

#include "zenoh-pico/collections/slice.h"

#include <stddef.h>
#include <string.h>

#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/result.h"

/*-------- Slice --------*/
_z_slice_t _z_slice_empty(void) { return (_z_slice_t){.start = NULL, .len = 0, ._is_alloc = false}; }

int8_t _z_slice_init(_z_slice_t *bs, size_t capacity) {
    int8_t ret = _Z_RES_OK;

    bs->start = (uint8_t *)z_malloc(capacity);
    if (bs->start != NULL) {
        bs->len = capacity;
        bs->_is_alloc = true;
    } else {
        bs->len = 0;
        bs->_is_alloc = false;
        ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    return ret;
}

_z_slice_t _z_slice_make(size_t capacity) {
    _z_slice_t bs;
    (void)_z_slice_init(&bs, capacity);
    return bs;
}

_z_slice_t _z_slice_wrap(const uint8_t *p, size_t len) {
    _z_slice_t bs;
    bs.start = p;
    bs.len = len;
    bs._is_alloc = false;
    return bs;
}

void _z_slice_reset(_z_slice_t *bs) {
    bs->start = NULL;
    bs->len = 0;
    bs->_is_alloc = false;
}

void _z_slice_clear(_z_slice_t *bs) {
    if ((bs->_is_alloc == true) && (bs->start != NULL)) {
        z_free((uint8_t *)bs->start);
    }
    _z_slice_reset(bs);
}

void _z_slice_free(_z_slice_t **bs) {
    _z_slice_t *ptr = *bs;

    if (ptr != NULL) {
        _z_slice_clear(ptr);

        z_free(ptr);
        *bs = NULL;
    }
}

void _z_slice_copy(_z_slice_t *dst, const _z_slice_t *src) {
    int8_t ret =
        _z_slice_init(dst, src->len);  // FIXME: it should check if dst is already initialized. Otherwise it will leak
    if (ret == _Z_RES_OK) {
        (void)memcpy((uint8_t *)dst->start, src->start, src->len);
    }
}

void _z_slice_move(_z_slice_t *dst, _z_slice_t *src) {
    dst->start = src->start;
    dst->len = src->len;
    dst->_is_alloc = src->_is_alloc;

    _z_slice_reset(src);
}

_z_slice_t _z_slice_duplicate(const _z_slice_t *src) {
    _z_slice_t dst = _z_slice_empty();
    _z_slice_copy(&dst, src);
    return dst;
}

_Bool _z_slice_is_empty(const _z_slice_t *bs) { return bs->len == 0; }
_z_slice_t _z_slice_steal(_z_slice_t *b) {
    _z_slice_t ret = *b;
    *b = _z_slice_empty();
    return ret;
}
_Bool _z_slice_eq(const _z_slice_t *left, const _z_slice_t *right) {
    return left->len == right->len && memcmp(left->start, right->start, left->len) == 0;
}

/*-------- Bytes --------*/
_Bool _z_bytes_check(_z_bytes_t bytes) { return _z_slice_check(bytes._slice); }

_z_bytes_t _z_bytes_null(void) {
    return (_z_bytes_t){
        ._slice = _z_slice_empty(),
    };
}

_z_bytes_t _z_bytes_make(size_t capacity) {
    return (_z_bytes_t){
        ._slice = _z_slice_make(capacity),
    };
}

void _z_bytes_copy(_z_bytes_t *dst, const _z_bytes_t *src) {
    // Init only if needed
    if (!_z_slice_check(dst->_slice)) {
        if (_z_slice_init(&dst->_slice, src->_slice.len) != _Z_RES_OK) {
            return;
        }
    }
    (void)memcpy((uint8_t *)dst->_slice.start, src->_slice.start, src->_slice.len);
}

_z_bytes_t _z_bytes_duplicate(const _z_bytes_t *src) {
    _z_bytes_t dst = _z_bytes_null();
    _z_bytes_copy(&dst, src);
    return dst;
}

void _z_bytes_move(_z_bytes_t *dst, _z_bytes_t *src) { _z_slice_move(&dst->_slice, &src->_slice); }

void _z_bytes_clear(_z_bytes_t *bytes) { _z_slice_clear(&bytes->_slice); }

void _z_bytes_free(_z_bytes_t **bs) {
    _z_bytes_t *ptr = *bs;

    if (ptr != NULL) {
        _z_bytes_clear(ptr);

        z_free(ptr);
        *bs = NULL;
    }
}

uint8_t _z_bytes_to_uint8(const _z_bytes_t *bs) {
    uint8_t val = 0;
    memcpy(&val, bs->_slice.start, sizeof(val));
    return val;
}

uint16_t _z_bytes_to_uint16(const _z_bytes_t *bs) {
    uint16_t val = 0;
    memcpy(&val, bs->_slice.start, sizeof(val));
    return val;
}

uint32_t _z_bytes_to_uint32(const _z_bytes_t *bs) {
    uint32_t val = 0;
    memcpy(&val, bs->_slice.start, sizeof(val));
    return val;
}

uint64_t _z_bytes_to_uint64(const _z_bytes_t *bs) {
    uint64_t val = 0;
    memcpy(&val, bs->_slice.start, sizeof(val));
    return val;
}

float _z_bytes_to_float(const _z_bytes_t *bs) {
    float val = 0;
    memcpy(&val, bs->_slice.start, sizeof(val));
    return val;
}

double _z_bytes_to_double(const _z_bytes_t *bs) {
    double val = 0;
    memcpy(&val, bs->_slice.start, sizeof(val));
    return val;
}

_z_bytes_t _z_bytes_from_uint8(uint8_t val) {
    _z_bytes_t ret = _z_bytes_null();
    // Init bytes array
    if (_z_slice_init(&ret._slice, sizeof(val)) != _Z_RES_OK) {
        return ret;
    }
    // Encode int
    memcpy((uint8_t *)ret._slice.start, &val, sizeof(val));
    return ret;
}

_z_bytes_t _z_bytes_from_uint16(uint16_t val) {
    _z_bytes_t ret = _z_bytes_null();
    // Init bytes array
    if (_z_slice_init(&ret._slice, sizeof(val)) != _Z_RES_OK) {
        return ret;
    }
    // Encode int
    memcpy((uint8_t *)ret._slice.start, &val, sizeof(val));
    return ret;
}

_z_bytes_t _z_bytes_from_uint32(uint32_t val) {
    _z_bytes_t ret = _z_bytes_null();
    // Init bytes array
    if (_z_slice_init(&ret._slice, sizeof(val)) != _Z_RES_OK) {
        return ret;
    }
    // Encode int
    memcpy((uint8_t *)ret._slice.start, &val, sizeof(val));
    return ret;
}

_z_bytes_t _z_bytes_from_uint64(uint64_t val) {
    _z_bytes_t ret = _z_bytes_null();
    // Init bytes array
    if (_z_slice_init(&ret._slice, sizeof(val)) != _Z_RES_OK) {
        return ret;
    }
    // Encode int
    memcpy((uint8_t *)ret._slice.start, &val, sizeof(val));
    return ret;
}

_z_bytes_t _z_bytes_from_float(float val) {
    _z_bytes_t ret = _z_bytes_null();
    // Init bytes array
    if (_z_slice_init(&ret._slice, sizeof(val)) != _Z_RES_OK) {
        return ret;
    }
    // Encode float
    memcpy((uint8_t *)ret._slice.start, &val, sizeof(val));
    return ret;
}

_z_bytes_t _z_bytes_from_double(double val) {
    _z_bytes_t ret = _z_bytes_null();
    // Init bytes array
    if (_z_slice_init(&ret._slice, sizeof(val)) != _Z_RES_OK) {
        return ret;
    }
    // Encode double
    memcpy((uint8_t *)ret._slice.start, &val, sizeof(val));
    return ret;
}