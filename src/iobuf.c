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

z_iobuf_t z_iobuf_wrap_wo(uint8_t *buf, size_t capacity, size_t rpos, size_t wpos)
{
    assert(rpos <= capacity && wpos <= capacity);
    z_iobuf_t iobuf;
    iobuf.r_pos = rpos;
    iobuf.w_pos = wpos;
    iobuf.capacity = capacity;
    iobuf.buf = buf;
    return iobuf;
}

z_iobuf_t z_iobuf_wrap(uint8_t *buf, size_t capacity)
{
    return z_iobuf_wrap_wo(buf, capacity, 0, 0);
}

z_iobuf_t z_iobuf_make(size_t capacity)
{
    return z_iobuf_wrap((uint8_t *)malloc(capacity), capacity);
}

void z_iobuf_free(z_iobuf_t *buf)
{
    buf->r_pos = 0;
    buf->w_pos = 0;
    buf->capacity = 0;
    free(buf->buf);
    buf = 0;
}

size_t z_iobuf_readable(const z_iobuf_t *iob)
{
    return iob->w_pos - iob->r_pos;
}
size_t z_iobuf_writable(const z_iobuf_t *iob)
{
    return iob->capacity - iob->w_pos;
}

int z_iobuf_write(z_iobuf_t *iob, uint8_t b)
{
    if (iob->w_pos >= iob->capacity)
        return -1;

    iob->buf[iob->w_pos++] = b;
    return 0;
}

int z_iobuf_write_slice(z_iobuf_t *iob, const uint8_t *bs, size_t offset, size_t length)
{
    if (z_iobuf_writable(iob) < length)
        return -1;

    memcpy(iob->buf + iob->w_pos, bs + offset, length);
    iob->w_pos += length;
    return 0;
}

int z_iobuf_write_bytes(z_iobuf_t *iob, const uint8_t *bs, size_t length)
{
    if (z_iobuf_writable(iob) < length)
        return -1;

    memcpy(iob->buf + iob->w_pos, bs, length);
    iob->w_pos += length;
    return 0;
}

uint8_t z_iobuf_read(z_iobuf_t *iob)
{
    assert(iob->r_pos < iob->w_pos);
    return iob->buf[iob->r_pos++];
}

uint8_t *z_iobuf_read_to_n(z_iobuf_t *iob, uint8_t *dst, size_t length)
{
    assert(z_iobuf_readable(iob) >= length);
    memcpy(dst, iob->buf + iob->r_pos, length);
    iob->r_pos += length;
    return dst;
}

uint8_t *z_iobuf_read_n(z_iobuf_t *iob, size_t length)
{
    uint8_t *dst = (uint8_t *)malloc(length);
    return z_iobuf_read_to_n(iob, dst, length);
}

void z_iobuf_put(z_iobuf_t *iob, uint8_t b, size_t pos)
{
    assert(pos < iob->capacity);
    iob->buf[pos] = b;
}

uint8_t z_iobuf_get(z_iobuf_t *iob, size_t pos)
{
    assert(pos < iob->capacity);
    return iob->buf[pos];
}

void z_iobuf_clear(z_iobuf_t *buf)
{
    buf->r_pos = 0;
    buf->w_pos = 0;
}

z_uint8_array_t z_iobuf_to_array(z_iobuf_t *buf)
{
    z_uint8_array_t a = {z_iobuf_readable(buf), &buf->buf[buf->r_pos]};
    return a;
}

void z_iobuf_compact(z_iobuf_t *buf)
{
    if (buf->r_pos == 0 && buf->w_pos == 0)
    {
        return;
    }
    size_t len = buf->w_pos - buf->r_pos;
    uint8_t *cp = buf->buf + buf->r_pos;
    memcpy(buf->buf, cp, len);
    buf->r_pos = 0;
    buf->w_pos = len;
}
