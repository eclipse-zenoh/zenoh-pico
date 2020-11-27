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
#include "zenoh-pico/private/logging.h"
#include "zenoh-pico/private/codec.h"
#include "zenoh-pico/net/private/codec.h"
#include "zenoh-pico/net/private/codec.h"
#include "zenoh-pico/net/property.h"

// @TODO: property and properties
// int _zn_property_encode(_z_wbuf_t *buf, const zn_property_t *m)
// {
//     _ZN_EC(_z_zint_encode(buf, m->id))
//     return _z_string_encode(buf, &m->value);
// }

// void _zn_property_decode_na(_z_rbuf_t *buf, _zn_property_result_t *r)
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

// _zn_property_result_t _zn_property_decode(_z_rbuf_t *buf)
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

int _zn_period_encode(_z_wbuf_t *buf, const zn_period_t *tp)
{
    _ZN_EC(_z_zint_encode(buf, tp->origin))
    _ZN_EC(_z_zint_encode(buf, tp->period))
    return _z_zint_encode(buf, tp->duration);
}

void _zn_period_decode_na(_z_rbuf_t *buf, _zn_period_result_t *r)
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

_zn_period_result_t _zn_period_decode(_z_rbuf_t *buf)
{
    _zn_period_result_t r;
    _zn_period_decode_na(buf, &r);
    return r;
}
