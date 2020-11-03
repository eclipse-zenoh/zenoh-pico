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
 *     ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <string.h>
#include "zenoh/types.h"

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

void _z_bytes_free(z_bytes_t *bs)
{
    free((uint8_t *)bs->val);
}

void _z_bytes_copy(z_bytes_t *dst, z_bytes_t *src)
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

/*-------- str_array --------*/
void _z_str_array_init(z_str_array_t *sa, size_t len)
{
    z_str_t **val = (z_str_t **)&sa->val;
    *val = (z_str_t *)malloc(len * sizeof(z_str_t));
    sa->len = len;
}

z_str_array_t _z_str_array_make(size_t len)
{
    z_str_array_t sa;
    _z_str_array_init(&sa, len);
    return sa;
}

void _z_str_array_free(z_str_array_t *sa)
{
    for (size_t i = 0; i < sa->len; i++)
        free((z_str_t)sa->val[i]);
    free((z_str_t *)sa->val);
}

void _z_str_array_copy(z_str_array_t *dst, z_str_array_t *src)
{
    _z_str_array_init(dst, src->len);
    for (size_t i = 0; i < src->len; i++)
        ((z_str_t *)dst->val)[i] = strdup(src->val[i]);
    dst->len = src->len;
}

void _z_str_array_move(z_str_array_t *dst, z_str_array_t *src)
{
    dst->val = src->val;
    dst->len = src->len;

    src->val = NULL;
    src->len = 0;
}
