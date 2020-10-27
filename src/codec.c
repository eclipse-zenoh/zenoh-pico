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
#include <stdio.h>
#include "zenoh/codec.h"
#include "zenoh/net/codec.h"
#include "zenoh/net/private/codec.h"
#include "zenoh/private/logging.h"

/*------------------ z_zint ------------------*/
int z_zint_encode(z_wbuf_t *wbf, z_zint_t v)
{
    while (v > 0x7f)
    {
        uint8_t c = (v & 0x7f) | 0x80;
        _ZN_EC(z_wbuf_write(wbf, (uint8_t)c))
        v = v >> 7;
    }
    return z_wbuf_write(wbf, (uint8_t)v);
}

z_zint_result_t z_zint_decode(z_rbuf_t *rbf)
{
    z_zint_result_t r;
    r.tag = Z_OK_TAG;
    r.value.zint = 0;

    uint8_t c;
    int i = 0;
    do
    {
        c = z_rbuf_read(rbf);
        _Z_DEBUG_VA("zint c = 0x%x\n", c);
        r.value.zint = r.value.zint | (((z_zint_t)c & 0x7f) << i);
        _Z_DEBUG_VA("current zint  = %zu\n", r.value.zint);
        i += 7;
    } while (c > 0x7f);

    return r;
}

/*------------------ uint8_array ------------------*/
int z_uint8_array_encode(z_wbuf_t *wbf, const z_uint8_array_t *bs)
{
    _ZN_EC(z_zint_encode(wbf, bs->length))
    if (wbf->is_expandable && bs->length > ZENOH_NET_TSID_LENGTH)
    {
        // Do not copy, just add a slice to the expandable buffer
        // Only create a new slice if the malloc is cheaper than copying a
        // large amount of data
        z_iosli_t sios = z_iosli_wrap(bs->elem, bs->length, 0, bs->length);
        z_iosli_t *pios = (z_iosli_t *)malloc(sizeof(z_iosli_t));
        memcpy(pios, &sios, sizeof(z_iosli_t));
        z_wbuf_add_iosli(wbf, pios);
        return 0;
    }
    else
    {
        return z_wbuf_write_bytes(wbf, bs->elem, 0, bs->length);
    }
}

void z_uint8_array_decode_na(z_rbuf_t *rbf, z_uint8_array_result_t *r)
{
    r->tag = Z_OK_TAG;
    z_zint_result_t r_zint = z_zint_decode(rbf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.uint8_array.length = r_zint.value.zint;
    // Decode without allocating
    r->value.uint8_array.elem = z_rbuf_get_rptr(rbf);
    // Move the read position
    z_rbuf_set_rpos(rbf, z_rbuf_get_rpos(rbf) + r->value.uint8_array.length);
}

z_uint8_array_result_t z_uint8_array_decode(z_rbuf_t *rbf)
{
    z_uint8_array_result_t r;
    z_uint8_array_decode_na(rbf, &r);
    return r;
}

/*------------------ string ------------------*/
int z_string_encode(z_wbuf_t *wbf, const char *s)
{
    size_t len = strlen(s);
    _ZN_EC(z_zint_encode(wbf, len))
    // Note that this does not put the string terminator on the wire.
    return z_wbuf_write_bytes(wbf, (uint8_t *)s, 0, len);
}

z_string_result_t z_string_decode(z_rbuf_t *rbf)
{
    z_string_result_t r;
    r.tag = Z_OK_TAG;
    z_zint_result_t vr = z_zint_decode(rbf);
    ASSURE_RESULT(vr, r, Z_ZINT_PARSE_ERROR)
    size_t len = vr.value.zint;
    // Allocate space for the string terminator
    char *s = (char *)malloc(len + 1);
    s[len] = '\0';
    z_rbuf_read_bytes(rbf, (uint8_t *)s, 0, len);
    r.value.string = s;
    return r;
}
