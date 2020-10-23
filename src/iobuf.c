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

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "zenoh/iobuf.h"

#include <stdio.h>

/*------------------ IOSli ------------------*/
z_iosli_t z_iosli_wrap(uint8_t *buf, size_t capacity, size_t r_pos, size_t w_pos)
{
    assert(r_pos <= w_pos && w_pos <= capacity);
    z_iosli_t ios;
    ios.r_pos = r_pos;
    ios.w_pos = w_pos;
    ios.capacity = capacity;
    ios.buf = buf;
    return ios;
}

z_iosli_t z_iosli_make(size_t capacity)
{
    return z_iosli_wrap((uint8_t *)malloc(capacity), capacity, 0, 0);
}

void z_iosli_resize(z_iosli_t *ios, size_t capacity)
{
    assert(ios->w_pos <= capacity);
    ios->capacity = capacity;
    ios->buf = (uint8_t *)realloc(ios->buf, ios->capacity);
}

size_t z_iosli_readable(const z_iosli_t *ios)
{
    return ios->w_pos - ios->r_pos;
}

uint8_t z_iosli_read(z_iosli_t *ios)
{
    assert(ios->r_pos < ios->w_pos);
    return ios->buf[ios->r_pos++];
}

void z_iosli_read_bytes(z_iosli_t *ios, uint8_t *dst, size_t offset, size_t length)
{
    assert(z_iosli_readable(ios) >= length);
    memcpy(dst + offset, ios->buf + ios->r_pos, length);
    ios->r_pos += length;
}

uint8_t z_iosli_get(const z_iosli_t *ios, size_t pos)
{
    assert(pos < ios->capacity);
    return ios->buf[pos];
}

size_t z_iosli_writable(const z_iosli_t *ios)
{
    return ios->capacity - ios->w_pos;
}

void z_iosli_write(z_iosli_t *ios, uint8_t b)
{
    assert(z_iosli_writable(ios) >= 1);
    ios->buf[ios->w_pos++] = b;
}

void z_iosli_write_bytes(z_iosli_t *ios, const uint8_t *bs, size_t offset, size_t length)
{
    assert(z_iosli_writable(ios) >= length);
    memcpy(ios->buf + ios->w_pos, bs + offset, length);
    ios->w_pos += length;
}

void z_iosli_put(z_iosli_t *ios, uint8_t b, size_t pos)
{
    assert(pos < ios->capacity);
    ios->buf[pos] = b;
}

z_uint8_array_t z_iosli_to_array(const z_iosli_t *ios)
{
    z_uint8_array_t a;
    a.length = z_iosli_readable(ios);
    a.elem = ios->buf + ios->r_pos;
    return a;
}

void z_iosli_clear(z_iosli_t *ios)
{
    ios->r_pos = 0;
    ios->w_pos = 0;
}

void z_iosli_free(z_iosli_t *ios)
{
    free(ios->buf);
    ios = NULL;
}

/*------------------ RBuf ------------------*/
z_rbuf_t z_rbuf_make(size_t capacity)
{
    z_rbuf_t rbf;
    rbf.ios = z_iosli_make(capacity);
    return rbf;
}

z_rbuf_t z_rbuf_view(z_rbuf_t *rbf, size_t length)
{
    assert(z_iosli_readable(&rbf->ios) >= length);
    z_rbuf_t v;
    v.ios = z_iosli_wrap(z_rbuf_get_rptr(rbf), length, 0, length);
    return v;
}

size_t z_rbuf_capacity(const z_rbuf_t *rbf)
{
    return rbf->ios.capacity;
}

size_t z_rbuf_space_left(const z_rbuf_t *rbf)
{
    return z_iosli_writable(&rbf->ios);
}

size_t z_rbuf_len(const z_rbuf_t *rbf)
{
    return z_iosli_readable(&rbf->ios);
}

uint8_t z_rbuf_read(z_rbuf_t *rbf)
{
    return z_iosli_read(&rbf->ios);
}

void z_rbuf_read_bytes(z_rbuf_t *rbf, uint8_t *dest, size_t offset, size_t length)
{
    z_iosli_read_bytes(&rbf->ios, dest, offset, length);
}

uint8_t z_rbuf_get(const z_rbuf_t *rbf, size_t pos)
{
    return z_iosli_get(&rbf->ios, pos);
}

size_t z_rbuf_get_rpos(const z_rbuf_t *rbf)
{
    return rbf->ios.r_pos;
}

size_t z_rbuf_get_wpos(const z_rbuf_t *rbf)
{
    return rbf->ios.w_pos;
}

void z_rbuf_set_rpos(z_rbuf_t *rbf, size_t r_pos)
{
    assert(r_pos <= rbf->ios.w_pos);
    rbf->ios.r_pos = r_pos;
}

void z_rbuf_set_wpos(z_rbuf_t *rbf, size_t w_pos)
{
    assert(w_pos <= rbf->ios.capacity);
    rbf->ios.w_pos = w_pos;
}

uint8_t *z_rbuf_get_rptr(const z_rbuf_t *rbf)
{
    return rbf->ios.buf + rbf->ios.r_pos;
}

