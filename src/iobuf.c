/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "zenoh-pico/private/iobuf.h"

/*------------------ IOSli ------------------*/
_z_iosli_t _z_iosli_wrap(uint8_t *buf, size_t capacity, size_t r_pos, size_t w_pos)
{
    assert(r_pos <= w_pos && w_pos <= capacity);
    _z_iosli_t ios;
    ios.r_pos = r_pos;
    ios.w_pos = w_pos;
    ios.capacity = capacity;
    ios.is_alloc = 0;
    ios.buf = buf;
    return ios;
}

_z_iosli_t _z_iosli_make(size_t capacity)
{
    _z_iosli_t ios = _z_iosli_wrap((uint8_t *)malloc(capacity), capacity, 0, 0);
    ios.is_alloc = 1;
    return ios;
}

size_t _z_iosli_readable(const _z_iosli_t *ios)
{
    return ios->w_pos - ios->r_pos;
}

uint8_t _z_iosli_read(_z_iosli_t *ios)
{
    assert(ios->r_pos < ios->w_pos);
    return ios->buf[ios->r_pos++];
}

void _z_iosli_read_bytes(_z_iosli_t *ios, uint8_t *dst, size_t offset, size_t length)
{
    assert(_z_iosli_readable(ios) >= length);
    memcpy(dst + offset, ios->buf + ios->r_pos, length);
    ios->r_pos += length;
}

uint8_t _z_iosli_get(const _z_iosli_t *ios, size_t pos)
{
    assert(pos < ios->capacity);
    return ios->buf[pos];
}

size_t _z_iosli_writable(const _z_iosli_t *ios)
{
    return ios->capacity - ios->w_pos;
}

void _z_iosli_write(_z_iosli_t *ios, uint8_t b)
{
    assert(_z_iosli_writable(ios) >= 1);
    ios->buf[ios->w_pos++] = b;
}

void _z_iosli_write_bytes(_z_iosli_t *ios, const uint8_t *bs, size_t offset, size_t length)
{
    assert(_z_iosli_writable(ios) >= length);
    memcpy(ios->buf + ios->w_pos, bs + offset, length);
    ios->w_pos += length;
}

void _z_iosli_put(_z_iosli_t *ios, uint8_t b, size_t pos)
{
    assert(pos < ios->capacity);
    ios->buf[pos] = b;
}

z_bytes_t _z_iosli_to_bytes(const _z_iosli_t *ios)
{
    z_bytes_t a;
    a.len = _z_iosli_readable(ios);
    a.val = ios->buf + ios->r_pos;
    return a;
}

void _z_iosli_clear(_z_iosli_t *ios)
{
    ios->r_pos = 0;
    ios->w_pos = 0;
}

void _z_iosli_free(_z_iosli_t *ios)
{
    if (ios->is_alloc)
        free(ios->buf);
    ios = NULL;
}

/*------------------ RBuf ------------------*/
_z_rbuf_t _z_rbuf_make(size_t capacity)
{
    _z_rbuf_t rbf;
    rbf.ios = _z_iosli_make(capacity);
    return rbf;
}

_z_rbuf_t _z_rbuf_view(_z_rbuf_t *rbf, size_t length)
{
    assert(_z_iosli_readable(&rbf->ios) >= length);
    _z_rbuf_t v;
    v.ios = _z_iosli_wrap(_z_rbuf_get_rptr(rbf), length, 0, length);
    return v;
}

size_t _z_rbuf_capacity(const _z_rbuf_t *rbf)
{
    return rbf->ios.capacity;
}

size_t _z_rbuf_space_left(const _z_rbuf_t *rbf)
{
    return _z_iosli_writable(&rbf->ios);
}

size_t _z_rbuf_len(const _z_rbuf_t *rbf)
{
    return _z_iosli_readable(&rbf->ios);
}

uint8_t _z_rbuf_read(_z_rbuf_t *rbf)
{
    return _z_iosli_read(&rbf->ios);
}

void _z_rbuf_read_bytes(_z_rbuf_t *rbf, uint8_t *dest, size_t offset, size_t length)
{
    _z_iosli_read_bytes(&rbf->ios, dest, offset, length);
}

