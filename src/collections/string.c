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
#include "zenoh-pico/utils/collections.h"

/*-------- string --------*/
z_string_t z_string_make(const z_str_t value)
{
    z_string_t s;
    s.val = strdup(value);
    s.len = strlen(value);
    return s;
}

void z_string_free(z_string_t *s)
{
    free((char *)s->val);
}

void _z_string_copy(z_string_t *dst, const z_string_t *src)
{
    if (src->val)
        dst->val = strdup(src->val);
    else
        dst->val = NULL;
    dst->len = src->len;
}

void _z_string_move(z_string_t *dst, z_string_t *src)
{
    dst->val = src->val;
    dst->len = src->len;

    src->val = NULL;
    src->len = 0;
}

void _z_string_move_str(z_string_t *dst, z_str_t src)
{
    dst->val = src;
    dst->len = strlen(src);
}

void _z_string_free(z_string_t *str)
{
    free((z_str_t)str->val);
}

void _z_string_reset(z_string_t *str)
{
    str->val = NULL;
    str->len = 0;
}

z_string_t _z_string_from_bytes(z_bytes_t *bs)
{
    z_string_t s;
    s.len = 2 * bs->len;
    char *s_val = (char *)malloc(s.len * sizeof(char) + 1);

    char c[] = "0123456789ABCDEF";
    for (size_t i = 0; i < bs->len; i++)
    {
        s_val[2 * i] = c[(bs->val[i] & 0xF0) >> 4];
        s_val[2 * i + 1] = c[bs->val[i] & 0x0F];
    }
    s_val[s.len] = '\0';
    s.val = s_val;

    return s;
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

void _z_str_array_copy(z_str_array_t *dst, const z_str_array_t *src)
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
