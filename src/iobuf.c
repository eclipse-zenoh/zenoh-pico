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

#include <string.h>
#include <assert.h>
#include "zenoh/iobuf.h"

/*------------------ IOSli ------------------*/
_z_iosli_t _z_iosli_wrap_wo(uint8_t *buf, size_t capacity, size_t rpos, size_t wpos)
{
    assert(rpos < capacity && wpos < capacity);
    _z_iosli_t iob;
    iob.r_pos = rpos;
    iob.w_pos = wpos;
    iob.capacity = capacity;
    iob.buf = buf;
    return iob;
}

_z_iosli_t _z_iosli_wrap(uint8_t *buf, size_t capacity)
{
    return _z_iosli_wrap_wo(buf, capacity, 0, 0);
}

_z_iosli_t _z_iosli_make(size_t capacity)
{
    return _z_iosli_wrap((uint8_t *)malloc(capacity), capacity);
}

int _z_iosli_resize(_z_iosli_t *iob, size_t capacity)
{
    assert(iob->w_pos < capacity && iob->r_pos < capacity);

    iob->buf = (uint8_t *)realloc(iob->buf, capacity);
    if (iob->buf)
    {
        iob->capacity = capacity;
        return 0;
    }
    else
    {
        return -1;
    }
}

void _z_iosli_free(_z_iosli_t *iob)
{
    iob->r_pos = 0;
    iob->w_pos = 0;
    iob->capacity = 0;
    free(iob->buf);
    iob = NULL;
}

size_t _z_iosli_readable(const _z_iosli_t *iob)
{
    return iob->w_pos - iob->r_pos;
}

size_t _z_iosli_writable(const _z_iosli_t *iob)
{
    return iob->capacity - iob->w_pos;
}

int _z_iosli_write(_z_iosli_t *iob, uint8_t b)
{
    if (iob->w_pos >= iob->capacity)
        return -1;

    iob->buf[iob->w_pos++] = b;
    return 0;
}

int _z_iosli_write_slice(_z_iosli_t *iob, const uint8_t *bs, size_t offset, size_t length)
{
    if (_z_iosli_writable(iob) < length)
        return -1;

    memcpy(iob->buf + iob->w_pos, bs + offset, length);
    iob->w_pos += length;
    return 0;
}

int _z_iosli_write_bytes(_z_iosli_t *iob, const uint8_t *bs, size_t length)
{
    return _z_iosli_write_slice(iob, bs, 0, length);
}

uint8_t _z_iosli_read(_z_iosli_t *iob)
{
    assert(iob->r_pos < iob->w_pos);
    return iob->buf[iob->r_pos++];
}

uint8_t *_z_iosli_read_to_n(_z_iosli_t *iob, uint8_t *dst, size_t length)
{
    assert(_z_iosli_readable(iob) >= length);
    memcpy(dst, iob->buf + iob->r_pos, length);
    iob->r_pos += length;
    return dst;
}

uint8_t *_z_iosli_read_n(_z_iosli_t *iob, size_t length)
{
    uint8_t *dst = (uint8_t *)malloc(length);
    return _z_iosli_read_to_n(iob, dst, length);
}

void _z_iosli_put(_z_iosli_t *iob, uint8_t b, size_t pos)
{
    assert(pos < iob->capacity);
    iob->buf[pos] = b;
}

uint8_t _z_iosli_get(_z_iosli_t *iob, size_t pos)
{
    assert(pos < iob->capacity);
    return iob->buf[pos];
}

void _z_iosli_clear(_z_iosli_t *iob)
{
    iob->r_pos = 0;
    iob->w_pos = 0;
}

void _z_iosli_compact(_z_iosli_t *iob)
{
    if (iob->r_pos == 0 && iob->w_pos == 0)
    {
        return;
    }
    size_t len = iob->w_pos - iob->r_pos;
    uint8_t *cp = iob->buf + iob->r_pos;
    memcpy(iob->buf, cp, len);
    iob->r_pos = 0;
    iob->w_pos = len;
}

