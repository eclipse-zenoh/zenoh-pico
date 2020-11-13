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
#include "zenoh-pico/private/codec.h"
#include "zenoh-pico/net/private/codec.h"
#include "zenoh-pico/net/private/codec.h"
#include "zenoh-pico/private/logging.h"

/*------------------ z_zint ------------------*/
int _z_zint_encode(_z_wbuf_t *wbf, z_zint_t v)
{
    while (v > 0x7f)
    {
        uint8_t c = (v & 0x7f) | 0x80;
        _ZN_EC(_z_wbuf_write(wbf, (uint8_t)c))
        v = v >> 7;
    }
    return _z_wbuf_write(wbf, (uint8_t)v);
}

_z_zint_result_t _z_zint_decode(_z_rbuf_t *rbf)
{
    _z_zint_result_t r;
    r.tag = _z_res_t_OK;
    r.value.zint = 0;

    uint8_t c;
    int i = 0;
    do
    {
        c = _z_rbuf_read(rbf);
        _Z_DEBUG_VA("zint c = 0x%x\n", c);
        r.value.zint = r.value.zint | (((z_zint_t)c & 0x7f) << i);
        _Z_DEBUG_VA("current zint  = %zu\n", r.value.zint);
        i += 7;
    } while (c > 0x7f);

    return r;
}

/*------------------ uint8_array ------------------*/
int _z_bytes_encode(_z_wbuf_t *wbf, const z_bytes_t *bs)
{
    _ZN_EC(_z_zint_encode(wbf, bs->len))
    if (wbf->is_expandable && bs->len > ZN_TSID_LENGTH)
    {
        // Do not copy, just add a slice to the expandable buffer
        // Only create a new slice if the malloc is cheaper than copying a
        // large amount of data
        _z_wbuf_add_iosli_from(wbf, bs->val, bs->len);
        return 0;
    }
    else
    {
        return _z_wbuf_write_bytes(wbf, bs->val, 0, bs->len);
    }
}

void _z_bytes_decode_na(_z_rbuf_t *rbf, _z_bytes_result_t *r)
{
    r->tag = _z_res_t_OK;
    _z_zint_result_t r_zint = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT);
    r->value.bytes.len = r_zint.value.zint;
    // Decode without allocating
    r->value.bytes.val = _z_rbuf_get_rptr(rbf);
    // Move the read position
    _z_rbuf_set_rpos(rbf, _z_rbuf_get_rpos(rbf) + r->value.bytes.len);
}

_z_bytes_result_t _z_bytes_decode(_z_rbuf_t *rbf)
{
    _z_bytes_result_t r;
    _z_bytes_decode_na(rbf, &r);
    return r;
}

/*------------------ string with null terminator ------------------*/
int _z_str_encode(_z_wbuf_t *wbf, const z_str_t s)
{
    size_t len = strlen(s);
    _ZN_EC(_z_zint_encode(wbf, len))
    // Note that this does not put the string terminator on the wire.
    return _z_wbuf_write_bytes(wbf, (uint8_t *)s, 0, len);
}

_z_str_result_t _z_str_decode(_z_rbuf_t *rbf)
{
    _z_str_result_t r;
    r.tag = _z_res_t_OK;
    _z_zint_result_t vr = _z_zint_decode(rbf);
    _ASSURE_RESULT(vr, r, _z_err_t_PARSE_ZINT);
    size_t len = vr.value.zint;
    // Allocate space for the string terminator
    z_str_t s = (z_str_t)malloc(len + 1);
    s[len] = '\0';
    _z_rbuf_read_bytes(rbf, (uint8_t *)s, 0, len);
    r.value.str = s;
    return r;
}
