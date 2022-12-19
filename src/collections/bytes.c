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

#include "zenoh-pico/collections/bytes.h"

#include <stddef.h>
#include <string.h>

#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/result.h"

/*-------- bytes --------*/
_z_bytes_t _z_bytes_empty(void) { return (_z_bytes_t){.start = NULL, .len = 0, ._is_alloc = false}; }

int8_t _z_bytes_init(_z_bytes_t *bs, size_t capacity) {
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

_z_bytes_t _z_bytes_make(size_t capacity) {
    _z_bytes_t bs;
    (void)_z_bytes_init(&bs, capacity);
    return bs;
}

_z_bytes_t _z_bytes_wrap(const uint8_t *p, size_t len) {
    _z_bytes_t bs;
    bs.start = p;
    bs.len = len;
    bs._is_alloc = false;
    return bs;
}

void _z_bytes_reset(_z_bytes_t *bs) {
    bs->start = NULL;
    bs->len = 0;
    bs->_is_alloc = false;
}

void _z_bytes_clear(_z_bytes_t *bs) {
    if ((bs->_is_alloc == true) && (bs->start != NULL)) {
        z_free((uint8_t *)bs->start);
    }
    _z_bytes_reset(bs);
}

void _z_bytes_free(_z_bytes_t **bs) {
    _z_bytes_t *ptr = *bs;

    if (ptr != NULL) {
        _z_bytes_clear(ptr);

        z_free(ptr);
        *bs = NULL;
    }
}

void _z_bytes_copy(_z_bytes_t *dst, const _z_bytes_t *src) {
    int8_t ret = _z_bytes_init(dst, src->len);  // FIXME: it should check if dst is already initialized. Otherwise it will leak
    if (ret == _Z_RES_OK) {
        (void)memcpy((uint8_t *)dst->start, src->start, src->len);
    }
}

void _z_bytes_move(_z_bytes_t *dst, _z_bytes_t *src) {
    dst->start = src->start;
    dst->len = src->len;
    dst->_is_alloc = src->_is_alloc;

    _z_bytes_reset(src);
}

_z_bytes_t _z_bytes_duplicate(const _z_bytes_t *src) {
    _z_bytes_t dst;
    _z_bytes_copy(&dst, src);
    return dst;
}

_Bool _z_bytes_is_empty(const _z_bytes_t *bs) { return bs->len == 0; }
