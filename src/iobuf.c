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
z_iosli_t z_iosli_wrap_wo(uint8_t *buf, size_t capacity, size_t r_pos, size_t w_pos)
{
    assert(r_pos <= w_pos && w_pos < capacity);
    z_iosli_t ios;
    ios.r_pos = r_pos;
    ios.w_pos = w_pos;
    ios.capacity = capacity;
    ios.buf = buf;
    return ios;
}

z_iosli_t z_iosli_wrap(uint8_t *buf, size_t capacity)
{
    return z_iosli_wrap_wo(buf, capacity, 0, 0);
}

z_iosli_t z_iosli_make(size_t capacity)
{
    return z_iosli_wrap((uint8_t *)malloc(capacity), capacity);
}

int z_iosli_resize(z_iosli_t *ios, size_t capacity)
{
    assert(ios->w_pos < capacity && ios->r_pos < capacity);

    ios->buf = (uint8_t *)realloc(ios->buf, capacity);
    if (ios->buf)
    {
        ios->capacity = capacity;
        return 0;
    }
    else
    {
        return -1;
    }
}

void z_iosli_free(z_iosli_t *ios)
{
    free(ios->buf);
    ios = NULL;
}

size_t z_iosli_readable(const z_iosli_t *ios)
{
    return ios->w_pos - ios->r_pos;
}

size_t z_iosli_writable(const z_iosli_t *ios)
{
    return ios->capacity - ios->w_pos;
}

int z_iosli_write(z_iosli_t *ios, uint8_t b)
{
    if (ios->w_pos >= ios->capacity)
        return -1;

    ios->buf[ios->w_pos++] = b;
    return 0;
}

int z_iosli_write_slice(z_iosli_t *ios, const uint8_t *bs, size_t offset, size_t length)
{
    if (z_iosli_writable(ios) < length)
        return -1;

    memcpy(ios->buf + ios->w_pos, bs + offset, length);
    ios->w_pos += length;
    return 0;
}

int z_iosli_write_bytes(z_iosli_t *ios, const uint8_t *bs, size_t length)
{
    return z_iosli_write_slice(ios, bs, 0, length);
}

uint8_t z_iosli_read(z_iosli_t *ios)
{
    assert(ios->r_pos < ios->w_pos);
    return ios->buf[ios->r_pos++];
}

void z_iosli_read_to_n(z_iosli_t *ios, uint8_t *dst, size_t length)
{
    assert(z_iosli_readable(ios) >= length);
    memcpy(dst, ios->buf + ios->r_pos, length);
    ios->r_pos += length;
}

uint8_t *z_iosli_read_n(z_iosli_t *ios, size_t length)
{
    uint8_t *dst = (uint8_t *)malloc(length);
    z_iosli_read_to_n(ios, dst, length);
    return dst;
}

void z_iosli_put(z_iosli_t *ios, uint8_t b, size_t pos)
{
    assert(pos < ios->capacity);
    ios->buf[pos] = b;
}

uint8_t z_iosli_get(const z_iosli_t *ios, size_t pos)
{
    assert(pos < ios->capacity);
    return ios->buf[pos];
}

void z_iosli_clear(z_iosli_t *ios)
{
    ios->r_pos = 0;
    ios->w_pos = 0;
}

void z_iosli_compact(z_iosli_t *ios)
{
    if (ios->r_pos == 0 && ios->w_pos == 0)
    {
        return;
    }
    size_t len = ios->w_pos - ios->r_pos;
    uint8_t *cp = ios->buf + ios->r_pos;
    memcpy(ios->buf, cp, len);
    ios->r_pos = 0;
    ios->w_pos = len;
}

/*------------------ IOBuf ------------------*/
int z_iobuf_is_contigous(const z_iobuf_t *iob)
{
    return iob->mode & Z_IOBUF_MODE_CONTIGOUS;
}

int z_iobuf_is_expandable(const z_iobuf_t *iob)
{
    return iob->mode & Z_IOBUF_MODE_EXPANDABLE;
}

void z_iobuf_add_iosli(z_iobuf_t *iob, const z_iosli_t *ios)
{
    assert(!z_iobuf_is_contigous(iob) && z_iobuf_is_expandable(iob));
    z_vec_append(&iob->value.eios.ioss, (z_iosli_t *)ios);
}

z_iosli_t *z_iobuf_get_iosli(const z_iobuf_t *iob, unsigned int idx)
{
    assert(!z_iobuf_is_contigous(iob));
    return (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, idx);
}

