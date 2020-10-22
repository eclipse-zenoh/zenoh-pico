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

void z_iosli_free(z_iosli_t *ios)
{
    free(ios->buf);
    ios = NULL;
}

/*------------------ IOSliArray ------------------*/
z_iosli_array_t z_iosli_array_make(size_t capacity)
{
    z_iosli_array_t a;
    a.capacity = capacity;
    a.length = 0;
    a.elem = (z_iosli_t *)malloc(capacity * sizeof(z_iosli_t));
    return a;
}

void z_iosli_array_append(z_iosli_array_t *ioss, z_iosli_t ios)
{
    if (ioss->length == ioss->capacity)
    {
        // Allocate more elements if we ran out of space
        ioss->capacity *= 2;
        ioss->elem = (z_iosli_t *)realloc(ioss->elem, ioss->capacity);
    }
    if (ioss->length > 0)
    {
        // We are appending a new iosli, the last iosli becomes
        // no longer writable
        z_iosli_t *last = &ioss->elem[ioss->length - 1];
        last->capacity = last->w_pos;
    }

    ioss->elem[ioss->length] = ios;
    ioss->length++;
}

void z_iosli_array_free(z_iosli_array_t *ioss)
{
    for (size_t i = 0; i < ioss->length; i++)
    {
        z_iosli_free(&ioss->elem[i]);
    }
    free(ioss->elem);
    ioss = NULL;
}

/*------------------ RBuf ------------------*/
z_rbuf_t z_rbuf_wrap(z_iosli_t ios, int is_expandable)
{
    z_rbuf_t rbf;
    rbf.ios = ios;
    rbf.is_expandable = is_expandable;
    return rbf;
}

z_rbuf_t z_rbuf_make(size_t capacity, int is_expandable)
{
    return z_rbuf_wrap(z_iosli_make(capacity), is_expandable);
}

z_rbuf_t z_rbuf_view(z_rbuf_t *rbf, size_t length)
{
    z_rbuf_t v;
    v.is_expandable = 0;
    v.ios = z_iosli_wrap((uint8_t *)rbf->ios.buf + rbf->ios.r_pos, length, 0, length);
    return v;
}

size_t z_rbuf_readable(const z_rbuf_t *rbf)
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

size_t z_rbuf_writable(const z_rbuf_t *rbf)
{
    return z_iosli_writable(&rbf->ios);
}

int z_rbuf_write(z_rbuf_t *rbf, uint8_t b)
{
    size_t writable = z_iosli_writable(&rbf->ios);
    if (writable >= 1)
    {
        z_iosli_write(&rbf->ios, b);
        return 0;
    }
    else if (rbf->is_expandable)
    {
        z_iosli_resize(&rbf->ios, 2 * rbf->ios.capacity);
        z_iosli_write(&rbf->ios, b);
        return 0;
    }
    else
    {
        return -1;
    }
}

int z_rbuf_write_bytes(z_rbuf_t *rbf, const uint8_t *bs, size_t offset, size_t length)
{
    size_t writable = z_iosli_writable(&rbf->ios);
    if (writable >= length)
    {
        z_iosli_write_bytes(&rbf->ios, bs, offset, length);
        return 0;
    }
    else if (rbf->is_expandable)
    {
        z_iosli_resize(&rbf->ios, rbf->ios.capacity + length - writable);
        z_iosli_write_bytes(&rbf->ios, bs, offset, length);
        return 0;
    }
    else
    {
        return -1;
    }
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

void z_rbuf_clear(z_rbuf_t *rbf)
{
    z_iosli_clear(&rbf->ios);
}

void z_rbuf_compact(z_rbuf_t *rbf)
{
    z_iosli_compact(&rbf->ios);
}

void z_rbuf_free(z_rbuf_t *rbf)
{
    z_iosli_free(&rbf->ios);
    rbf = NULL;
}

/*------------------ WBuf ------------------*/
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
        wbf.ioss = z_iosli_array_make(4);
    }
    else
    {
        wbf.ioss = z_iosli_array_make(1);
    }
    z_iosli_array_append(&wbf.ioss, z_iosli_make(capacity));
    wbf.is_expandable = is_expandable;

    return wbf;
}

size_t z_wbuf_readable(const z_wbuf_t *wbf)
{
    size_t readable = 0;
    for (size_t i = wbf->r_idx; i <= wbf->w_idx; i++)
    {
        readable += z_iosli_readable(&wbf->ioss.elem[i]);
    }
    return readable;
}

