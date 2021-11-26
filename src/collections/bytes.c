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
void _z_bytes_init(z_bytes_t *bs, size_t capacity)
{
    bs->val = (uint8_t *)malloc(capacity * sizeof(uint8_t));
    bs->len = capacity;
}

z_bytes_t _z_bytes_make(size_t capacity)
{
    z_bytes_t bs;
    _z_bytes_init(&bs, capacity);
    return bs;
}

void _z_bytes_clear(z_bytes_t *bs)
{
    free((uint8_t *)bs->val);
    bs->val = NULL;
    bs->len = 0;
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

    src->val = NULL;
    src->len = 0;
}

z_bytes_t _z_bytes_dup(const z_bytes_t *src)
{
    z_bytes_t dst;
    _z_bytes_copy(&dst, src);
    return dst;
}

void _z_bytes_reset(z_bytes_t *bs)
{
    bs->val = NULL;
    bs->len = 0;
}

int _z_bytes_is_empty(const z_bytes_t *bs)
{
    return bs->len == 0;
}
