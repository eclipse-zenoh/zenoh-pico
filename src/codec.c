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

/*------------------ uint8 -------------------*/
int _z_uint8_encode(_z_wbuf_t *wbf, uint8_t v)
{
    return _z_wbuf_write(wbf, v);
}

_z_uint8_result_t _z_uint8_decode(_z_zbuf_t *zbf)
{
    _z_uint8_result_t r;

    if (_z_zbuf_can_read(zbf))
    {
        r.tag = _z_res_t_OK;
        r.value.uint8 = _z_zbuf_read(zbf);
    }
    else
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _z_err_t_PARSE_UINT8;
        _Z_ERROR("WARNING: Not enough bytes to read\n");
    }

    return r;
}

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

_z_zint_result_t _z_zint_decode(_z_zbuf_t *zbf)
{
    _z_zint_result_t r;
    r.tag = _z_res_t_OK;
    r.value.zint = 0;

    int i = 0;
    _z_uint8_result_t r_uint8;
    do
    {
        r_uint8 = _z_uint8_decode(zbf);
        _ASSURE_RESULT(r_uint8, r, _z_err_t_PARSE_ZINT);

        _Z_DEBUG_VA("zint c = 0x%x\n", c);
        r.value.zint = r.value.zint | (((z_zint_t)r_uint8.value.uint8 & 0x7f) << i);
        _Z_DEBUG_VA("current zint  = %zu\n", r.value.zint);
        i += 7;
    } while (r_uint8.value.uint8 > 0x7f);

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

void _z_bytes_decode_na(_z_zbuf_t *zbf, _z_bytes_result_t *r)
{
    r->tag = _z_res_t_OK;
    _z_zint_result_t r_zint = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT);
    r->value.bytes.len = r_zint.value.zint;
    // Check if we have enought bytes to read
    if (_z_zbuf_len(zbf) < r->value.bytes.len)
    {
        r->tag = _z_res_t_ERR;
        r->value.error = _z_err_t_PARSE_BYTES;
        _Z_ERROR("WARNING: Not enough bytes to read\n");
        return;
    }

    // Decode without allocating
    r->value.bytes.val = _z_zbuf_get_rptr(zbf);
    // Move the read position
    _z_zbuf_set_rpos(zbf, _z_zbuf_get_rpos(zbf) + r->value.bytes.len);
}

_z_bytes_result_t _z_bytes_decode(_z_zbuf_t *zbf)
{
    _z_bytes_result_t r;
    _z_bytes_decode_na(zbf, &r);
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

_z_str_result_t _z_str_decode(_z_zbuf_t *zbf)
{
    _z_str_result_t r;
    r.tag = _z_res_t_OK;
    _z_zint_result_t vr = _z_zint_decode(zbf);
    _ASSURE_RESULT(vr, r, _z_err_t_PARSE_ZINT);
    size_t len = vr.value.zint;
    // Check if we have enough bytes to read
    if (_z_zbuf_len(zbf) < len)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _z_err_t_PARSE_STRING;
        _Z_ERROR("WARNING: Not enough bytes to read\n");
        return r;
    }
    // Allocate space for the string terminator
    z_str_t s = (z_str_t)malloc(len + 1);
    s[len] = '\0';
    _z_zbuf_read_bytes(zbf, (uint8_t *)s, 0, len);
    r.value.str = s;
    return r;
}
