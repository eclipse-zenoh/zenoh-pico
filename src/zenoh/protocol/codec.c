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
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <stdio.h>
#include <stdlib.h>
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/protocol/private/codec.h"
#include "zenoh-pico/utils/property.h"

// @TODO: property and properties
// int _zn_property_encode(_z_wbuf_t *buf, const zn_property_t *m)
// {
//     _ZN_EC(_z_zint_encode(buf, m->id))
//     return _z_string_encode(buf, &m->value);
// }

// void _zn_property_decode_na(_z_zbuf_t *buf, _zn_property_result_t *r)
// {
//     _z_zint_result_t r_zint;
//     _z_string_result_t r_str;
//     r->tag = _z_res_t_OK;
//     r_zint = _z_zint_decode(buf);
//     _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT);
//     r_str = _z_string_decode(buf);
//     _ASSURE_P_RESULT(r_str, r, _z_err_t_PARSE_STRING);
//     r->value.property.id = r_zint.value.zint;
//     r->value.property.value = r_str.value.string;
// }

// _zn_property_result_t _zn_property_decode(_z_zbuf_t *buf)
// {
//     _zn_property_result_t r;
//     _zn_property_decode_na(buf, &r);
//     return r;
// }

// int _zn_properties_encode(_z_wbuf_t *buf, const zn_properties_t *ps)
// {
//     zn_property_t *p;
//     size_t len = zn_properties_len(ps);
//     _ZN_EC(_z_zint_encode(buf, len))
//     for (size_t i = 0; i < len; ++i)
//     {
//         p = (zn_property_t *)_z_vec_get(ps, i);
//         _ZN_EC(zn_property_encode(buf, p))
//     }
//     return 0;
// }

/*------------------ period ------------------*/
int _zn_period_encode(_z_wbuf_t *buf, const zn_period_t *tp)
{
    _ZN_EC(_z_zint_encode(buf, tp->origin))
    _ZN_EC(_z_zint_encode(buf, tp->period))
    return _z_zint_encode(buf, tp->duration);
}

void _zn_period_decode_na(_z_zbuf_t *buf, _zn_period_result_t *r)
{
    r->tag = _z_res_t_OK;

    _z_zint_result_t r_origin = _z_zint_decode(buf);
    _ASSURE_P_RESULT(r_origin, r, _z_err_t_PARSE_ZINT);
    _z_zint_result_t r_period = _z_zint_decode(buf);
    _ASSURE_P_RESULT(r_period, r, _z_err_t_PARSE_ZINT);
    _z_zint_result_t r_duration = _z_zint_decode(buf);
    _ASSURE_P_RESULT(r_duration, r, _z_err_t_PARSE_ZINT);

    r->value.period.origin = r_origin.value.zint;
    r->value.period.period = r_period.value.zint;
    r->value.period.duration = r_duration.value.zint;
}

_zn_period_result_t _zn_period_decode(_z_zbuf_t *buf)
{
    _zn_period_result_t r;
    _zn_period_decode_na(buf, &r);
    return r;
}

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
