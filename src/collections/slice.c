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
#include "zenoh-pico/utils/endianness.h"
#include "zenoh-pico/utils/result.h"

/*-------- Slice --------*/
_z_slice_t _z_slice_empty(void) { return (_z_slice_t){.start = NULL, .len = 0, ._is_alloc = false}; }

int8_t _z_slice_init(_z_slice_t *bs, size_t capacity) {
    int8_t ret = _Z_RES_OK;

    bs->start = capacity == 0 ? NULL : (uint8_t *)z_malloc(capacity);
    if (bs->start != NULL) {
        bs->len = capacity;
        bs->_is_alloc = true;
    } else {
        bs->len = 0;
        bs->_is_alloc = false;
    }

    if (bs->len != capacity) {
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

_z_slice_t _z_slice_wrap_copy(const uint8_t *p, size_t len) {
    _z_slice_t bs = _z_slice_wrap(p, len);
    return _z_slice_duplicate(&bs);
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

int8_t _z_slice_copy(_z_slice_t *dst, const _z_slice_t *src) {
    int8_t ret =
        _z_slice_init(dst, src->len);  // FIXME: it should check if dst is already initialized. Otherwise it will leak
    if (ret == _Z_RES_OK) {
        (void)memcpy((uint8_t *)dst->start, src->start, src->len);
    }
    return ret;
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