/*------------------ IOBuf ------------------*/
int z_iobuf_add_iosli(z_iobuf_t *iob, _z_iosli_t *ios)
{
    if (z_iobuf_is_expandable(iob) && !z_iobuf_is_contigous(iob))
    {
        z_vec_append(&iob->ioss, ios);
        return 0;
    }
    else
    {
        return -1;
    }
}

_z_iosli_t *z_iobuf_get_iosli(z_iobuf_t *iob, unsigned int idx)
{
    return (_z_iosli_t *)z_vec_get(&iob->ioss, idx);
}

z_iobuf_t z_iobuf_make(size_t chunk, unsigned int mode)
{
    z_iobuf_t iob;
    iob.mode = mode;
    iob.r_idx = 0;
    iob.w_idx = 0;
    iob.chunk = chunk;

    iob.ioss = z_vec_make(1);
    _z_iosli_t ios = _z_iosli_make(chunk);
    z_vec_append(&iob.ioss, &ios);

    return iob;
}

int z_iobuf_is_contigous(const z_iobuf_t *iob)
{
    return iob->mode & Z_IOBUF_MODE_CONTIGOUS;
}

int z_iobuf_is_expandable(const z_iobuf_t *iob)
{
    return iob->mode & Z_IOBUF_MODE_EXPANDABLE;
}

size_t z_iobuf_writable(const z_iobuf_t *iob)
{
    size_t len = z_vec_len(&iob->ioss) - iob->w_idx;
    size_t writable = 0;
    for (size_t i = iob->w_idx; i < len; i++)
    {
        writable += _z_iosli_writable((_z_iosli_t *)z_vec_get(&iob->ioss, i));
    }

    return writable;
}

int z_iobuf_write(z_iobuf_t *iob, uint8_t b)
{
    do
    {
        _z_iosli_t *ios = (_z_iosli_t *)z_vec_get(&iob->ioss, iob->w_idx);
        int res = _z_iosli_write(ios, b);
        if (res == 0)
        {
            return 0;
        }
        else if (iob->w_idx + 1 < z_vec_len(&iob->ioss))
        {
            iob->w_idx++;
        }
        else if (z_iobuf_is_expandable(iob))
        {
            if (z_iobuf_is_contigous(iob))
            {
                res = _z_iosli_resize(ios, ios->capacity + iob->chunk);
                if (res != 0)
                    return -1;
            }
            else
            {
                // Create a new iobuf
                _z_iosli_t niob = _z_iosli_make(iob->chunk);
                // Add the new iobuf to the current buffer
                z_iobuf_add_iosli(iob, &niob);

                iob->w_idx++;
            }
        }
        else
        {
            return -1;
        }

    } while (1);
}

int z_iobuf_write_slice(z_iobuf_t *iob, const uint8_t *bs, size_t offset, size_t length)
{
    do
    {
        _z_iosli_t *ios = (_z_iosli_t *)z_vec_get(&iob->ioss, iob->w_idx);
        int res = _z_iosli_write_slice(ios, bs, offset, length);

        if (res == 0)
        {
            return 0;
        }
        else if (iob->w_idx + 1 < z_vec_len(&iob->ioss))
        {
            iob->w_idx++;
        }
        else if (z_iobuf_is_expandable(iob))
        {
            if (z_iobuf_is_contigous(iob))
            {
                // Compute the new target size
                size_t min_cap = ios->capacity + length - _z_iosli_writable(ios);
                size_t new_cap = iob->chunk;
                do
                {
                    new_cap += iob->chunk;
                } while (new_cap < min_cap);

                // Resize
                res = _z_iosli_resize(ios, new_cap);
                if (res != 0)
                    return -1;
            }
            else
            {
                // Write what remains here
                size_t writable = _z_iosli_writable(ios);
                int res = _z_iosli_write_slice(ios, bs, offset, writable);
                if (res != 0)
                    return -1;

                // Update the offset
                offset += writable;
                length -= writable;
                // Create a new iobuf
                _z_iosli_t niob = _z_iosli_make(iob->chunk);
                // Add the new iobuf to the current buffer
                z_iobuf_add_iosli(iob, &niob);

                iob->w_idx++;
            }
        }
        else
        {
            return -1;
        }
    } while (1);
}