uint8_t z_wbuf_read(z_wbuf_t *wbf)
{
    do
    {
        assert(wbf->r_idx <= wbf->w_idx);
        size_t readable = z_iosli_readable(&wbf->ioss.elem[wbf->r_idx]);
        if (readable > 0)
        {
            return z_iosli_read(&wbf->ioss.elem[wbf->r_idx]);
        }
        else
        {
            wbf->r_idx++;
        }
    } while (1);
}

void z_wbuf_read_bytes(z_wbuf_t *wbf, uint8_t *dest, size_t offset, size_t length)
{
    do
    {
        assert(wbf->r_idx <= wbf->w_idx);
        size_t readable = z_iosli_readable(&wbf->ioss.elem[wbf->r_idx]);
        if (readable > 0)
        {
            size_t to_read = readable <= length ? readable : length;
            z_iosli_read_bytes(&wbf->ioss.elem[wbf->r_idx], dest, offset, length);
            offset += to_read;
            length -= to_read;
        }
        else
        {
            wbf->r_idx++;
        }
    } while (length > 0);
}

uint8_t z_wbuf_get(const z_wbuf_t *wbf, size_t pos)
{
    size_t i = 0;
    do
    {
        assert(i < wbf->ioss.length);
        z_iosli_t *ios = &wbf->ioss.elem[i];
        if (pos < ios->capacity)
        {
            return z_iosli_get(ios, pos);
        }
        else
        {
            pos -= ios->capacity;
        }
    } while (1);
}

size_t z_wbuf_writable(const z_wbuf_t *wbf)
{
    return z_iosli_writable(&wbf->ioss.elem[wbf->w_idx]);
}

int z_wbuf_write(z_wbuf_t *wbf, uint8_t b)
{
    size_t writable = z_iosli_writable(&wbf->ioss.elem[wbf->w_idx]);
    if (writable >= 1)
    {
        z_iosli_write(&wbf->ioss.elem[wbf->w_idx], b);
        return 0;
    }
    else if (wbf->is_expandable)
    {
        z_iosli_t ios = z_iosli_make(wbf->capacity);
        z_iosli_array_append(&wbf->ioss, ios);
        wbf->w_idx++;
        z_iosli_write(&wbf->ioss.elem[wbf->w_idx], b);
        return 0;
    }
    else
    {
        return -1;
    }
}