size_t z_iobuf_len_iosli(const z_iobuf_t *iob)
{
    assert(!z_iobuf_is_contigous(iob));
    return z_vec_len(&iob->value.eios.ioss);
}

z_iobuf_t z_iobuf_uninit(unsigned int mode)
{
    z_iobuf_t iob;
    iob.mode = mode;

    if (z_iobuf_is_contigous(&iob))
    {
        iob.value.cios.capacity = 0;
        iob.value.cios.r_pos = 0;
        iob.value.cios.w_pos = 0;
        iob.value.cios.buf = NULL;
    }
    else
    {
        iob.value.eios.r_idx = 0;
        iob.value.eios.w_idx = 0;
        iob.value.eios.chunk = 0;
        iob.value.eios.ioss = z_vec_uninit();
    }

    return iob;
}

z_iobuf_t z_iobuf_make(size_t chunk, unsigned int mode)
{
    z_iobuf_t iob = z_iobuf_uninit(mode);
    z_iosli_t ios = z_iosli_make(chunk);
    if (z_iobuf_is_contigous(&iob))
    {
        iob.value.cios = ios;
    }
    else
    {
        iob.value.eios.chunk = chunk;
        iob.value.eios.ioss = z_vec_make(1);
        z_iobuf_add_iosli(&iob, &ios);
    }

    return iob;
}

z_iobuf_t z_iobuf_wrap_wo(uint8_t *buf, size_t capacity, size_t r_pos, size_t w_pos)
{
    z_iobuf_t iob = z_iobuf_uninit(Z_IOBUF_MODE_CONTIGOUS);
    iob.value.cios = z_iosli_wrap_wo(buf, capacity, r_pos, w_pos);

    return iob;
}

z_iobuf_t z_iobuf_wrap(uint8_t *buf, size_t capacity)
{
    return z_iobuf_wrap_wo(buf, capacity, 0, 0);
}

size_t z_iobuf_writable(const z_iobuf_t *iob)
{
    if (z_iobuf_is_contigous(iob))
    {
        return z_iosli_writable(&iob->value.cios);
    }
    else
    {
        size_t num = z_vec_len(&iob->value.eios.ioss) - iob->value.eios.w_idx;
        size_t writable = 0;
        for (size_t i = iob->value.eios.w_idx; i < num; i++)
        {
            writable += z_iosli_writable((z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i));
        }

        return writable;
    }
}

int z_iobuf_write(z_iobuf_t *iob, uint8_t b)
{
    if (z_iobuf_is_contigous(iob))
    {
        z_iosli_t *ios = &iob->value.cios;
        int res = z_iosli_write(ios, b);
        if (res == 0)
        {
            return 0;
        }
        else if (z_iobuf_is_expandable(iob))
        {
            res = z_iosli_resize(ios, ios->capacity + iob->value.eios.chunk);
            if (res != 0)
                return -1;
            return z_iosli_write(ios, b);
        }
        else
        {
            return -1;
        }
    }
    else
    {
        do
        {
            z_iosli_t *ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, iob->value.eios.w_idx);
            int res = z_iosli_write(ios, b);
            if (res == 0)
            {
                return 0;
            }
            else if (iob->value.eios.w_idx + 1 < z_vec_len(&iob->value.eios.ioss))
            {
                iob->value.eios.w_idx++;
            }
            else if (z_iobuf_is_expandable(iob))
            {
                // Create a new iobuf
                z_iosli_t niob = z_iosli_make(iob->value.eios.chunk);
                // Add the new iobuf to the current buffer
                z_iobuf_add_iosli(iob, &niob);

                iob->value.eios.w_idx++;
            }
            else
            {
                return -1;
            }
        } while (1);
    }
}

