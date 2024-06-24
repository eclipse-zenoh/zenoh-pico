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

#include "zenoh-pico/protocol/iobuf.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/utils/pointers.h"
#include "zenoh-pico/utils/result.h"

/*------------------ IOSli ------------------*/
_z_iosli_t _z_iosli_wrap(const uint8_t *buf, size_t length, size_t r_pos, size_t w_pos) {
    assert((r_pos <= w_pos) && (w_pos <= length));
    _z_iosli_t ios;
    ios._r_pos = r_pos;
    ios._w_pos = w_pos;
    ios._capacity = length;
    ios._is_alloc = false;
    ios._buf = (uint8_t *)buf;
    return ios;
}

void __z_iosli_init(_z_iosli_t *ios, size_t capacity) {
    ios->_r_pos = 0;
    ios->_w_pos = 0;
    ios->_capacity = capacity;
    ios->_is_alloc = true;
    ios->_buf = (uint8_t *)z_malloc(capacity);
    if (ios->_buf == NULL) {
        ios->_capacity = 0;
        ios->_is_alloc = false;
    }
}

_z_iosli_t _z_iosli_make(size_t capacity) {
    _z_iosli_t ios;
    __z_iosli_init(&ios, capacity);
    return ios;
}

_z_iosli_t *_z_iosli_new(size_t capacity) {
    _z_iosli_t *pios = (_z_iosli_t *)z_malloc(sizeof(_z_iosli_t));
    if (pios != NULL) {
        __z_iosli_init(pios, capacity);
    }
    return pios;
}

size_t _z_iosli_readable(const _z_iosli_t *ios) { return ios->_w_pos - ios->_r_pos; }

uint8_t _z_iosli_read(_z_iosli_t *ios) {
    assert(ios->_r_pos < ios->_w_pos);
    return ios->_buf[ios->_r_pos++];
}

void _z_iosli_read_bytes(_z_iosli_t *ios, uint8_t *dst, size_t offset, size_t length) {
    assert(_z_iosli_readable(ios) >= length);
    uint8_t *w_pos = _z_ptr_u8_offset(dst, (ptrdiff_t)offset);
    (void)memcpy(w_pos, ios->_buf + ios->_r_pos, length);
    ios->_r_pos = ios->_r_pos + length;
}

uint8_t _z_iosli_get(const _z_iosli_t *ios, size_t pos) {
    assert(pos < ios->_capacity);
    return ios->_buf[pos];
}

size_t _z_iosli_writable(const _z_iosli_t *ios) { return ios->_capacity - ios->_w_pos; }

void _z_iosli_write(_z_iosli_t *ios, uint8_t b) {
    assert(_z_iosli_writable(ios) >= (size_t)1);
    ios->_buf[ios->_w_pos] = b;
    ios->_w_pos += 1;
}

void _z_iosli_write_bytes(_z_iosli_t *ios, const uint8_t *bs, size_t offset, size_t length) {
    assert(_z_iosli_writable(ios) >= length);
    uint8_t *w_pos = _z_ptr_u8_offset(ios->_buf, (ptrdiff_t)ios->_w_pos);
    (void)memcpy(w_pos, _z_cptr_u8_offset(bs, (ptrdiff_t)offset), length);
    ios->_w_pos += length;
}

void _z_iosli_put(_z_iosli_t *ios, uint8_t b, size_t pos) {
    assert(pos < ios->_capacity);
    ios->_buf[pos] = b;
}

_z_slice_t _z_iosli_to_bytes(const _z_iosli_t *ios) {
    _z_slice_t a;
    a.len = _z_iosli_readable(ios);
    a.start = _z_cptr_u8_offset(ios->_buf, (ptrdiff_t)ios->_r_pos);
    return a;
}

void _z_iosli_reset(_z_iosli_t *ios) {
    ios->_r_pos = 0;
    ios->_w_pos = 0;
}

size_t _z_iosli_size(const _z_iosli_t *ios) {
    (void)(ios);
    return sizeof(_z_iosli_t);
}

void _z_iosli_clear(_z_iosli_t *ios) {
    if ((ios->_is_alloc == true) && (ios->_buf != NULL)) {
        z_free(ios->_buf);
        ios->_buf = NULL;
    }
}

