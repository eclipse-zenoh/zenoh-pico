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

/*------------------ IOBuf ------------------*/
z_iobuf_t z_iobuf_wrap_wo(uint8_t *buf, size_t capacity, size_t rpos, size_t wpos)
{
    assert(rpos < capacity && wpos < capacity);
    z_iobuf_t iob;
    iob.r_pos = rpos;
    iob.w_pos = wpos;
    iob.capacity = capacity;
    iob.buf = buf;
    return iob;
}

z_iobuf_t z_iobuf_wrap(uint8_t *buf, size_t capacity)
{
    return z_iobuf_wrap_wo(buf, capacity, 0, 0);
}

z_iobuf_t z_iobuf_make(size_t capacity)
{
    return z_iobuf_wrap((uint8_t *)malloc(capacity), capacity);
}

int z_iobuf_resize(z_iobuf_t *iob, size_t capacity)
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

void z_iobuf_free(z_iobuf_t *iob)
{
    iob->r_pos = 0;
    iob->w_pos = 0;
    iob->capacity = 0;
    free(iob->buf);
    iob = NULL;
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
    return z_iobuf_write_slice(iob, bs, 0, length);
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

void z_iobuf_clear(z_iobuf_t *iob)
{
    iob->r_pos = 0;
    iob->w_pos = 0;
}

void z_iobuf_compact(z_iobuf_t *iob)
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

/*------------------ WBuf ------------------*/
int z_wbuf_add_iobuf(z_wbuf_t *wb, z_iobuf_t *iob)
{
    if (wb->is_expandable && !wb->is_contigous)
    {
        z_vec_append(&wb->iobs, iob);
        return 0;
    }
    else
    {
        return -1;
    }
}

z_iobuf_t *z_wbuf_get_iobuf(z_wbuf_t *wb, unsigned int idx)
{
    return (z_iobuf_t *)z_vec_get(&wb->iobs, idx);
}

z_wbuf_t z_wbuf_make(size_t capacity, int is_expandable, int is_contigous)
{
    z_wbuf_t wb;
    wb.is_expandable = is_expandable;
    wb.is_contigous = is_contigous;
    wb.capacity = capacity;
    wb.idx = 0;

    wb.iobs = z_vec_make(1);
    z_iobuf_t iob = z_iobuf_make(capacity);
    z_vec_append(&wb.iobs, &iob);

    return wb;
}

size_t z_wbuf_writable(const z_wbuf_t *wb)
{
    size_t len = z_vec_len(&wb->iobs) - wb->idx;
    size_t writable = 0;
    for (size_t i = wb->idx; i < len; i++)
    {
        writable += z_iobuf_writable((z_iobuf_t *)z_vec_get(&wb->iobs, i));
    }

    return writable;
}

int z_wbuf_write(z_wbuf_t *wb, uint8_t b)
{
    do
    {
        z_iobuf_t *iob = (z_iobuf_t *)z_vec_get(&wb->iobs, wb->idx);
        int res = z_iobuf_write(iob, b);
        if (res == 0)
        {
            return 0;
        }
        else if (wb->idx + 1 < z_vec_len(&wb->iobs))
        {
            wb->idx++;
        }
        else if (wb->is_expandable)
        {
            if (wb->is_contigous)
            {
                res = z_iobuf_resize(iob, iob->capacity + wb->capacity);
                if (res != 0)
                    return -1;
            }
            else
            {
                // Create a new slice
                z_iobuf_t niob = z_iobuf_make(wb->capacity);
                // Add the new slice to the current buffer
                z_wbuf_add_iobuf(wb, &niob);

                wb->idx++;
            }
        }
        else
        {
            return -1;
        }

    } while (1);
}

// int z_wbuf_write_slice(z_wbuf_t *iob, const uint8_t *bs, size_t offset, size_t length)
// {
//     do
//     {
//         z_iobuf_t *ios = (z_iobuf_t *)z_vec_get(&wb->iobs, iob->idx);
//         size_t writable = z_iobuf_writable(ios);
//         size_t to_write = writable < length ? writable : length;

//         int res = z_iobuf_write_slice(ios, bs, offset, length);
//         length -= to_write;
//         offset += to_write;

//         if (res == 0 && length == 0)
//         {
//             return 0;
//         }
//         else if (iob->idx + 1 < z_vec_len(&wb->iobs))
//         {
//             iob->idx++;
//         }
//         else if (iob->is_expandable)
//         {
//             // Get the capacity of the first slice
//             z_iobuf_t *fios = (z_iobuf_t *)z_vec_get(&wb->iobs, 0);
//             // Create a new slice
//             z_iobuf_t nios = z_iobuf_make(fios->capacity);
//             // Add the new slice to the current buffer
//             z_wbuf_add_iosli(iob, &nios);

//             iob->idx++;
//         }
//         else
//         {
//             return -1;
//         }
//     } while (1);
// }

// int z_wbuf_write_bytes(z_wbuf_t *iob, const uint8_t *bs, size_t length)
// {
//     return z_wbuf_write_slice(iob, bs, 0, length);
// }

// void z_wbuf_put(z_wbuf_t *iob, uint8_t b, size_t pos)
// {
//     size_t i = 0;
//     do
//     {
//         z_iobuf_t *ios = (z_iobuf_t *)z_vec_get(&wb->iobs, i);
//         if (pos < ios->capacity)
//         {
//             z_iobuf_put(ios, pos, b);
//             return;
//         }

//         i++;
//         pos -= ios->capacity;
//     } while (1);
// }

/*------------------ RBuf ------------------*/