uint8_t *z_rbuf_get_wptr(const z_rbuf_t *rbf)
{
    return rbf->ios.buf + rbf->ios.w_pos;
}

void z_rbuf_clear(z_rbuf_t *rbf)
{
    z_iosli_clear(&rbf->ios);
}

void z_rbuf_compact(z_rbuf_t *rbf)
{
    if (rbf->ios.r_pos == 0 && rbf->ios.w_pos == 0)
        return;

    size_t len = z_iosli_readable(&rbf->ios);
    memcpy(rbf->ios.buf, z_rbuf_get_rptr(rbf), len * sizeof(uint8_t));
    z_rbuf_set_rpos(rbf, 0);
    z_rbuf_set_wpos(rbf, len);
}

void z_rbuf_free(z_rbuf_t *rbf)
{
    z_iosli_free(&rbf->ios);
    rbf = NULL;
}

/*------------------ WBuf ------------------*/
void z_wbuf_add_iosli(z_wbuf_t *wbf, z_iosli_t *ios)
{
    size_t len = z_vec_len(&wbf->ioss);
    if (len > 0)
    {
        // We are appending a new iosli, the last iosli becomes no longer writable
        z_iosli_t *last = z_wbuf_get_iosli(wbf, len - 1);
        last->capacity = last->w_pos;
        wbf->w_idx++;
    }

    z_vec_append(&wbf->ioss, ios);
}

void z_wbuf_new_iosli(z_wbuf_t *wbf, size_t capacity)
{
    z_iosli_t *ios = (z_iosli_t *)malloc(sizeof(z_iosli_t));
    ios->r_pos = 0;
    ios->w_pos = 0;
    ios->capacity = capacity;
    ios->buf = (uint8_t *)malloc(capacity);

    z_wbuf_add_iosli(wbf, ios);
}

z_iosli_t *z_wbuf_get_iosli(const z_wbuf_t *wbf, size_t idx)
{
    return (z_iosli_t *)z_vec_get(&wbf->ioss, idx);
}

size_t z_wbuf_len_iosli(const z_wbuf_t *wbf)
{
    return z_vec_len(&wbf->ioss);
}

z_wbuf_t z_wbuf_make(size_t capacity, int is_expandable)
{
    z_wbuf_t wbf;
    wbf.r_idx = 0;
    wbf.w_idx = 0;
    wbf.capacity = capacity;
    if (is_expandable)
    {
        // Preallocate 4 slots, this is usually what we expect
        // when fragmenting a zenoh data message with attachment
        wbf.ioss = z_vec_make(4);
    }
    else
    {
        wbf.ioss = z_vec_make(1);
    }
    z_wbuf_new_iosli(&wbf, capacity);
    wbf.is_expandable = is_expandable;

    return wbf;
}

size_t z_wbuf_capacity(const z_wbuf_t *wbf)
{
    size_t cap = 0;
    for (size_t i = 0; i < z_wbuf_len_iosli(wbf); i++)
    {
        z_iosli_t *ios = z_wbuf_get_iosli(wbf, i);
        cap += ios->capacity;
    }
    return cap;
}

size_t z_wbuf_len(const z_wbuf_t *wbf)
{
    size_t len = 0;
    for (size_t i = wbf->r_idx; i <= wbf->w_idx; i++)
    {
        z_iosli_t *ios = z_wbuf_get_iosli(wbf, i);
        len += z_iosli_readable(ios);
    }
    return len;
}

size_t z_wbuf_space_left(const z_wbuf_t *wbf)
{
    z_iosli_t *ios = z_wbuf_get_iosli(wbf, wbf->w_idx);
    return z_iosli_writable(ios);
}

uint8_t _z_wbuf_read(z_wbuf_t *wbf)
{
    do
    {
        assert(wbf->r_idx <= wbf->w_idx);
        z_iosli_t *ios = z_wbuf_get_iosli(wbf, wbf->r_idx);
        size_t readable = z_iosli_readable(ios);
        if (readable > 0)
        {
            return z_iosli_read(ios);
        }
        else
        {
            wbf->r_idx++;
        }
    } while (1);
}

void _z_wbuf_read_bytes(z_wbuf_t *wbf, uint8_t *dest, size_t offset, size_t length)
{
    do
    {
        assert(wbf->r_idx <= wbf->w_idx);
        z_iosli_t *ios = z_wbuf_get_iosli(wbf, wbf->r_idx);
        size_t readable = z_iosli_readable(ios);
        if (readable > 0)
        {
            size_t to_read = readable <= length ? readable : length;
            z_iosli_read_bytes(ios, dest, offset, to_read);
            offset += to_read;
            length -= to_read;
        }
        else
        {
            wbf->r_idx++;
        }
    } while (length > 0);
}

uint8_t _z_wbuf_get(const z_wbuf_t *wbf, size_t pos)
{
    size_t i = 0;
    z_iosli_t *ios;
    do
    {
        assert(i < z_vec_len(&wbf->ioss));
        ios = z_wbuf_get_iosli(wbf, i);
        if (pos < ios->capacity)
            break;
        else
            pos -= ios->capacity;
    } while (++i);

    return z_iosli_get(ios, pos);
}