void _z_iosli_free(_z_iosli_t **ios) {
    _z_iosli_t *ptr = *ios;

    if (ptr != NULL) {
        _z_iosli_clear(ptr);

        z_free(ptr);
        *ios = NULL;
    }
}

void _z_iosli_copy(_z_iosli_t *dst, const _z_iosli_t *src) {
    dst->_r_pos = src->_r_pos;
    dst->_w_pos = src->_w_pos;
    dst->_capacity = src->_capacity;
    dst->_is_alloc = src->_is_alloc;
    if (dst->_is_alloc == true) {
        dst->_buf = (uint8_t *)z_malloc(src->_capacity);
        if (dst->_buf != NULL) {
            (void)memcpy(dst->_buf, src->_buf, src->_capacity);
        }
    } else {
        dst->_buf = src->_buf;
    }
}

_z_iosli_t *_z_iosli_clone(const _z_iosli_t *src) {
    _z_iosli_t *dst = (_z_iosli_t *)z_malloc(_z_iosli_size(src));
    if (dst != NULL) {
        _z_iosli_copy(dst, src);
    }
    return dst;
}

/*------------------ ZBuf ------------------*/
_z_zbuf_t _z_zbuf_make(size_t capacity) {
    _z_zbuf_t zbf;
    zbf._ios = _z_iosli_make(capacity);
    return zbf;
}

_z_zbuf_t _z_zbuf_view(_z_zbuf_t *zbf, size_t length) {
    assert(_z_iosli_readable(&zbf->_ios) >= length);
    _z_zbuf_t v;
    v._ios = _z_iosli_wrap(_z_zbuf_get_rptr(zbf), length, 0, length);
    return v;
}
_z_zbuf_t _z_slice_as_zbuf(_z_slice_t slice) {
    return (_z_zbuf_t){._ios = {._buf = (uint8_t *)slice.start,  // Safety: `_z_zbuf_t` is an immutable buffer
                                ._is_alloc = false,
                                ._capacity = slice.len,
                                ._r_pos = 0,
                                ._w_pos = slice.len}};
}

size_t _z_zbuf_capacity(const _z_zbuf_t *zbf) { return zbf->_ios._capacity; }

size_t _z_zbuf_space_left(const _z_zbuf_t *zbf) { return _z_iosli_writable(&zbf->_ios); }

uint8_t const *_z_zbuf_start(const _z_zbuf_t *zbf) {
    return _z_ptr_u8_offset(zbf->_ios._buf, (ptrdiff_t)zbf->_ios._r_pos);
}
size_t _z_zbuf_len(const _z_zbuf_t *zbf) { return _z_iosli_readable(&zbf->_ios); }

_Bool _z_zbuf_can_read(const _z_zbuf_t *zbf) { return _z_zbuf_len(zbf) > (size_t)0; }

uint8_t _z_zbuf_read(_z_zbuf_t *zbf) { return _z_iosli_read(&zbf->_ios); }

void _z_zbuf_read_bytes(_z_zbuf_t *zbf, uint8_t *dest, size_t offset, size_t length) {
    _z_iosli_read_bytes(&zbf->_ios, dest, offset, length);
}

uint8_t _z_zbuf_get(const _z_zbuf_t *zbf, size_t pos) { return _z_iosli_get(&zbf->_ios, pos); }

size_t _z_zbuf_get_rpos(const _z_zbuf_t *zbf) { return zbf->_ios._r_pos; }

size_t _z_zbuf_get_wpos(const _z_zbuf_t *zbf) { return zbf->_ios._w_pos; }

void _z_zbuf_set_rpos(_z_zbuf_t *zbf, size_t r_pos) {
    assert(r_pos <= zbf->_ios._w_pos);
    zbf->_ios._r_pos = r_pos;
}

void _z_zbuf_set_wpos(_z_zbuf_t *zbf, size_t w_pos) {
    assert(w_pos <= zbf->_ios._capacity);
    zbf->_ios._w_pos = w_pos;
}

uint8_t *_z_zbuf_get_rptr(const _z_zbuf_t *zbf) { return zbf->_ios._buf + zbf->_ios._r_pos; }

