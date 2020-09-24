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

#include <stdio.h>
#include "zenoh/codec.h"
#include "zenoh/private/logging.h"

void z_zint_encode(z_iobuf_t *buf, z_zint_t v)
{
    while (v > 0x7f)
    {
        uint8_t c = (v & 0x7f) | 0x80;
        z_iobuf_write(buf, (uint8_t)c);
        v = v >> 7;
    }
    z_iobuf_write(buf, (uint8_t)v);
}

z_zint_result_t
z_zint_decode(z_iobuf_t *buf)
{
    z_zint_result_t r;
    r.tag = Z_OK_TAG;
    r.value.zint = 0;

    uint8_t c;
    int i = 0;
    do
    {
        c = z_iobuf_read(buf);
        _Z_DEBUG_VA("zint c = 0x%x\n", c);
        r.value.zint = r.value.zint | (((z_zint_t)c & 0x7f) << i);
        _Z_DEBUG_VA("current zint  = %zu\n", r.value.zint);
        i += 7;
    } while (c > 0x7f);
    return r;
}

void z_uint8_array_encode(z_iobuf_t *buf, const z_uint8_array_t *bs)
{
    z_zint_encode(buf, bs->length);
    z_iobuf_write_slice(buf, bs->elem, 0, bs->length);
}

void z_uint8_array_decode_na(z_iobuf_t *buf, z_uint8_array_result_t *r)
{
    r->tag = Z_OK_TAG;
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.uint8_array.length = (unsigned int)r_zint.value.zint;
    r->value.uint8_array.elem = z_iobuf_read_n(buf, r->value.uint8_array.length);
}

z_uint8_array_result_t
z_uint8_array_decode(z_iobuf_t *buf)
{
    z_uint8_array_result_t r;
    z_uint8_array_decode_na(buf, &r);
    return r;
}

void z_string_encode(z_iobuf_t *buf, const char *s)
{
    size_t len = strlen(s);
    z_zint_encode(buf, len);
    // Note that this does not put the string terminator on the wire.
    z_iobuf_write_slice(buf, (uint8_t *)s, 0, len);
}

z_string_result_t
z_string_decode(z_iobuf_t *buf)
{
    z_string_result_t r;
    r.tag = Z_OK_TAG;
    z_zint_result_t vr = z_zint_decode(buf);
    ASSURE_RESULT(vr, r, Z_ZINT_PARSE_ERROR)
    size_t len = vr.value.zint;
    // Allocate space for the string terminator.
    char *s = (char *)malloc(len + 1);
    s[len] = '\0';
    z_iobuf_read_to_n(buf, (uint8_t *)s, len);
    r.value.string = s;
    return r;
}

void z_locators_encode(z_iobuf_t *buf, const z_vec_t *locators)
{
    // Encode the number of locators
    unsigned int len = z_vec_length(locators);
    z_zint_encode(buf, len);
    // Encode the locators
    for (unsigned int i = 0; i < len; ++i)
        z_string_encode(buf, z_vec_get(locators, i));
}

void z_locators_decode_na(z_iobuf_t *buf, z_locators_result_t *r)
{
    r->tag = Z_OK_TAG;
    // Decode the number of locators
    r->value.locators.elem_ = 0;
    z_zint_result_t r_n = z_zint_decode(buf);
    ASSURE_P_RESULT(r_n, r, Z_ZINT_PARSE_ERROR)
    unsigned int len = r_n.value.zint;
    // Decode the locators
    r->value.locators = z_vec_make(len);
    for (unsigned int i = 0; i < len; ++i)
    {
        z_string_result_t r_l = z_string_decode(buf);
        ASSURE_P_RESULT(r_l, r, Z_STRING_PARSE_ERROR)
        z_vec_append(&r->value.locators, r_l.value.string);
    }
}

z_locators_result_t
z_locators_decode(z_iobuf_t *buf)
{
    z_locators_result_t r;
    z_locators_decode_na(buf, &r);
    return r;
}