int z_wbuf_write(z_wbuf_t *wbf, uint8_t b)
{
    z_iosli_t *ios = z_wbuf_get_iosli(wbf, wbf->w_idx);
    size_t writable = z_iosli_writable(ios);
    if (writable >= 1)
    {
        z_iosli_write(ios, b);
        return 0;
    }
    else if (wbf->is_expandable)
    {
        z_wbuf_new_iosli(wbf, wbf->capacity);
        ios = z_wbuf_get_iosli(wbf, wbf->w_idx);
        z_iosli_write(ios, b);
        return 0;
    }
    else
    {
        return -1;
    }
}

int z_wbuf_write_bytes(z_wbuf_t *wbf, const uint8_t *bs, size_t offset, size_t length)
{
    z_iosli_t *ios = z_wbuf_get_iosli(wbf, wbf->w_idx);
    size_t writable = z_iosli_writable(ios);
    if (writable >= length)
    {
        z_iosli_write_bytes(ios, bs, offset, length);
        return 0;
    }
    else if (wbf->is_expandable)
    {
        // Write what left
        if (writable > 0)
        {
            z_iosli_write_bytes(ios, bs, offset, writable);
            offset += writable;
            length -= writable;
        }

        // Allocate N capacity slots to hold at least length bytes
        size_t capacity = wbf->capacity * (1 + (length / wbf->capacity));
        z_wbuf_new_iosli(wbf, capacity);
        ios = z_wbuf_get_iosli(wbf, wbf->w_idx);
        z_iosli_write_bytes(ios, bs, offset, length);
        return 0;
    }
    else
    {
        return -1;
    }
}

void z_wbuf_put(z_wbuf_t *wbf, uint8_t b, size_t pos)
{
    size_t i = 0;
    z_iosli_t *ios;
    do
    {
        assert(i < z_vec_len(&wbf->ioss));
        ios = z_wbuf_get_iosli(wbf, i);
        if (pos < ios->capacity)
            break;
        else
            pos -= ios->capacity;
    } while (++i);

    z_iosli_put(ios, b, pos);
    return;
}

size_t z_wbuf_get_rpos(const z_wbuf_t *wbf)
{
    size_t pos = 0;
    z_iosli_t *ios;
    for (size_t i = 0; i < wbf->r_idx; i++)
    {
        ios = z_wbuf_get_iosli(wbf, i);
        pos += ios->capacity;
    }
    ios = z_wbuf_get_iosli(wbf, wbf->r_idx);
    pos += ios->r_pos;
    return pos;
}

size_t z_wbuf_get_wpos(const z_wbuf_t *wbf)
{
    size_t pos = 0;
    z_iosli_t *ios;
    for (size_t i = 0; i < wbf->w_idx; i++)
    {
        ios = z_wbuf_get_iosli(wbf, i);
        pos += ios->capacity;
    }
    ios = z_wbuf_get_iosli(wbf, wbf->w_idx);
    pos += ios->w_pos;
    return pos;
}

void z_wbuf_set_rpos(z_wbuf_t *wbf, size_t pos)
{
    size_t i = 0;
    do
    {
        assert(i <= wbf->w_idx);
        z_iosli_t *ios = z_wbuf_get_iosli(wbf, i);
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

void z_wbuf_set_wpos(z_wbuf_t *wbf, size_t pos)
{
    size_t i = 0;
    do
    {
        assert(i <= z_vec_len(&wbf->ioss));
        z_iosli_t *ios = z_wbuf_get_iosli(wbf, i);
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

z_rbuf_t z_wbuf_to_rbuf(const z_wbuf_t *wbf)
{
    size_t len = z_wbuf_len(wbf);
    z_rbuf_t rbf = z_rbuf_make(len);
    for (size_t i = 0; i < z_vec_len(&wbf->ioss); i++)
    {
        z_iosli_t *ios = z_wbuf_get_iosli(wbf, i);
        z_iosli_write_bytes(&rbf.ios, ios->buf, ios->r_pos, z_iosli_readable(ios));
    }
    return rbf;
}

int z_wbuf_copy_into(z_wbuf_t *dst, z_wbuf_t *src, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        int res = z_wbuf_write(dst, _z_wbuf_read(src));
        if (res != 0)
            return -1;
    }
    return 0;
}

void z_wbuf_clear(z_wbuf_t *wbf)
{
    wbf->r_idx = 0;
    wbf->w_idx = 0;
    for (size_t i = 0; i < z_vec_len(&wbf->ioss); i++)
    {
        z_iosli_clear(z_wbuf_get_iosli(wbf, i));
    }
}

void z_wbuf_free(z_wbuf_t *wbf)
{
    for (size_t i = 0; i < z_vec_len(&wbf->ioss); i++)
    {
        z_iosli_t *ios = z_wbuf_get_iosli(wbf, i);
        z_iosli_free(ios);
    }
    z_vec_free(&wbf->ioss);
    wbf = NULL;
}