uint8_t *_z_zbuf_get_wptr(const _z_zbuf_t *zbf) { return zbf->_ios._buf + zbf->_ios._w_pos; }

void _z_zbuf_reset(_z_zbuf_t *zbf) { _z_iosli_reset(&zbf->_ios); }

void _z_zbuf_clear(_z_zbuf_t *zbf) { _z_iosli_clear(&zbf->_ios); }

void _z_zbuf_compact(_z_zbuf_t *zbf) {
    if ((zbf->_ios._r_pos != 0) || (zbf->_ios._w_pos != 0)) {
        size_t len = _z_iosli_readable(&zbf->_ios);
        (void)memcpy(zbf->_ios._buf, _z_zbuf_get_rptr(zbf), len);
        _z_zbuf_set_rpos(zbf, 0);
        _z_zbuf_set_wpos(zbf, len);
    }
}

void _z_zbuf_free(_z_zbuf_t **zbf) {
    _z_zbuf_t *ptr = *zbf;

    if (ptr != NULL) {
        _z_iosli_clear(&ptr->_ios);

        z_free(ptr);
        *zbf = NULL;
    }
}

/*------------------ WBuf ------------------*/
void _z_wbuf_add_iosli(_z_wbuf_t *wbf, _z_iosli_t *ios) {
    wbf->_w_idx = wbf->_w_idx + 1;
    _z_iosli_vec_append(&wbf->_ioss, ios);
}

_z_iosli_t *__z_wbuf_new_iosli(size_t capacity) {
    _z_iosli_t *ios = (_z_iosli_t *)z_malloc(sizeof(_z_iosli_t));
    if (ios != NULL) {
        __z_iosli_init(ios, capacity);
    }
    return ios;
}

_z_iosli_t *_z_wbuf_get_iosli(const _z_wbuf_t *wbf, size_t idx) { return _z_iosli_vec_get(&wbf->_ioss, idx); }

size_t _z_wbuf_len_iosli(const _z_wbuf_t *wbf) { return _z_iosli_vec_len(&wbf->_ioss); }

_z_wbuf_t _z_wbuf_make(size_t capacity, _Bool is_expandable) {
    _z_wbuf_t wbf;
    if (is_expandable == true) {
        // Preallocate 4 slots, this is usually what we expect
        // when fragmenting a zenoh data message with attachment
        wbf._ioss = _z_iosli_vec_make(4);
        _z_wbuf_add_iosli(&wbf, __z_wbuf_new_iosli(capacity));
    } else {
        wbf._ioss = _z_iosli_vec_make(1);
        _z_wbuf_add_iosli(&wbf, __z_wbuf_new_iosli(capacity));
    }
    wbf._w_idx = 0;  // This __must__ come after adding ioslices to reset w_idx
    wbf._r_idx = 0;
    wbf._expansion_step = is_expandable ? capacity : 0;
    wbf._capacity = capacity;

    return wbf;
}

size_t _z_wbuf_capacity(const _z_wbuf_t *wbf) {
    size_t cap = 0;
    for (size_t i = 0; i < _z_wbuf_len_iosli(wbf); i++) {
        _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, i);
        cap = cap + ios->_capacity;
    }
    return cap;
}

size_t _z_wbuf_len(const _z_wbuf_t *wbf) {
    size_t len = 0;
    for (size_t i = wbf->_r_idx; i <= wbf->_w_idx; i++) {
        _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, i);
        len = len + _z_iosli_readable(ios);
    }
    return len;
}

size_t _z_wbuf_space_left(const _z_wbuf_t *wbf) {
    _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, wbf->_w_idx);
    return _z_iosli_writable(ios);
}

uint8_t _z_wbuf_read(_z_wbuf_t *wbf) {
    uint8_t ret = 0;

    do {
        assert(wbf->_r_idx <= wbf->_w_idx);
        _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, wbf->_r_idx);
        size_t readable = _z_iosli_readable(ios);
        if (readable > (size_t)0) {
            ret = _z_iosli_read(ios);
            break;
        } else {
            wbf->_r_idx = wbf->_r_idx + (size_t)1;
        }
    } while (1);

    return ret;
}