int z_iobuf_write_slice(z_iobuf_t *iob, const uint8_t *bs, size_t offset, size_t length)
{
    if (z_iobuf_is_contigous(iob))
    {
        z_iosli_t *ios = &iob->value.cios;
        int res = z_iosli_write_slice(ios, bs, offset, length);
        if (res == 0)
        {
            return 0;
        }
        else if (z_iobuf_is_expandable(iob))
        {
            // Compute the new target size
            size_t min_cap = ios->capacity + length - z_iosli_writable(ios);
            size_t new_cap = ios->capacity + iob->value.eios.chunk;
            while (new_cap < min_cap)
            {
                new_cap += iob->value.eios.chunk;
            }

            // Resize the iosli
            res = z_iosli_resize(ios, new_cap);
            if (res != 0)
                return -1;

            // Write the slice
            return z_iosli_write_slice(ios, bs, offset, length);
        }
        else
        {
            return -1;
        }
    }
    else
    {
        do
        {
            z_iosli_t *ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, iob->value.eios.w_idx);
            int res = z_iosli_write_slice(ios, bs, offset, length);

            if (res == 0)
            {
                return 0;
            }
            else if (iob->value.eios.w_idx + 1 < z_vec_len(&iob->value.eios.ioss))
            {
                iob->value.eios.w_idx++;
            }
            else if (z_iobuf_is_expandable(iob))
            {
                // Write what remains here
                size_t writable = z_iosli_writable(ios);
                int res = z_iosli_write_slice(ios, bs, offset, writable);
                if (res != 0)
                    return -1;

                // Update the offset and the remaining length
                offset += writable;
                length -= writable;

                // Create a new iosli
                size_t new_cap = iob->value.eios.chunk;
                while (new_cap < length)
                {
                    new_cap += iob->value.eios.chunk;
                }
                z_iosli_t nios = z_iosli_make(new_cap);
                // Add the new iobuf to the current buffer
                z_iobuf_add_iosli(iob, &nios);

                iob->value.eios.w_idx++;
            }
            else
            {
                return -1;
            }
        } while (1);
    }
}

int z_iobuf_write_bytes(z_iobuf_t *iob, const uint8_t *bs, size_t length)
{
    return z_iobuf_write_slice(iob, bs, 0, length);
}

void z_iobuf_put(z_iobuf_t *iob, uint8_t b, size_t pos)
{
    if (z_iobuf_is_contigous(iob))
    {
        z_iosli_put(&iob->value.cios, b, pos);
        return;
    }
    else
    {
        size_t i = 0;
        do
        {
            z_iosli_t *ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i);
            if (pos < ios->capacity)
            {
                z_iosli_put(ios, pos, b);
                return;
            }

            i++;
            pos -= ios->capacity;
        } while (1);
    }
}

size_t z_iobuf_readable(const z_iobuf_t *iob)
{
    if (z_iobuf_is_contigous(iob))
    {
        return z_iosli_readable(&iob->value.cios);
    }
    else
    {
        size_t len = z_vec_len(&iob->value.eios.ioss) - iob->value.eios.r_idx;
        size_t readable = 0;
        for (size_t i = iob->value.eios.r_idx; i < len; i++)
        {
            readable += z_iosli_readable((z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i));
        }

        return readable;
    }
}

uint8_t z_iobuf_read(z_iobuf_t *iob)
{
    if (z_iobuf_is_contigous(iob))
    {
        return z_iosli_read(&iob->value.cios);
    }
    else
    {
        z_iosli_t *iosli;
        do
        {
            iosli = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, iob->value.eios.r_idx);
            if (z_iosli_readable(iosli) > 0)
            {
                return z_iosli_read(iosli);
            }
            else
            {
                iob->value.eios.r_idx++;
            }
        } while (1);
    }
}

void z_iobuf_read_to_n(z_iobuf_t *iob, uint8_t *dst, size_t length)
{
    if (z_iobuf_is_contigous(iob))
    {
        z_iosli_read_to_n(&iob->value.cios, dst, length);
        return;
    }
    else
    {
        uint8_t *w_pos = dst;
        do
        {
            z_iosli_t *ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, iob->value.eios.r_idx);
            size_t readbale = z_iosli_readable(ios);
            if (readbale > 0)
            {
                size_t to_read = readbale < length ? readbale : length;
                z_iosli_read_to_n(ios, w_pos, to_read);
                length -= to_read;
                w_pos += to_read;
            }
            else
            {
                iob->value.eios.r_idx++;
            }
        } while (length > 0);

        return;
    }
}

uint8_t *z_iobuf_read_n(z_iobuf_t *iob, size_t length)
{
    uint8_t *dst = (uint8_t *)malloc(length);
    z_iobuf_read_to_n(iob, dst, length);
    return dst;
}

uint8_t z_iobuf_get(const z_iobuf_t *iob, size_t pos)
{
    if (z_iobuf_is_contigous(iob))
    {
        return z_iosli_get(&iob->value.cios, pos);
    }
    else
    {
        size_t i = 0;
        do
        {
            z_iosli_t *ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i);
            if (pos < ios->capacity)
            {
                return z_iosli_get(ios, pos);
            }

            i++;
            pos -= ios->capacity;
        } while (1);
    }
}

void z_iobuf_set_rpos(z_iobuf_t *iob, size_t pos)
{
    if (z_iobuf_is_contigous(iob))
    {
        iob->value.cios.r_pos = pos;
    }
    else
    {
        size_t i = 0;
        do
        {
            z_iosli_t *ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i);
            if (pos < ios->capacity)
            {
                assert(i <= iob->value.eios.w_idx && pos <= ios->w_pos);
                iob->value.eios.r_idx = i;
                ios->r_pos = pos;
                return;
            }

            i++;
            pos -= ios->capacity;
        } while (1);
    }
}

