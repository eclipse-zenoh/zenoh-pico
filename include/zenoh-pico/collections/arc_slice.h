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
#include "zenoh-pico/system/platform-common.h"

_Z_REFCOUNT_DEFINE(_z_slice, _z_slice)

/*-------- ArcSlice --------*/
/**
 * An atomically reference counted subslice.
 *
 * Members:
 *   _z_slice_rc_t len: Rc counted slice.
 *   size_t start: Offset to the subslice start.
 *   size_t len: Length of the subslice.
 */

typedef struct {
    _z_slice_rc_t slice;
    size_t start;
    size_t len;
} _z_arc_slice_t;

_z_arc_slice_t _z_arc_slice_empty(void);
_z_arc_slice_t _z_arc_slice_wrap(_z_slice_t s, size_t offset, size_t len);
_z_arc_slice_t _z_arc_slice_get_subslice(const _z_arc_slice_t* s, size_t offset, size_t len);
size_t _z_arc_slice_len(const _z_arc_slice_t* s);
_Bool _z_arc_slice_is_empty(const _z_arc_slice_t* s);
const uint8_t* _z_arc_slice_data(const _z_arc_slice_t* s);
int8_t _z_arc_slice_copy(_z_arc_slice_t* dst, const _z_arc_slice_t* src);
int8_t _z_arc_slice_move(_z_arc_slice_t* dst, _z_arc_slice_t* src);
int8_t _z_arc_slice_drop(_z_arc_slice_t* s);

#endif /* ZENOH_PICO_COLLECTIONS_ARC_SLICE_H */