int z_wbuf_write_bytes(z_wbuf_t *wbf, const uint8_t *bs, size_t offset, size_t length)
{
    size_t writable = z_iosli_writable(&wbf->ioss.elem[wbf->w_idx]);
    if (writable >= length)
    {
        z_iosli_write_bytes(&wbf->ioss.elem[wbf->w_idx], bs, offset, length);
        return 0;
    }
    else if (wbf->is_expandable)
    {
        // Write what left
        if (writable > 0)
        {
            z_iosli_write_bytes(&wbf->ioss.elem[wbf->w_idx], bs, offset, writable);
            offset += writable;
            length -= writable;
        }

        // Allocate N capacity slots to hold at least length bytes
        size_t capacity = wbf->capacity * (1 + (wbf->capacity % length));
        z_iosli_t ios = z_iosli_make(capacity);
        z_iosli_array_append(&wbf->ioss, ios);
        wbf->w_idx++;
        z_iosli_write_bytes(&wbf->ioss.elem[wbf->w_idx], bs, offset, writable);
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
    do
    {
        assert(i < wbf->ioss.length);
        z_iosli_t *ios = &wbf->ioss.elem[i];
        if (pos < ios->capacity)
        {
            z_iosli_put(ios, b, pos);
            return;
        }

        pos -= ios->capacity;
    } while (++i);
}

size_t z_wbuf_get_rpos(const z_wbuf_t *wbf)
{
    size_t pos = 0;
    for (size_t i = 0; i < wbf->r_idx; i++)
    {
        pos += wbf->ioss.elem[i].capacity;
    }
    pos += wbf->ioss.elem[wbf->r_idx].r_pos;
    return pos;
}

size_t z_wbuf_get_wpos(const z_wbuf_t *wbf)
{
    size_t pos = 0;
    for (size_t i = 0; i < wbf->w_idx; i++)
    {
        pos += wbf->ioss.elem[i].capacity;
    }
    pos += wbf->ioss.elem[wbf->w_idx].w_pos;
    return pos;
}

void z_wbuf_set_rpos(z_wbuf_t *wbf, size_t pos)
{
    size_t i = 0;
    do
    {
        assert(i <= wbf->w_idx);
        z_iosli_t *ios = &wbf->ioss.elem[i];
        if (pos <= ios->w_pos)
        {
            ios->r_pos = pos;
            return;
        }

        pos -= ios->capacity;
    } while (++i);
}

void z_wbuf_set_wpos(z_wbuf_t *wbf, size_t pos)
{
    size_t i = 0;
    do
    {
        assert(i <= wbf->ioss.length);
        z_iosli_t *ios = &wbf->ioss.elem[i];
        if (pos <= ios->capacity)
        {
            assert(pos >= ios->r_pos);
            ios->w_pos = pos;
            return;
        }

        pos -= ios->capacity;
    } while (++i);
}

z_uint8_array_t z_wbuf_to_array(const z_wbuf_t *wbf)
{
    assert(!wbf->is_expandable);
    z_iosli_t *ios = &wbf->ioss.elem[0];
    z_uint8_array_t a;
    a.length = z_iosli_readable(ios);
    a.elem = (uint8_t *)ios->buf + ios->r_pos;
    return a;
}

void z_wbuf_clear(z_wbuf_t *wbf)
{
    wbf->r_idx = 0;
    wbf->w_idx = 0;
    for (size_t i = 0; i < wbf->ioss.length; i++)
    {
        z_iosli_clear(&wbf->ioss.elem[i]);
    }
}

void z_wbuf_free(z_wbuf_t *wbf)
{
    z_iosli_array_free(&wbf->ioss);
    wbf = NULL;
}

// int z_iobuf_is_contigous(const z_iobuf_t *iob)
// {
//     return iob->mode & Z_IOBUF_MODE_CONTIGOUS;
// }

// int z_iobuf_is_expandable(const z_iobuf_t *iob)
// {
//     return iob->mode & Z_IOBUF_MODE_EXPANDABLE;
// }

// void z_iobuf_add_iosli(z_iobuf_t *iob, const z_iosli_t *ios)
// {
//     assert(!z_iobuf_is_contigous(iob) && z_iobuf_is_expandable(iob));
//     z_vec_append(&iob->value.eios.ioss, (z_iosli_t *)ios);
// }

// z_iosli_t *z_iobuf_get_iosli(const z_iobuf_t *iob, unsigned int idx)
// {
//     assert(!z_iobuf_is_contigous(iob));
//     return (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, idx);
// }

// size_t z_iobuf_len_iosli(const z_iobuf_t *iob)
// {
//     assert(!z_iobuf_is_contigous(iob));
//     return z_vec_len(&iob->value.eios.ioss);
// }

// z_iobuf_t z_iobuf_uninit(unsigned int mode)
// {
//     z_iobuf_t iob;
//     iob.mode = mode;

//     if (z_iobuf_is_contigous(&iob))
//     {
//         iob.value.cios.capacity = 0;
//         iob.value.cios.r_pos = 0;
//         iob.value.cios.w_pos = 0;
//         iob.value.cios.buf = NULL;
//     }
//     else
//     {
//         iob.value.eios.r_idx = 0;
//         iob.value.eios.w_idx = 0;
//         iob.value.eios.chunk = 0;
//         iob.value.eios.ioss = z_vec_uninit();
//     }

//     return iob;
// }

// z_iobuf_t z_iobuf_make(size_t chunk, unsigned int mode)
// {
//     z_iobuf_t iob = z_iobuf_uninit(mode);
//     if (z_iobuf_is_contigous(&iob))
//     {
//         iob.value.cios = z_iosli_make(chunk);
//     }
//     else
//     {
//         iob.value.eios.chunk = chunk;
//         iob.value.eios.ioss = z_vec_make(1);
//     }

//     return iob;
// }

// z_iobuf_t z_iobuf_wrap_wo(uint8_t *buf, size_t capacity, size_t r_pos, size_t w_pos)
// {
//     z_iobuf_t iob = z_iobuf_uninit(Z_IOBUF_MODE_CONTIGOUS);
//     iob.value.cios = z_iosli_wrap_wo(buf, capacity, r_pos, w_pos);

//     return iob;
// }

// z_iobuf_t z_iobuf_wrap(uint8_t *buf, size_t capacity)
// {
//     return z_iobuf_wrap_wo(buf, capacity, 0, 0);
// }

// size_t z_iobuf_writable(const z_iobuf_t *iob)
// {
//     if (z_iobuf_is_contigous(iob))
//     {
//         return z_iosli_writable(&iob->value.cios);
//     }
//     else
//     {
//         size_t writable = 0;
//         for (size_t i = iob->value.eios.w_idx; i < z_vec_len(&iob->value.eios.ioss); i++)
//         {
//             writable += z_iosli_writable((z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i));
//         }
//         return writable;
//     }
// }

// int z_iobuf_write(z_iobuf_t *iob, uint8_t b)
// {
//     if (z_iobuf_is_contigous(iob))
//     {
//         z_iosli_t *ios = &iob->value.cios;
//         int res = z_iosli_write(ios, b);
//         if (res == 0)
//         {
//             return 0;
//         }
//         else if (z_iobuf_is_expandable(iob))
//         {
//             res = z_iosli_resize(ios, ios->capacity + iob->value.eios.chunk);
//             if (res != 0)
//                 return -1;
//             return z_iosli_write(ios, b);
//         }
//         else
//         {
//             return -1;
//         }
//     }
//     else
//     {
//         do
//         {
//             z_iosli_t *ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, iob->value.eios.w_idx);
//             int res = z_iosli_write(ios, b);
//             if (res == 0)
//             {
//                 return 0;
//             }
//             else if (iob->value.eios.w_idx + 1 < z_vec_len(&iob->value.eios.ioss))
//             {
//                 iob->value.eios.w_idx++;
//             }
//             else if (z_iobuf_is_expandable(iob))
//             {
//                 // Add a new iosli to the current iob
//                 z_iobuf_add_iosli(iob, z_iosli_alloc(iob->value.eios.chunk));
//                 iob->value.eios.w_idx++;
//             }
//             else
//             {
//                 return -1;
//             }
//         } while (1);
//     }
// }

// int z_iobuf_write_slice(z_iobuf_t *iob, const uint8_t *bs, size_t offset, size_t length)
// {
//     if (z_iobuf_is_contigous(iob))
//     {
//         z_iosli_t *ios = &iob->value.cios;
//         int res = z_iosli_write_slice(ios, bs, offset, length);
//         if (res == 0)
//         {
//             return 0;
//         }
//         else if (z_iobuf_is_expandable(iob))
//         {
//             // Compute the new target size
//             size_t min_cap = ios->capacity + length - z_iosli_writable(ios);
//             size_t new_cap = ios->capacity + iob->value.eios.chunk;
//             while (new_cap < min_cap)
//             {
//                 new_cap += iob->value.eios.chunk;
//             }

//             // Resize the iosli
//             res = z_iosli_resize(ios, new_cap);
//             if (res != 0)
//                 return -1;

//             // Write the slice
//             return z_iosli_write_slice(ios, bs, offset, length);
//         }
//         else
//         {
//             return -1;
//         }
//     }
//     else
//     {
//         do
//         {
//             z_iosli_t *ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, iob->value.eios.w_idx);
//             int res = z_iosli_write_slice(ios, bs, offset, length);

//             if (res == 0)
//             {
//                 return 0;
//             }
//             else if (iob->value.eios.w_idx + 1 < z_vec_len(&iob->value.eios.ioss))
//             {
//                 iob->value.eios.w_idx++;
//             }
//             else if (z_iobuf_is_expandable(iob))
//             {
//                 // Write what remains here
//                 size_t writable = z_iosli_writable(ios);
//                 int res = z_iosli_write_slice(ios, bs, offset, writable);
//                 if (res != 0)
//                 {
//                     return -1;
//                 }

//                 // Update the offset and the remaining length
//                 offset += writable;
//                 length -= writable;

//                 // Create a new iosli
//                 size_t new_cap = iob->value.eios.chunk;
//                 while (new_cap < length)
//                 {
//                     new_cap += iob->value.eios.chunk;
//                 }
//                 // Add a new iosli to the current iobuf
//                 z_iobuf_add_iosli(iob, z_iosli_alloc(new_cap));

//                 iob->value.eios.w_idx++;
//             }
//             else
//             {
//                 return -1;
//             }
//         } while (1);
//     }
// }

// int z_iobuf_write_bytes(z_iobuf_t *iob, const uint8_t *bs, size_t length)
// {
//     return z_iobuf_write_slice(iob, bs, 0, length);
// }

// void z_iobuf_put(z_iobuf_t *iob, uint8_t b, size_t pos)
// {
//     if (z_iobuf_is_contigous(iob))
//     {
//         z_iosli_put(&iob->value.cios, b, pos);
//         return;
//     }
//     else
//     {
//         size_t i = 0;
//         do
//         {
//             z_iosli_t *ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i);
//             if (pos < ios->capacity)
//             {
//                 z_iosli_put(ios, pos, b);
//                 return;
//             }

//             i++;
//             pos -= ios->capacity;
//         } while (1);
//     }
// }

// size_t z_iobuf_readable(const z_iobuf_t *iob)
// {
//     if (z_iobuf_is_contigous(iob))
//     {
//         return z_iosli_readable(&iob->value.cios);
//     }
//     else
//     {
//         size_t readable = 0;
//         for (size_t i = iob->value.eios.r_idx; i <= iob->value.eios.w_idx; i++)
//         {
//             readable += z_iosli_readable((z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i));
//         }
//         return readable;
//     }
// }

// uint8_t z_iobuf_read(z_iobuf_t *iob)
// {
//     if (z_iobuf_is_contigous(iob))
//     {
//         return z_iosli_read(&iob->value.cios);
//     }
//     else
//     {
//         z_iosli_t *iosli;
//         do
//         {
//             iosli = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, iob->value.eios.r_idx);
//             if (z_iosli_readable(iosli) > 0)
//             {
//                 return z_iosli_read(iosli);
//             }
//             else
//             {
//                 iob->value.eios.r_idx++;
//             }
//         } while (1);
//     }
// }

// void z_iobuf_read_to_n(z_iobuf_t *iob, uint8_t *dst, size_t length)
// {
//     if (z_iobuf_is_contigous(iob))
//     {
//         z_iosli_read_to_n(&iob->value.cios, dst, length);
//         return;
//     }
//     else
//     {
//         uint8_t *w_pos = dst;
//         do
//         {
//             z_iosli_t *ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, iob->value.eios.r_idx);
//             size_t readbale = z_iosli_readable(ios);
//             if (readbale > 0)
//             {
//                 size_t to_read = readbale < length ? readbale : length;
//                 z_iosli_read_to_n(ios, w_pos, to_read);
//                 length -= to_read;
//                 w_pos += to_read;
//             }
//             else
//             {
//                 iob->value.eios.r_idx++;
//             }
//         } while (length > 0);

//         return;
//     }
// }

// uint8_t *z_iobuf_read_n(z_iobuf_t *iob, size_t length)
// {
//     uint8_t *dst = (uint8_t *)malloc(length);
//     z_iobuf_read_to_n(iob, dst, length);
//     return dst;
// }

// uint8_t z_iobuf_get(const z_iobuf_t *iob, size_t pos)
// {
//     if (z_iobuf_is_contigous(iob))
//     {
//         return z_iosli_get(&iob->value.cios, pos);
//     }
//     else
//     {
//         size_t i = 0;
//         do
//         {
//             z_iosli_t *ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i);
//             if (pos < ios->capacity)
//             {
//                 return z_iosli_get(ios, pos);
//             }

//             i++;
//             pos -= ios->capacity;
//         } while (1);
//     }
// }

// void z_iobuf_set_rpos(z_iobuf_t *iob, size_t pos)
// {
//     if (z_iobuf_is_contigous(iob))
//     {
//         iob->value.cios.r_pos = pos;
//     }
//     else
//     {
//         size_t i = 0;
//         do
//         {
//             z_iosli_t *ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i);
//             if (pos < ios->capacity)
//             {
//                 assert(i <= iob->value.eios.w_idx && pos <= ios->w_pos);
//                 iob->value.eios.r_idx = i;
//                 ios->r_pos = pos;
//                 return;
//             }

//             i++;
//             pos -= ios->capacity;
//         } while (1);
//     }
// }

// void z_iobuf_set_wpos(z_iobuf_t *iob, size_t pos)
// {
//     if (z_iobuf_is_contigous(iob))
//     {
//         iob->value.cios.w_pos = pos;
//     }
//     else
//     {
//         size_t i = 0;
//         do
//         {
//             z_iosli_t *ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i);
//             if (pos < ios->capacity)
//             {
//                 assert(i >= iob->value.eios.r_idx && pos >= ios->r_pos);
//                 iob->value.eios.w_idx = i;
//                 ios->w_pos = pos;
//                 return;
//             }

//             i++;
//             pos -= ios->capacity;
//         } while (1);
//     }
// }

// size_t z_iobuf_get_rpos(const z_iobuf_t *iob)
// {
//     if (z_iobuf_is_contigous(iob))
//     {
//         return iob->value.cios.r_pos;
//     }
//     else
//     {
//         z_iosli_t *ios;
//         size_t r_pos = 0;
//         for (size_t i = 0; i < iob->value.eios.r_idx; i++)
//         {
//             ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i);
//             r_pos += ios->capacity;
//         }
//         ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, iob->value.eios.r_idx);
//         r_pos += ios->r_pos;
//         return r_pos;
//     }
// }

// size_t z_iobuf_get_wpos(const z_iobuf_t *iob)
// {
//     if (z_iobuf_is_contigous(iob))
//     {
//         return iob->value.cios.w_pos;
//     }
//     else
//     {
//         z_iosli_t *ios;
//         size_t w_pos = 0;
//         for (size_t i = 0; i < iob->value.eios.w_idx; i++)
//         {
//             ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i);
//             w_pos += ios->capacity;
//         }
//         ios = (z_iosli_t *)z_vec_get(&iob->value.eios.ioss, iob->value.eios.w_idx);
//         w_pos += ios->w_pos;
//         return w_pos;
//     }
// }

// z_uint8_array_t z_iobuf_to_array(const z_iobuf_t *iob)
// {
//     assert(z_iobuf_is_contigous(iob));
//     const z_iosli_t *ios = &iob->value.cios;
//     z_uint8_array_t a;
//     a.length = z_iosli_readable(ios);
//     a.elem = &ios->buf[ios->r_pos];
//     return a;
// }

// int z_iobuf_copy_into(z_iobuf_t *dst, const z_iobuf_t *src)
// {
//     if (z_iobuf_is_contigous(src))
//     {
//         if (z_iobuf_is_expandable(dst) && !z_iobuf_is_contigous(dst))
//         {
//             // Do not copy the payload, directly add the slice
//             z_iosli_t *ios = z_iosli_copy_shallow((z_iosli_t *)&src->value.cios);
//             z_iobuf_add_iosli(dst, ios);
//             return 0;
//         }
//         else
//         {
//             return z_iobuf_write_slice(dst, src->value.cios.buf, src->value.cios.r_pos, z_iosli_readable(&src->value.cios));
//         }
//     }
//     else
//     {
//         if (z_iobuf_is_expandable(dst) && !z_iobuf_is_contigous(dst))
//         {
//             printf("ANDA: %zu %zu %zu\n", z_iobuf_len_iosli(src), src->value.eios.r_idx, src->value.eios.w_idx);
//             // Do not copy the payload, directly add the slices
//             for (size_t i = src->value.eios.r_idx; i <= src->value.eios.w_idx; ++i)
//             {
//                 printf("POR AQUI: %zu\n", i);
//                 z_iosli_t *ios = z_iosli_copy_shallow(z_iobuf_get_iosli(src, i));
//                 z_iobuf_add_iosli(dst, ios);
//             }
//             return 0;
//         }
//         else
//         {
//             for (size_t i = 0; i < z_iobuf_len_iosli(src); i++)
//             {
//                 z_iosli_t *ios = z_iobuf_get_iosli(src, i);
//                 int res = z_iobuf_write_slice(dst, ios->buf, ios->r_pos, z_iosli_readable(ios));
//                 if (res != 0)
//                     return -1;
//             }
//             return 0;
//         }
//     }
// }

// void z_iobuf_clear(z_iobuf_t *iob)
// {
//     if (z_iobuf_is_contigous(iob))
//     {
//         z_iosli_clear(&iob->value.cios);
//     }
//     else
//     {
//         iob->value.eios.r_idx = 0;
//         iob->value.eios.w_idx = 0;
//         for (size_t i = 0; i < z_vec_len(&iob->value.eios.ioss); i++)
//             z_iosli_clear((z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i));
//     }
// }

// void z_iobuf_compact(z_iobuf_t *iob)
// {
//     assert(z_iobuf_is_contigous(iob));
//     z_iosli_compact(&iob->value.cios);
// }

// void z_iobuf_free(z_iobuf_t *iob)
// {
//     if (z_iobuf_is_contigous(iob))
//     {
//         z_iosli_free(&iob->value.cios);
//     }
//     else
//     {
//         for (size_t i = 0; i < z_vec_len(&iob->value.eios.ioss); i++)
//             z_iosli_free((z_iosli_t *)z_vec_get(&iob->value.eios.ioss, i));
//         z_vec_free(&iob->value.eios.ioss);
//     }

//     iob = NULL;
// }
