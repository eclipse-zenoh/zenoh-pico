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

#include <string.h>
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/collections/bytes.h"

/*-------- bytes --------*/
void _z_bytes_init(z_bytes_t *bs, size_t capacity)
{
    bs->val = (uint8_t *)z_malloc(capacity * sizeof(uint8_t));
    bs->len = capacity;
    bs->is_alloc = 1;
}

z_bytes_t _z_bytes_make(size_t capacity)
{
    z_bytes_t bs;
    _z_bytes_init(&bs, capacity);
    return bs;
}

z_bytes_t _z_bytes_wrap(const uint8_t *p, size_t len)
{
    z_bytes_t bs;
    bs.val = p;
    bs.len = len;
    bs.is_alloc = 0;
    return bs;
}

void _z_bytes_reset(z_bytes_t *bs)
{
    bs->val = NULL;
    bs->len = 0;
    bs->is_alloc = 0;
}

void _z_bytes_clear(z_bytes_t *bs)
{
    if (!bs->is_alloc)
        return;

    z_free((uint8_t *)bs->val);
    _z_bytes_reset(bs);
}

void _z_bytes_free(z_bytes_t **bs)
{
    z_bytes_t *ptr = (z_bytes_t *)*bs;
    _z_bytes_clear(ptr);
    *bs = NULL;
}

void _z_bytes_copy(z_bytes_t *dst, const z_bytes_t *src)
{
    _z_bytes_init(dst, src->len);
    memcpy((uint8_t *)dst->val, src->val, src->len);
}

void _z_bytes_move(z_bytes_t *dst, z_bytes_t *src)
{
    dst->val = src->val;
    dst->len = src->len;
    dst->is_alloc = src->is_alloc;

    _z_bytes_reset(src);
}

z_bytes_t _z_bytes_duplicate(const z_bytes_t *src)
{
    z_bytes_t dst;
    _z_bytes_copy(&dst, src);
    return dst;
}

int _z_bytes_is_empty(const z_bytes_t *bs)
{
    return bs->len == 0;
}
