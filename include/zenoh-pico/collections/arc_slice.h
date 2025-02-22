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

#ifndef ZENOH_PICO_COLLECTIONS_ARC_SLICE_H
#define ZENOH_PICO_COLLECTIONS_ARC_SLICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "refcount.h"
#include "slice.h"
#include "zenoh-pico/system/common/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

_Z_SIMPLE_REFCOUNT_DEFINE(_z_slice, _z_slice)

/*-------- ArcSlice --------*/
/**
 * An atomically reference counted subslice.
 *
 * Members:
 *   _z_slice_simple_rc_t len: Rc counted slice.
 *   size_t start: Offset to the subslice start.
 *   size_t len: Length of the subslice.
 */

typedef struct {
    _z_slice_simple_rc_t slice;
    size_t start;
    size_t len;
} _z_arc_slice_t;

static inline _z_arc_slice_t _z_arc_slice_empty(void) { return (_z_arc_slice_t){0}; }
static inline size_t _z_arc_slice_len(const _z_arc_slice_t* s) { return s->len; }
static inline bool _z_arc_slice_is_empty(const _z_arc_slice_t* s) { return _z_arc_slice_len(s) == 0; }
_z_arc_slice_t _z_arc_slice_wrap(_z_slice_t s, size_t offset, size_t len);
_z_arc_slice_t _z_arc_slice_wrap_slice_rc(_z_slice_simple_rc_t* slice_rc, size_t offset, size_t len);
_z_arc_slice_t _z_arc_slice_get_subslice(const _z_arc_slice_t* s, size_t offset, size_t len);
const uint8_t* _z_arc_slice_data(const _z_arc_slice_t* s);
z_result_t _z_arc_slice_copy(_z_arc_slice_t* dst, const _z_arc_slice_t* src);
z_result_t _z_arc_slice_move(_z_arc_slice_t* dst, _z_arc_slice_t* src);
z_result_t _z_arc_slice_drop(_z_arc_slice_t* s);

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_COLLECTIONS_ARC_SLICE_H */
