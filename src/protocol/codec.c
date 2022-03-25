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

#include "zenoh-pico/protocol/codec.h"
#include "zenoh-pico/utils/logging.h"

// @TODO: property and properties
// int _z_property_encode(_z_wbuf_t *buf, const _z_property_t *m)
// {
//     _Z_EC(_z_zint_encode(buf, m->id))
//     return _z_string_encode(buf, &m->value);
// }

// void _z_property_decode_na(_z_zbuf_t *buf, _z_property_result_t *r)
// {
//     _z_zint_result_t r_zint;
//     _z_string_result_t r_str;
//     r->tag = _Z_RES_OK;
//     r_zint = _z_zint_decode(buf);
//     _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT);
//     r_str = _z_string_decode(buf);
//     _ASSURE_P_RESULT(r_str, r, _Z_ERR_PARSE_STRING);
//     r->value.property.id = r_zint.value.zint;
//     r->value.property.value = r_str.value.string;
// }

// _z_property_result_t _z_property_decode(_z_zbuf_t *buf)
// {
//     _z_property_result_t r;
//     _z_property_decode_na(buf, &r);
//     return r;
// }

// int _z_properties_encode(_z_wbuf_t *buf, const _z_properties_t *ps)
// {
//     _z_property_t *p;
//     size_t len = _z_properties_len(ps);
//     _Z_EC(_z_zint_encode(buf, len))
//     for (size_t i = 0; i < len; i++)
//     {
//         p = (_z_property_t *)_z_vec_get(ps, i);
//         _Z_EC(z_property_encode(buf, p))
//     }
//     return 0;
// }

/*------------------ period ------------------*/
int _z_period_encode(_z_wbuf_t *buf, const _z_period_t *tp)
{
    _Z_EC(_z_zint_encode(buf, tp->origin))
    _Z_EC(_z_zint_encode(buf, tp->period))
    return _z_zint_encode(buf, tp->duration);
}

void _z_period_decode_na(_z_zbuf_t *buf, _z_period_result_t *r)
{
    r->tag = _Z_RES_OK;

    _z_zint_result_t r_origin = _z_zint_decode(buf);
    _ASSURE_P_RESULT(r_origin, r, _Z_ERR_PARSE_ZINT);
    _z_zint_result_t r_period = _z_zint_decode(buf);
    _ASSURE_P_RESULT(r_period, r, _Z_ERR_PARSE_ZINT);
    _z_zint_result_t r_duration = _z_zint_decode(buf);
    _ASSURE_P_RESULT(r_duration, r, _Z_ERR_PARSE_ZINT);

    r->value.period.origin = r_origin.value.zint;
    r->value.period.period = r_period.value.zint;
    r->value.period.duration = r_duration.value.zint;
}

_z_period_result_t _z_period_decode(_z_zbuf_t *buf)
{
    _z_period_result_t r;
    _z_period_decode_na(buf, &r);
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
        r.tag = _Z_RES_OK;
        r.value.uint8 = _z_zbuf_read(zbf);
    }
    else
    {
        r.tag = _Z_RES_ERR;
        r.value.error = _Z_ERR_PARSE_UINT8;
        _Z_DEBUG("WARNING: Not enough bytes to read\n");
    }

    return r;
}

/*------------------ z_zint ------------------*/
int _z_zint_encode(_z_wbuf_t *wbf, _z_zint_t v)
{
    while (v > 0x7f)
    {
        uint8_t c = (v & 0x7f) | 0x80;
        _Z_EC(_z_wbuf_write(wbf, (uint8_t)c))
        v = v >> 7;
    }
    return _z_wbuf_write(wbf, (uint8_t)v);
}

_z_zint_result_t _z_zint_decode(_z_zbuf_t *zbf)
{
    _z_zint_result_t r;
    r.tag = _Z_RES_OK;
    r.value.zint = 0;

    int i = 0;
    _z_uint8_result_t r_uint8;
    do
    {
        r_uint8 = _z_uint8_decode(zbf);
        _ASSURE_RESULT(r_uint8, r, _Z_ERR_PARSE_ZINT);

        r.value.zint = r.value.zint | (((_z_zint_t)r_uint8.value.uint8 & 0x7f) << i);
        i += 7;
    } while (r_uint8.value.uint8 > 0x7f);

    return r;
}

/*------------------ uint8_array ------------------*/
int _z_bytes_encode(_z_wbuf_t *wbf, const _z_bytes_t *bs)
{
    _Z_EC(_z_zint_encode(wbf, bs->len))
    if (wbf->is_expandable && bs->len > Z_TSID_LENGTH)
        return _z_wbuf_wrap_bytes(wbf, bs->val, 0, bs->len);
    else
        return _z_wbuf_write_bytes(wbf, bs->val, 0, bs->len);
}

void _z_bytes_decode_na(_z_zbuf_t *zbf, _z_bytes_result_t *r)
{
    r->tag = _Z_RES_OK;
    _z_zint_result_t r_zint = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT);
    // Check if we have enought bytes to read
    if (_z_zbuf_len(zbf) < r_zint.value.zint)
    {
        r->tag = _Z_RES_ERR;
        r->value.error = _Z_ERR_PARSE_BYTES;
        _Z_DEBUG("WARNING: Not enough bytes to read\n");
        return;
    }

    // Decode without allocating
    r->value.bytes = _z_bytes_wrap(_z_zbuf_get_rptr(zbf), r_zint.value.zint);
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
int _z_str_encode(_z_wbuf_t *wbf, const _z_str_t s)
{
    size_t len = strlen(s);
    _Z_EC(_z_zint_encode(wbf, len))
    // Note that this does not put the string terminator on the wire.
    return _z_wbuf_write_bytes(wbf, (uint8_t *)s, 0, len);
}

_z_str_result_t _z_str_decode(_z_zbuf_t *zbf)
{
    _z_str_result_t r;
    r.tag = _Z_RES_OK;
    _z_zint_result_t vr = _z_zint_decode(zbf);
    _ASSURE_RESULT(vr, r, _Z_ERR_PARSE_ZINT);
    size_t len = vr.value.zint;

    // Check if we have enough bytes to read
    if (_z_zbuf_len(zbf) < len)
    {
        r.tag = _Z_RES_ERR;
        r.value.error = _Z_ERR_PARSE_STRING;
        _Z_DEBUG("WARNING: Not enough bytes to read\n");
        return r;
    }

    // Allocate space for the string terminator
    _z_str_t s = (_z_str_t)malloc(len + 1);
    s[len] = '\0';
    _z_zbuf_read_bytes(zbf, (uint8_t *)s, 0, len);
    r.value.str = s;

    return r;
}