void _z_wbuf_read_bytes(_z_wbuf_t *wbf, uint8_t *dest, size_t offset, size_t length) {
    size_t loffset = offset;
    size_t llength = length;

    do {
        assert(wbf->_r_idx <= wbf->_w_idx);
        _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, wbf->_r_idx);
        size_t readable = _z_iosli_readable(ios);
        if (readable > (size_t)0) {
            size_t to_read = (readable <= llength) ? readable : llength;
            _z_iosli_read_bytes(ios, dest, loffset, to_read);
            loffset = loffset + to_read;
            llength = llength - to_read;
        } else {
            wbf->_r_idx = wbf->_r_idx + 1;
        }
    } while (llength > (size_t)0);
}

uint8_t _z_wbuf_get(const _z_wbuf_t *wbf, size_t pos) {
    size_t current = pos;
    size_t i = 0;

    _z_iosli_t *ios;
    do {
        assert(i < _z_iosli_vec_len(&wbf->_ioss));
        ios = _z_wbuf_get_iosli(wbf, i);
        if (current < ios->_capacity) {
            break;
        } else {
            current = current - ios->_capacity;
        }

        i = i + (size_t)1;
    } while (1);

    return _z_iosli_get(ios, current);
}

int8_t _z_wbuf_write(_z_wbuf_t *wbf, uint8_t b) {
    _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, wbf->_w_idx);
    size_t writable = _z_iosli_writable(ios);
    if (writable == (size_t)0) {
        wbf->_w_idx += 1;
        if (wbf->_ioss._len <= wbf->_w_idx) {
            if (wbf->_expansion_step != 0) {
                ios = __z_wbuf_new_iosli(wbf->_expansion_step);
                _z_iosli_vec_append(&wbf->_ioss, ios);
            } else {
                return _Z_ERR_TRANSPORT_NO_SPACE;
            }
        }
        ios = _z_wbuf_get_iosli(wbf, wbf->_w_idx);
    }
    _z_iosli_write(ios, b);
    return _Z_RES_OK;
}

int8_t _z_wbuf_write_bytes(_z_wbuf_t *wbf, const uint8_t *bs, size_t offset, size_t length) {
    int8_t ret = _Z_RES_OK;

    size_t loffset = offset;
    size_t llength = length;

    _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, wbf->_w_idx);
    size_t writable = _z_iosli_writable(ios);
    if (writable >= llength) {
        _z_iosli_write_bytes(ios, bs, loffset, llength);
    } else if (wbf->_expansion_step != 0) {
        _z_iosli_write_bytes(ios, bs, loffset, writable);
        llength = llength - writable;
        loffset = loffset + writable;
        while (llength > (size_t)0) {
            ios = __z_wbuf_new_iosli(wbf->_expansion_step);
            _z_wbuf_add_iosli(wbf, ios);

            writable = _z_iosli_writable(ios);
            if (llength < writable) {
                writable = llength;
            }

            _z_iosli_write_bytes(ios, bs, loffset, writable);
            llength = llength - writable;
            loffset = loffset + writable;
        }
    } else {
        ret = _Z_ERR_TRANSPORT_NO_SPACE;
    }

    return ret;
}

int8_t _z_wbuf_wrap_bytes(_z_wbuf_t *wbf, const uint8_t *bs, size_t offset, size_t length) {
    int8_t ret = _Z_RES_OK;

    _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, wbf->_w_idx);
    size_t writable = _z_iosli_writable(ios);
    ios->_capacity = ios->_w_pos;  // Block writing on this ioslice
                                   // The remaining space is allocated in a new ioslice

    _z_iosli_t wios = _z_iosli_wrap(bs, length, offset, offset + length);
    _z_wbuf_add_iosli(wbf, _z_iosli_clone(&wios));
    _z_wbuf_add_iosli(wbf, __z_wbuf_new_iosli(writable));

    return ret;
}

void _z_wbuf_put(_z_wbuf_t *wbf, uint8_t b, size_t pos) {
    size_t current = pos;
    size_t i = 0;
    _z_iosli_t *ios;
    do {
        assert(i < _z_iosli_vec_len(&wbf->_ioss));
        ios = _z_wbuf_get_iosli(wbf, i);
        if (current < ios->_capacity) {
            break;
        } else {
            current = current - ios->_capacity;
        }

        i = i + (size_t)1;
    } while (1);

    _z_iosli_put(ios, b, current);
}