uint8_t _z_rbuf_get(const _z_rbuf_t *rbf, size_t pos)
{
    return _z_iosli_get(&rbf->ios, pos);
}

size_t _z_rbuf_get_rpos(const _z_rbuf_t *rbf)
{
    return rbf->ios.r_pos;
}

size_t _z_rbuf_get_wpos(const _z_rbuf_t *rbf)
{
    return rbf->ios.w_pos;
}

void _z_rbuf_set_rpos(_z_rbuf_t *rbf, size_t r_pos)
{
    assert(r_pos <= rbf->ios.w_pos);
    rbf->ios.r_pos = r_pos;
}

void _z_rbuf_set_wpos(_z_rbuf_t *rbf, size_t w_pos)
{
    assert(w_pos <= rbf->ios.capacity);
    rbf->ios.w_pos = w_pos;
}

uint8_t *_z_rbuf_get_rptr(const _z_rbuf_t *rbf)
{
    return rbf->ios.buf + rbf->ios.r_pos;
}

uint8_t *_z_rbuf_get_wptr(const _z_rbuf_t *rbf)
{
    return rbf->ios.buf + rbf->ios.w_pos;
}

void _z_rbuf_clear(_z_rbuf_t *rbf)
{
    _z_iosli_clear(&rbf->ios);
}

void _z_rbuf_compact(_z_rbuf_t *rbf)
{
    if (rbf->ios.r_pos == 0 && rbf->ios.w_pos == 0)
        return;

    size_t len = _z_iosli_readable(&rbf->ios);
    memcpy(rbf->ios.buf, _z_rbuf_get_rptr(rbf), len * sizeof(uint8_t));
    _z_rbuf_set_rpos(rbf, 0);
    _z_rbuf_set_wpos(rbf, len);
}

void _z_rbuf_free(_z_rbuf_t *rbf)
{
    _z_iosli_free(&rbf->ios);
    rbf = NULL;
}

/*------------------ WBuf ------------------*/
void _z_wbuf_add_iosli(_z_wbuf_t *wbf, _z_iosli_t *ios)
{
    size_t len = _z_vec_len(&wbf->ioss);
    if (len > 0)
    {
        // We are appending a new iosli, the last iosli becomes no longer writable
        _z_iosli_t *last = _z_wbuf_get_iosli(wbf, len - 1);
        last->capacity = last->w_pos;
        wbf->w_idx++;
    }

    _z_vec_append(&wbf->ioss, ios);
}

void _z_wbuf_add_iosli_from(_z_wbuf_t *wbf, const uint8_t *buf, size_t capacity)
{
    _z_iosli_t sios = _z_iosli_make(capacity);
    _z_iosli_t *pios = (_z_iosli_t *)malloc(sizeof(_z_iosli_t));
    memcpy(pios, &sios, sizeof(_z_iosli_t));
    memcpy(pios->buf, buf, capacity);
    pios->w_pos = pios->capacity;

    _z_wbuf_add_iosli(wbf, pios);
}

void _z_wbuf_new_iosli(_z_wbuf_t *wbf, size_t capacity)
{
    _z_iosli_t sios = _z_iosli_make(capacity);
    _z_iosli_t *pios = (_z_iosli_t *)malloc(sizeof(_z_iosli_t));
    memcpy(pios, &sios, sizeof(_z_iosli_t));

    _z_wbuf_add_iosli(wbf, pios);
}

_z_iosli_t *_z_wbuf_get_iosli(const _z_wbuf_t *wbf, size_t idx)
{
    return (_z_iosli_t *)_z_vec_get(&wbf->ioss, idx);
}

size_t _z_wbuf_len_iosli(const _z_wbuf_t *wbf)
{
    return _z_vec_len(&wbf->ioss);
}

_z_wbuf_t _z_wbuf_make(size_t capacity, int is_expandable)
{
    _z_wbuf_t wbf;
    wbf.r_idx = 0;
    wbf.w_idx = 0;
    if (is_expandable)
    {
        // Preallocate 4 slots, this is usually what we expect
        // when fragmenting a zenoh data message with attachment
        wbf.ioss = _z_vec_make(4);
    }
    else
    {
        wbf.ioss = _z_vec_make(1);
    }
    wbf.is_expandable = is_expandable;

    if (capacity > 0)
    {
        _z_wbuf_new_iosli(&wbf, capacity);
    }
    wbf.capacity = capacity;

    return wbf;
}

