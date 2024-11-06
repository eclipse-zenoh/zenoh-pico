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

_z_arc_slice_t _z_arc_slice_wrap(_z_slice_t s, size_t offset, size_t len) {
    assert(offset + len <= s.len);
    _z_arc_slice_t arc_s;

    arc_s.slice = _z_slice_simple_rc_new_from_val(&s);
    if (_Z_RC_IS_NULL(&arc_s.slice)) {
        return _z_arc_slice_empty();
    }
    arc_s.len = len;
    arc_s.start = offset;
    return arc_s;
}

_z_arc_slice_t _z_arc_slice_wrap_slice_rc(_z_slice_simple_rc_t* slice_rc, size_t offset, size_t len) {
    assert(offset + len <= _Z_RC_IN_VAL(slice_rc)->len);
    _z_arc_slice_t arc_s;
    arc_s.slice = _z_slice_simple_rc_clone(slice_rc);
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
    out.slice = _z_slice_simple_rc_clone(&s->slice);
    out.len = len;
    out.start = s->start + offset;
    return out;
}

const uint8_t* _z_arc_slice_data(const _z_arc_slice_t* s) { return _Z_RC_IN_VAL(&s->slice)->start + s->start; }

z_result_t _z_arc_slice_copy(_z_arc_slice_t* dst, const _z_arc_slice_t* src) {
    _z_slice_simple_rc_copy(&dst->slice, &src->slice);
    dst->len = src->len;
    dst->start = src->start;
    return _Z_RES_OK;
}

z_result_t _z_arc_slice_move(_z_arc_slice_t* dst, _z_arc_slice_t* src) {
    dst->slice = src->slice;
    dst->len = src->len;
    dst->start = src->start;
    src->len = 0;
    src->start = 0;
    src->slice = _z_slice_simple_rc_null();
    return _Z_RES_OK;
}

z_result_t _z_arc_slice_drop(_z_arc_slice_t* s) {
    _z_slice_simple_rc_drop(&s->slice);
    s->len = 0;
    return _Z_RES_OK;
}