size_t _z_wbuf_get_rpos(const _z_wbuf_t *wbf) {
    size_t pos = 0;
    _z_iosli_t *ios;
    for (size_t i = 0; i < wbf->_r_idx; i++) {
        ios = _z_wbuf_get_iosli(wbf, i);
        pos = pos + ios->_capacity;
    }
    ios = _z_wbuf_get_iosli(wbf, wbf->_r_idx);
    pos = pos + ios->_r_pos;
    return pos;
}

size_t _z_wbuf_get_wpos(const _z_wbuf_t *wbf) {
    size_t pos = 0;
    _z_iosli_t *ios;
    for (size_t i = 0; i < wbf->_w_idx; i++) {
        ios = _z_wbuf_get_iosli(wbf, i);
        pos = pos + ios->_capacity;
    }
    ios = _z_wbuf_get_iosli(wbf, wbf->_w_idx);
    pos = pos + ios->_w_pos;
    return pos;
}

void _z_wbuf_set_rpos(_z_wbuf_t *wbf, size_t pos) {
    size_t current = pos;
    size_t i = 0;
    do {
        assert(i <= wbf->_w_idx);
        _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, i);
        if (current <= ios->_w_pos) {
            wbf->_r_idx = i;
            ios->_r_pos = current;
            break;
        }

        ios->_r_pos = ios->_w_pos;
        current = current - ios->_capacity;

        i = i + (size_t)1;
    } while (1);
}

void _z_wbuf_set_wpos(_z_wbuf_t *wbf, size_t pos) {
    size_t current = pos;
    size_t i = 0;
    do {
        assert(i <= _z_iosli_vec_len(&wbf->_ioss));

        _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, i);
        if ((current <= ios->_capacity) && (current >= ios->_r_pos)) {
            wbf->_w_idx = i;
            ios->_w_pos = current;
            break;
        }

        ios->_w_pos = ios->_capacity;
        current = current - ios->_capacity;

        i = i + (size_t)1;
    } while (1);
}

_z_zbuf_t _z_wbuf_to_zbuf(const _z_wbuf_t *wbf) {
    size_t len = _z_wbuf_len(wbf);
    _z_zbuf_t zbf = _z_zbuf_make(len);
    for (size_t i = wbf->_r_idx; i <= wbf->_w_idx; i++) {
        _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, i);
        _z_iosli_write_bytes(&zbf._ios, ios->_buf, ios->_r_pos, _z_iosli_readable(ios));
    }
    return zbf;
}

int8_t _z_wbuf_siphon(_z_wbuf_t *dst, _z_wbuf_t *src, size_t length) {
    int8_t ret = _Z_RES_OK;

    for (size_t i = 0; i < length; i++) {
        ret = _z_wbuf_write(dst, _z_wbuf_read(src));
        if (ret != _Z_RES_OK) {
            break;
        }
    }

    return ret;
}

void _z_wbuf_copy(_z_wbuf_t *dst, const _z_wbuf_t *src) {
    dst->_capacity = src->_capacity;
    dst->_r_idx = src->_r_idx;
    dst->_w_idx = src->_w_idx;
    dst->_expansion_step = src->_expansion_step;
    _z_iosli_vec_copy(&dst->_ioss, &src->_ioss);
}

void _z_wbuf_reset(_z_wbuf_t *wbf) {
    wbf->_r_idx = 0;
    wbf->_w_idx = 0;

    // Reset to default iosli allocation
    for (size_t i = 0; i < _z_iosli_vec_len(&wbf->_ioss); i++) {
        _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, i);
        if (ios->_is_alloc == false) {
            _z_iosli_vec_remove(&wbf->_ioss, i);
        } else {
            _z_iosli_reset(ios);
        }
    }
}

void _z_wbuf_clear(_z_wbuf_t *wbf) { _z_iosli_vec_clear(&wbf->_ioss); }

void _z_wbuf_free(_z_wbuf_t **wbf) {
    _z_wbuf_t *ptr = *wbf;

    if (ptr != NULL) {
        _z_wbuf_clear(ptr);

        z_free(ptr);
        *wbf = NULL;
    }
}