size_t _z_wbuf_capacity(const _z_wbuf_t *wbf)
{
    size_t cap = 0;
    for (size_t i = 0; i < _z_wbuf_len_iosli(wbf); i++)
    {
        _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, i);
        cap += ios->capacity;
    }
    return cap;
}

size_t _z_wbuf_len(const _z_wbuf_t *wbf)
{
    size_t len = 0;
    for (size_t i = wbf->r_idx; i <= wbf->w_idx; i++)
    {
        _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, i);
        len += _z_iosli_readable(ios);
    }
    return len;
}

size_t _z_wbuf_space_left(const _z_wbuf_t *wbf)
{
    _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, wbf->w_idx);
    return _z_iosli_writable(ios);
}

uint8_t __z_wbuf_read(_z_wbuf_t *wbf)
{
    do
    {
        assert(wbf->r_idx <= wbf->w_idx);
        _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, wbf->r_idx);
        size_t readable = _z_iosli_readable(ios);
        if (readable > 0)
        {
            return _z_iosli_read(ios);
        }
        else
        {
            wbf->r_idx++;
        }
    } while (1);
}

void __z_wbuf_read_bytes(_z_wbuf_t *wbf, uint8_t *dest, size_t offset, size_t length)
{
    do
    {
        assert(wbf->r_idx <= wbf->w_idx);
        _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, wbf->r_idx);
        size_t readable = _z_iosli_readable(ios);
        if (readable > 0)
        {
            size_t to_read = readable <= length ? readable : length;
            _z_iosli_read_bytes(ios, dest, offset, to_read);
            offset += to_read;
            length -= to_read;
        }
        else
        {
            wbf->r_idx++;
        }
    } while (length > 0);
}

uint8_t __z_wbuf_get(const _z_wbuf_t *wbf, size_t pos)
{
    size_t i = 0;
    _z_iosli_t *ios;
    do
    {
        assert(i < _z_vec_len(&wbf->ioss));
        ios = _z_wbuf_get_iosli(wbf, i);
        if (pos < ios->capacity)
            break;
        else
            pos -= ios->capacity;
    } while (++i);

    return _z_iosli_get(ios, pos);
}

int _z_wbuf_write(_z_wbuf_t *wbf, uint8_t b)
{
    _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, wbf->w_idx);
    size_t writable = _z_iosli_writable(ios);
    if (writable >= 1)
    {
        _z_iosli_write(ios, b);
        return 0;
    }
    else if (wbf->is_expandable)
    {
        _z_wbuf_new_iosli(wbf, wbf->capacity);
        ios = _z_wbuf_get_iosli(wbf, wbf->w_idx);
        _z_iosli_write(ios, b);
        return 0;
    }
    else
    {
        return -1;
    }
}

int _z_wbuf_write_bytes(_z_wbuf_t *wbf, const uint8_t *bs, size_t offset, size_t length)
{
    _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, wbf->w_idx);
    size_t writable = _z_iosli_writable(ios);
    if (writable >= length)
    {
        _z_iosli_write_bytes(ios, bs, offset, length);
        return 0;
    }
    else if (wbf->is_expandable)
    {
        // Write what left
        if (writable > 0)
        {
            _z_iosli_write_bytes(ios, bs, offset, writable);
            offset += writable;
            length -= writable;
        }

        // Allocate N capacity slots to hold at least length bytes
        size_t capacity = wbf->capacity * (1 + (length / wbf->capacity));
        _z_wbuf_new_iosli(wbf, capacity);
        ios = _z_wbuf_get_iosli(wbf, wbf->w_idx);
        _z_iosli_write_bytes(ios, bs, offset, length);
        return 0;
    }
    else
    {
        return -1;
    }
}