void z_iobuf_set_wpos(z_iobuf_t *iob, size_t pos)
{
    if (z_iobuf_is_contigous(iob))
    {
        iob->value.cios.w_pos = pos;
    }
    else
    {
        size_t i = 0;
        do
        {
            z_iosli_t *ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i);
            if (pos < ios->capacity)
            {
                assert(i >= iob->value.eios.r_idx && pos >= ios->r_pos);
                iob->value.eios.w_idx = i;
                ios->w_pos = pos;
                return;
            }

            i++;
            pos -= ios->capacity;
        } while (1);
    }
}

size_t z_iobuf_get_rpos(const z_iobuf_t *iob)
{
    if (z_iobuf_is_contigous(iob))
    {
        return iob->value.cios.r_pos;
    }
    else
    {
        z_iosli_t *ios;
        size_t r_pos = 0;
        for (size_t i = 0; i < iob->value.eios.r_idx; i++)
        {
            ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i);
            r_pos += ios->capacity;
        }
        ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, iob->value.eios.r_idx);
        r_pos += ios->r_pos;
        return r_pos;
    }
}

size_t z_iobuf_get_wpos(const z_iobuf_t *iob)
{
    if (z_iobuf_is_contigous(iob))
    {
        return iob->value.cios.w_pos;
    }
    else
    {
        z_iosli_t *ios;
        size_t w_pos = 0;
        for (size_t i = 0; i < iob->value.eios.w_idx; i++)
        {
            ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i);
            w_pos += ios->capacity;
        }
        ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, iob->value.eios.w_idx);
        w_pos += ios->w_pos;
        return w_pos;
    }
}

z_uint8_array_t z_iobuf_to_array(const z_iobuf_t *iob)
{
    assert(z_iobuf_is_contigous(iob));
    const z_iosli_t *ios = &iob->value.cios;
    z_uint8_array_t a;
    a.length = z_iosli_readable(ios);
    a.elem = &ios->buf[ios->r_pos];
    return a;
}

int z_iobuf_copy_into(z_iobuf_t *dst, const z_iobuf_t *src)
{
    if (z_iobuf_is_contigous(src))
    {
        if (z_iobuf_is_expandable(dst) && !z_iobuf_is_contigous(dst))
        {
            // Do not copy the payload, directly add the slice
            z_iobuf_add_iosli(dst, &src->value.cios);
            return 0;
        }
        else
        {
            return z_iobuf_write_slice(dst, src->value.cios.buf, src->value.cios.r_pos, z_iosli_readable(&src->value.cios));
        }
    }
    else
    {
        if (z_iobuf_is_expandable(dst) && !z_iobuf_is_contigous(dst))
        {
            // Do not copy the payload, directly add the slices
            for (size_t i = 0; i < z_iobuf_len_iosli(src); i++)
            {
                z_iosli_t *ios = z_iobuf_get_iosli(src, i);
                z_iobuf_add_iosli(dst, ios);
            }
            return 0;
        }
        else
        {
            for (size_t i = 0; i < z_iobuf_len_iosli(src); i++)
            {
                z_iosli_t *ios = z_iobuf_get_iosli(src, i);
                int res = z_iobuf_write_slice(dst, ios->buf, ios->r_pos, z_iosli_readable(ios));
                if (res != 0)
                    return -1;
            }
            return 0;
        }
    }
}

void z_iobuf_clear(z_iobuf_t *iob)
{
    if (z_iobuf_is_contigous(iob))
    {
        z_iosli_clear(&iob->value.cios);
    }
    else
    {
        iob->value.eios.r_idx = 0;
        iob->value.eios.w_idx = 0;
        for (size_t i = 0; i < z_vec_len(&iob->value.eios.ioss); i++)
            z_iosli_clear((z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i));
    }
}

void z_iobuf_compact(z_iobuf_t *iob)
{
    assert(z_iobuf_is_contigous(iob));
    z_iosli_compact(&iob->value.cios);
}

void z_iobuf_free(z_iobuf_t *iob)
{
    if (z_iobuf_is_contigous(iob))
    {
        z_iosli_free(&iob->value.cios);
    }
    else
    {
        for (size_t i = 0; i < z_vec_len(&iob->value.eios.ioss); i++)
            z_iosli_free((z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i));
        z_vec_free_inner(&iob->value.eios.ioss);
    }

    iob = NULL;
}