int z_iobuf_write_bytes(z_iobuf_t *iob, const uint8_t *bs, size_t length)
{
    return z_iobuf_write_slice(iob, bs, 0, length);
}

void z_iobuf_put(z_iobuf_t *iob, uint8_t b, size_t pos)
{
    size_t i = 0;
    do
    {
        _z_iosli_t *ios = (_z_iosli_t *)z_vec_get(&iob->ioss, i);
        if (pos < ios->capacity)
        {
            _z_iosli_put(ios, pos, b);
            return;
        }

        i++;
        pos -= ios->capacity;
    } while (1);
}

size_t z_iobuf_readable(const z_iobuf_t *iob)
{
    size_t len = z_vec_len(&iob->ioss) - iob->r_idx;
    size_t readable = 0;
    for (size_t i = iob->r_idx; i < len; i++)
    {
        readable += _z_iosli_readable((_z_iosli_t *)z_vec_get(&iob->ioss, i));
    }

    return readable;
}

uint8_t z_iobuf_read(z_iobuf_t *iob)
{
    _z_iosli_t *iosli;
    do
    {
        iosli = (_z_iosli_t *)z_vec_get(&iob->ioss, iob->r_idx);
        if (_z_iosli_readable(iosli) > 0)
        {
            return _z_iosli_read(iosli);
        }
        else
        {
            iob->r_idx++;
        }
    } while (1);
}

uint8_t *z_iobuf_read_to_n(z_iobuf_t *iob, uint8_t *dst, size_t length)
{
    uint8_t *w_pos = dst;
    do
    {
        _z_iosli_t *ios = (_z_iosli_t *)z_vec_get(&iob->ioss, iob->r_idx);
        size_t readbale = _z_iosli_readable(ios);
        if (readbale > 0)
        {
            size_t to_read = readbale < length ? readbale : length;
            _z_iosli_read_to_n(ios, w_pos, to_read);
            length -= to_read;
            w_pos += to_read;
        }
        else
        {
            iob->r_idx++;
        }
    } while (length > 0);

    return dst;
}

uint8_t *z_iobuf_read_n(z_iobuf_t *iob, size_t length)
{
    uint8_t *dst = (uint8_t *)malloc(length);
    return z_iobuf_read_to_n(iob, dst, length);
}

uint8_t z_iobuf_get(z_iobuf_t *iob, size_t pos)
{
    size_t i = 0;
    do
    {
        _z_iosli_t *ios = (_z_iosli_t *)z_vec_get(&iob->ioss, i);
        if (pos < ios->capacity)
        {
            return _z_iosli_get(ios, pos);
        }

        i++;
        pos -= ios->capacity;
    } while (1);
}

void z_iobuf_clear(z_iobuf_t *iob)
{
    iob->r_idx = 0;
    iob->w_idx = 0;
    for (size_t i = 0; i < z_vec_len(&iob->ioss); i++)
        _z_iosli_clear((_z_iosli_t *)z_vec_get(&iob->ioss, i));
}

void z_iobuf_compact(z_iobuf_t *iob)
{
    assert(!z_iobuf_is_expandable(iob) || z_iobuf_is_contigous(iob));

    for (size_t i = 0; i < z_vec_len(&iob->ioss); i++)
        _z_iosli_compact((_z_iosli_t *)z_vec_get(&iob->ioss, i));
}

void z_iobuf_free(z_iobuf_t *iob)
{
    for (size_t i = 0; i < z_vec_len(&iob->ioss); i++)
        _z_iosli_free((_z_iosli_t *)z_vec_get(&iob->ioss, i));
    z_vec_free(&iob->ioss);
}
