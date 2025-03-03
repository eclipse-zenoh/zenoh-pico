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

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/endianness.h"
#include "zenoh-pico/utils/pointers.h"
#include "zenoh-pico/utils/result.h"

void _z_default_deleter(void *data, void *context) {
    _ZP_UNUSED(context);
    z_free(data);
}

void _z_static_deleter(void *data, void *context) {
    _ZP_UNUSED(data);
    _ZP_UNUSED(context);
}

_z_delete_context_t _z_delete_context_default(void) { return _z_delete_context_create(_z_default_deleter, NULL); }
_z_delete_context_t _z_delete_context_static(void) { return _z_delete_context_create(_z_static_deleter, NULL); }

void _z_delete_context_delete(_z_delete_context_t *c, void *data) {
    if (!_z_delete_context_is_null(c)) {
        c->deleter(data, c->context);
    }
}

/*-------- Slice --------*/
z_result_t _z_slice_init(_z_slice_t *bs, size_t capacity) {
    assert(capacity != 0);
    bs->start = (uint8_t *)z_malloc(capacity);
    if (bs->start == NULL) {
        bs->len = 0;
        bs->_delete_context = _z_delete_context_null();
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    bs->len = capacity;
    bs->_delete_context = _z_delete_context_default();
    return _Z_RES_OK;
}

_z_slice_t _z_slice_make(size_t capacity) {
    _z_slice_t bs;
    (void)_z_slice_init(&bs, capacity);
    return bs;
}

_z_slice_t _z_slice_from_buf_custom_deleter(const uint8_t *p, size_t len, _z_delete_context_t dc) {
    _z_slice_t bs;
    bs.start = p;
    bs.len = len;
    bs._delete_context = dc;
    return bs;
}

_z_slice_t _z_slice_alias_buf(const uint8_t *p, size_t len) {
    return _z_slice_from_buf_custom_deleter(p, len, _z_delete_context_null());
}

_z_slice_t _z_slice_copy_from_buf(const uint8_t *p, size_t len) {
    _z_slice_t bs = _z_slice_alias_buf(p, len);
    return _z_slice_duplicate(&bs);
}

void _z_slice_clear(_z_slice_t *bs) {
    if (bs->start != NULL) {
        _z_delete_context_delete(&bs->_delete_context, (void *)bs->start);
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

z_result_t _z_slice_copy(_z_slice_t *dst, const _z_slice_t *src) {
    // Make sure dst slice is not init beforehand, or suffer memory leak
    z_result_t ret = _z_slice_init(dst, src->len);
    if (ret == _Z_RES_OK) {
        (void)memcpy((uint8_t *)dst->start, src->start, src->len);
    }
    return ret;
}

z_result_t _z_slice_n_copy(_z_slice_t *dst, const _z_slice_t *src, size_t offset, size_t len) {
    assert(offset + len <= src->len);
    // Make sure dst slice is not init beforehand, or suffer memory leak
    z_result_t ret = _z_slice_init(dst, len);
    if (ret == _Z_RES_OK) {
        const uint8_t *start = _z_cptr_u8_offset(src->start, (ptrdiff_t)offset);
        (void)memcpy((uint8_t *)dst->start, start, len);
    }
    return ret;
}

z_result_t _z_slice_move(_z_slice_t *dst, _z_slice_t *src) {
    // avoid moving of aliased slices
    if (!_z_slice_is_alloced(src)) {
        *dst = _z_slice_null();
        _z_slice_t csrc;
        _Z_RETURN_IF_ERR(_z_slice_copy(&csrc, src));
        *src = csrc;
    }
    *dst = *src;
    _z_slice_reset(src);
    return _Z_RES_OK;
}

_z_slice_t _z_slice_duplicate(const _z_slice_t *src) {
    _z_slice_t dst = _z_slice_null();
    _z_slice_copy(&dst, src);
    return dst;
}

_z_slice_t _z_slice_steal(_z_slice_t *b) {
    _z_slice_t ret = *b;
    *b = _z_slice_null();
    return ret;
}
bool _z_slice_eq(const _z_slice_t *left, const _z_slice_t *right) {
    return left->len == right->len && memcmp(left->start, right->start, left->len) == 0;
}

bool _z_slice_is_alloced(const _z_slice_t *s) { return !_z_delete_context_is_null(&s->_delete_context); }
