/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *     ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <stdlib.h>
#include <string.h>
#include "zenoh-pico/collections/bytes.h"

/*-------- bytes --------*/
void _z_bytes_init(_z_bytes_t *bs, size_t capacity)
{
    bs->val = (uint8_t *)malloc(capacity * sizeof(uint8_t));
    bs->len = capacity;
    bs->_is_alloc = 1;
}

_z_bytes_t _z_bytes_make(size_t capacity)
{
    _z_bytes_t bs;
    _z_bytes_init(&bs, capacity);
    return bs;
}

_z_bytes_t _z_bytes_wrap(const uint8_t *p, size_t len)
{
    _z_bytes_t bs;
    bs.val = p;
    bs.len = len;
    bs._is_alloc = 0;
    return bs;
}

void _z_bytes_reset(_z_bytes_t *bs)
{
    bs->val = NULL;
    bs->len = 0;
    bs->_is_alloc = 0;
}

void _z_bytes_clear(_z_bytes_t *bs)
{
    if (!bs->_is_alloc)
        return;

    free((uint8_t *)bs->val);
    _z_bytes_reset(bs);
}

void _z_bytes_free(_z_bytes_t **bs)
{
    _z_bytes_t *ptr = (_z_bytes_t *)*bs;
    _z_bytes_clear(ptr);

    free(ptr);
    *bs = NULL;
}

void _z_bytes_copy(_z_bytes_t *dst, const _z_bytes_t *src)
{
    _z_bytes_init(dst, src->len);
    memcpy((uint8_t *)dst->val, src->val, src->len);
}

void _z_bytes_move(_z_bytes_t *dst, _z_bytes_t *src)
{
    dst->val = src->val;
    dst->len = src->len;
    dst->_is_alloc = src->_is_alloc;

    _z_bytes_reset(src);
}

_z_bytes_t _z_bytes_duplicate(const _z_bytes_t *src)
{
    _z_bytes_t dst;
    _z_bytes_copy(&dst, src);
    return dst;
}

uint8_t _z_bytes_is_empty(const _z_bytes_t *bs)
{
    return bs->len == 0;
}
