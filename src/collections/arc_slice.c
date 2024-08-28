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

#include "zenoh-pico/collections/arc_slice.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

_z_arc_slice_t _z_arc_slice_empty(void) {
    _z_arc_slice_t s;
    s.len = 0;
    s.start = 0;
    s.slice = _z_slice_rc_null();
    return s;
}

_z_arc_slice_t _z_arc_slice_wrap(_z_slice_t s, size_t offset, size_t len) {
    assert(offset + len <= s.len);
    _z_arc_slice_t arc_s;

    arc_s.slice = _z_slice_rc_new_from_val(&s);
    if (_Z_RC_IS_NULL(&arc_s.slice)) {
        return _z_arc_slice_empty();
    }
    arc_s.len = len;
    arc_s.start = offset;
    return arc_s;
}

_z_arc_slice_t _z_arc_slice_get_subslice(const _z_arc_slice_t* s, size_t offset, size_t len) {
    assert(offset + len <= s->len);
    assert(!_Z_RC_IS_NULL(&s->slice) || (len == 0 && offset == 0));

    if (_Z_RC_IS_NULL(&s->slice)) {
        return _z_arc_slice_empty();
    }
    _z_arc_slice_t out;
    out.slice = _z_slice_rc_clone(&s->slice);
    out.len = len;
    out.start = s->start + offset;
    return out;
}

size_t _z_arc_slice_len(const _z_arc_slice_t* s) { return s->len; }

_Bool _z_arc_slice_is_empty(const _z_arc_slice_t* s) { return _z_arc_slice_len(s) == 0; }

const uint8_t* _z_arc_slice_data(const _z_arc_slice_t* s) { return _Z_RC_IN_VAL(&s->slice)->start + s->start; }

int8_t _z_arc_slice_copy(_z_arc_slice_t* dst, const _z_arc_slice_t* src) {
    _z_slice_rc_copy(&dst->slice, &src->slice);
    dst->len = src->len;
    dst->start = src->start;
    return _Z_RES_OK;
}

int8_t _z_arc_slice_move(_z_arc_slice_t* dst, _z_arc_slice_t* src) {
    dst->slice = src->slice;
    dst->len = src->len;
    dst->start = src->start;
    src->len = 0;
    src->start = 0;
    src->slice = _z_slice_rc_null();
    return _Z_RES_OK;
}

int8_t _z_arc_slice_drop(_z_arc_slice_t* s) {
    _z_slice_rc_drop(&s->slice);
    *s = _z_arc_slice_empty();
    return _Z_RES_OK;
}