void _z_wbuf_put(_z_wbuf_t *wbf, uint8_t b, size_t pos)
{
    size_t i = 0;
    _z_iosli_t *ios;
    do
    {
        assert(i < _z_vec_len(&wbf->ioss));
        ios = _z_wbuf_get_iosli(wbf, i);
        if (pos < ios->capacity)
            break;
        else
            pos -= ios->capacity;
    } while (++i);

    _z_iosli_put(ios, b, pos);
    return;
}

size_t _z_wbuf_get_rpos(const _z_wbuf_t *wbf)
{
    size_t pos = 0;
    _z_iosli_t *ios;
    for (size_t i = 0; i < wbf->r_idx; i++)
    {
        ios = _z_wbuf_get_iosli(wbf, i);
        pos += ios->capacity;
    }
    ios = _z_wbuf_get_iosli(wbf, wbf->r_idx);
    pos += ios->r_pos;
    return pos;
}

size_t _z_wbuf_get_wpos(const _z_wbuf_t *wbf)
{
    size_t pos = 0;
    _z_iosli_t *ios;
    for (size_t i = 0; i < wbf->w_idx; i++)
    {
        ios = _z_wbuf_get_iosli(wbf, i);
        pos += ios->capacity;
    }
    ios = _z_wbuf_get_iosli(wbf, wbf->w_idx);
    pos += ios->w_pos;
    return pos;
}

void _z_wbuf_set_rpos(_z_wbuf_t *wbf, size_t pos)
{
    size_t i = 0;
    do
    {
        assert(i <= wbf->w_idx);
        _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, i);
        if (pos <= ios->w_pos)
        {
            wbf->r_idx = i;
            ios->r_pos = pos;
            return;
        }

        ios->r_pos = ios->w_pos;
        pos -= ios->capacity;
    } while (++i);
}

void _z_wbuf_set_wpos(_z_wbuf_t *wbf, size_t pos)
{
    size_t i = 0;
    do
    {
        assert(i <= _z_vec_len(&wbf->ioss));
        _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, i);
        if (pos <= ios->capacity && pos >= ios->r_pos)
        {
            wbf->w_idx = i;
            ios->w_pos = pos;
            return;
        }

        ios->w_pos = ios->capacity;
        pos -= ios->capacity;
    } while (++i);
}

_z_rbuf_t _z_wbuf_to_rbuf(const _z_wbuf_t *wbf)
{
    size_t len = _z_wbuf_len(wbf);
    _z_rbuf_t rbf = _z_rbuf_make(len);
    for (size_t i = wbf->r_idx; i <= wbf->w_idx; i++)
    {
        _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, i);
        _z_iosli_write_bytes(&rbf.ios, ios->buf, ios->r_pos, _z_iosli_readable(ios));
    }
    return rbf;
}

int _z_wbuf_copy_into(_z_wbuf_t *dst, _z_wbuf_t *src, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        int res = _z_wbuf_write(dst, __z_wbuf_read(src));
        if (res != 0)
            return -1;
    }
    return 0;
}

void _z_wbuf_clear(_z_wbuf_t *wbf)
{
    wbf->r_idx = 0;
    wbf->w_idx = 0;
    for (size_t i = 0; i < _z_wbuf_len_iosli(wbf); i++)
    {
        _z_iosli_clear(_z_wbuf_get_iosli(wbf, i));
    }
}

void _z_wbuf_reset(_z_wbuf_t *wbf)
{
    wbf->r_idx = 0;
    wbf->w_idx = 0;
    for (size_t i = 0; i < _z_wbuf_len_iosli(wbf); i++)
    {
        _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, i);
        _z_iosli_free(ios);
    }
    _z_vec_free(&wbf->ioss);

    if (wbf->is_expandable)
    {
        // Preallocate 4 slots, this is usually what we expect
        // when fragmenting a zenoh data message with attachment
        wbf->ioss = _z_vec_make(4);
    }
    else
    {
        wbf->ioss = _z_vec_make(1);
    }
}

void _z_wbuf_free(_z_wbuf_t *wbf)
{
    for (size_t i = 0; i < _z_wbuf_len_iosli(wbf); i++)
    {
        _z_iosli_t *ios = _z_wbuf_get_iosli(wbf, i);
        _z_iosli_free(ios);
    }
    _z_vec_free(&wbf->ioss);
    wbf = NULL;
}
