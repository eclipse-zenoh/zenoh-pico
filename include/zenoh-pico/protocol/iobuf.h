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

#ifndef ZENOH_PICO_PROTOCOL_IOBUF_H
#define ZENOH_PICO_PROTOCOL_IOBUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/collections/arc_slice.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/collections/vec.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------ IOSli ------------------*/

typedef struct {
    size_t _r_pos;
    size_t _w_pos;
    size_t _capacity;
    uint8_t *_buf;
    bool _is_alloc;
} _z_iosli_t;

static inline _z_iosli_t _z_iosli_null(void) { return (_z_iosli_t){0}; }
_z_iosli_t _z_iosli_make(size_t capacity);
_z_iosli_t *_z_iosli_new(size_t capacity);
_z_iosli_t _z_iosli_wrap(const uint8_t *buf, size_t length, size_t r_pos, size_t w_pos);
_z_iosli_t _z_iosli_steal(_z_iosli_t *ios);

size_t _z_iosli_readable(const _z_iosli_t *ios);
uint8_t _z_iosli_read(_z_iosli_t *ios);
void _z_iosli_read_bytes(_z_iosli_t *ios, uint8_t *dest, size_t offset, size_t length);
void _z_iosli_copy_bytes(_z_iosli_t *dst, const _z_iosli_t *src);
uint8_t _z_iosli_get(const _z_iosli_t *ios, size_t pos);

size_t _z_iosli_writable(const _z_iosli_t *ios);
void _z_iosli_write(_z_iosli_t *ios, uint8_t b);
void _z_iosli_write_bytes(_z_iosli_t *ios, const uint8_t *bs, size_t offset, size_t length);
void _z_iosli_put(_z_iosli_t *ios, uint8_t b, size_t pos);
void _z_iosli_reset(_z_iosli_t *ios);

_z_slice_t _z_iosli_to_bytes(const _z_iosli_t *ios);

size_t _z_iosli_size(const _z_iosli_t *ios);
void _z_iosli_clear(_z_iosli_t *ios);
void _z_iosli_free(_z_iosli_t **ios);
void _z_iosli_copy(_z_iosli_t *dst, const _z_iosli_t *src);
_z_iosli_t *_z_iosli_clone(const _z_iosli_t *src);

_Z_ELEM_DEFINE(_z_iosli, _z_iosli_t, _z_iosli_size, _z_iosli_clear, _z_iosli_copy, _z_noop_move)
_Z_VEC_DEFINE(_z_iosli, _z_iosli_t)

/*------------------ ZBuf ------------------*/
typedef struct {
    _z_iosli_t _ios;
    _z_slice_simple_rc_t _slice;
} _z_zbuf_t;

static inline size_t _z_zbuf_get_ref_count(const _z_zbuf_t *zbf) { return _z_slice_simple_rc_count(&zbf->_slice); }
static inline _z_zbuf_t _z_zbuf_null(void) { return (_z_zbuf_t){0}; }
_z_zbuf_t _z_zbuf_make(size_t capacity);
_z_zbuf_t _z_zbuf_view(_z_zbuf_t *zbf, size_t length);
/// Constructs a _borrowing_ reader on `slice`
_z_zbuf_t _z_slice_as_zbuf(_z_slice_t slice);

size_t _z_zbuf_capacity(const _z_zbuf_t *zbf);
uint8_t const *_z_zbuf_start(const _z_zbuf_t *zbf);
size_t _z_zbuf_len(const _z_zbuf_t *zbf);
void _z_zbuf_copy_bytes(_z_zbuf_t *dst, const _z_zbuf_t *src);
bool _z_zbuf_can_read(const _z_zbuf_t *zbf);
size_t _z_zbuf_space_left(const _z_zbuf_t *zbf);

uint8_t _z_zbuf_read(_z_zbuf_t *zbf);
void _z_zbuf_read_bytes(_z_zbuf_t *zbf, uint8_t *dest, size_t offset, size_t length);
uint8_t _z_zbuf_get(const _z_zbuf_t *zbf, size_t pos);

size_t _z_zbuf_get_rpos(const _z_zbuf_t *zbf);
size_t _z_zbuf_get_wpos(const _z_zbuf_t *zbf);
void _z_zbuf_set_rpos(_z_zbuf_t *zbf, size_t r_pos);
void _z_zbuf_set_wpos(_z_zbuf_t *zbf, size_t w_pos);

uint8_t *_z_zbuf_get_rptr(const _z_zbuf_t *zbf);
uint8_t *_z_zbuf_get_wptr(const _z_zbuf_t *zbf);

void _z_zbuf_compact(_z_zbuf_t *zbf);
void _z_zbuf_reset(_z_zbuf_t *zbf);
void _z_zbuf_clear(_z_zbuf_t *zbf);
void _z_zbuf_free(_z_zbuf_t **zbf);

/*------------------ WBuf ------------------*/
typedef struct {
    _z_iosli_vec_t _ioss;
    size_t _r_idx;
    size_t _w_idx;
    size_t _capacity;
    size_t _expansion_step;
} _z_wbuf_t;

static inline _z_wbuf_t _z_wbuf_null(void) { return (_z_wbuf_t){0}; }
_z_wbuf_t _z_wbuf_make(size_t capacity, bool is_expandable);

size_t _z_wbuf_capacity(const _z_wbuf_t *wbf);
size_t _z_wbuf_len(const _z_wbuf_t *wbf);
size_t _z_wbuf_space_left(const _z_wbuf_t *wbf);

z_result_t _z_wbuf_write(_z_wbuf_t *wbf, uint8_t b);
z_result_t _z_wbuf_write_bytes(_z_wbuf_t *wbf, const uint8_t *bs, size_t offset, size_t length);
z_result_t _z_wbuf_wrap_bytes(_z_wbuf_t *wbf, const uint8_t *bs, size_t offset, size_t length);
void _z_wbuf_put(_z_wbuf_t *wbf, uint8_t b, size_t pos);

size_t _z_wbuf_get_rpos(const _z_wbuf_t *wbf);
size_t _z_wbuf_get_wpos(const _z_wbuf_t *wbf);
void _z_wbuf_set_rpos(_z_wbuf_t *wbf, size_t r_pos);
void _z_wbuf_set_wpos(_z_wbuf_t *wbf, size_t w_pos);

void _z_wbuf_add_iosli(_z_wbuf_t *wbf, _z_iosli_t *ios);
_z_iosli_t *_z_wbuf_get_iosli(const _z_wbuf_t *wbf, size_t idx);
size_t _z_wbuf_len_iosli(const _z_wbuf_t *wbf);

_z_zbuf_t _z_wbuf_to_zbuf(const _z_wbuf_t *wbf);
_z_zbuf_t _z_wbuf_moved_as_zbuf(_z_wbuf_t *wbf);
z_result_t _z_wbuf_siphon(_z_wbuf_t *dst, _z_wbuf_t *src, size_t length);

void _z_wbuf_copy(_z_wbuf_t *dst, const _z_wbuf_t *src);
void _z_wbuf_reset(_z_wbuf_t *wbf);
void _z_wbuf_clear(_z_wbuf_t *wbf);
void _z_wbuf_free(_z_wbuf_t **wbf);

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_PROTOCOL_IOBUF_H */
