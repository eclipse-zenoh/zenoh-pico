//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

#include "zenoh-pico/protocol/codec.h"

#include "zenoh-pico/utils/logging.h"

/*------------------ period ------------------*/
int8_t _z_period_encode(_z_wbuf_t *buf, const _z_period_t *tp) {
    _Z_EC(_z_zint_encode(buf, tp->origin))
    _Z_EC(_z_zint_encode(buf, tp->period))
    return _z_zint_encode(buf, tp->duration);
}

void _z_period_decode_na(_z_zbuf_t *buf, _z_period_result_t *r) {
    r->_tag = _Z_RES_OK;

    _z_zint_result_t r_origin = _z_zint_decode(buf);
    _ASSURE_P_RESULT(r_origin, r, _Z_ERR_PARSE_ZINT);
    _z_zint_result_t r_period = _z_zint_decode(buf);
    _ASSURE_P_RESULT(r_period, r, _Z_ERR_PARSE_ZINT);
    _z_zint_result_t r_duration = _z_zint_decode(buf);
    _ASSURE_P_RESULT(r_duration, r, _Z_ERR_PARSE_ZINT);

    r->_value.origin = r_origin._value;
    r->_value.period = r_period._value;
    r->_value.duration = r_duration._value;
}

_z_period_result_t _z_period_decode(_z_zbuf_t *buf) {
    _z_period_result_t r;
    _z_period_decode_na(buf, &r);
    return r;
}

/*------------------ uint8 -------------------*/
int8_t _z_uint8_encode(_z_wbuf_t *wbf, uint8_t v) { return _z_wbuf_write(wbf, v); }

_z_uint8_result_t _z_uint8_decode(_z_zbuf_t *zbf) {
    _z_uint8_result_t r;

    if (_z_zbuf_can_read(zbf) == true) {
        r._tag = _Z_RES_OK;
        r._value = _z_zbuf_read(zbf);
    } else {
        r._tag = _Z_ERR_PARSE_UINT8;
        _Z_DEBUG("WARNING: Not enough bytes to read\n");
    }

    return r;
}

/*------------------ z_zint ------------------*/
int8_t _z_zint_encode(_z_wbuf_t *wbf, _z_zint_t v) {
    _z_zint_t lv = v;

    while (lv > 0x7f) {
        uint8_t c = (lv & 0x7f) | 0x80;
        _Z_EC(_z_wbuf_write(wbf, c))
        lv = lv >> (_z_zint_t)7;
    }

    uint8_t c = lv & 0xff;
    return _z_wbuf_write(wbf, c);
}

_z_zint_result_t _z_zint_decode(_z_zbuf_t *zbf) {
    _z_zint_result_t r;
    r._tag = _Z_RES_OK;
    r._value = 0;

    uint8_t i = 0;
    _z_uint8_result_t r_uint8;
    do {
        r_uint8 = _z_uint8_decode(zbf);
        _ASSURE_RESULT(r_uint8, r, _Z_ERR_PARSE_ZINT);

        r._value = r._value | (((_z_zint_t)r_uint8._value & 0x7f) << i);
        i = i + (uint8_t)7;
    } while (r_uint8._value > 0x7f);

    return r;
}

/*------------------ uint8_array ------------------*/
int8_t _z_bytes_encode(_z_wbuf_t *wbf, const _z_bytes_t *bs) {
    int8_t ret = _Z_RES_OK;

    _Z_EC(_z_zint_encode(wbf, bs->len))
    if ((wbf->_is_expandable == true) && (bs->len > Z_TSID_LENGTH)) {
        ret = _z_wbuf_wrap_bytes(wbf, bs->start, 0, bs->len);
    } else {
        ret = _z_wbuf_write_bytes(wbf, bs->start, 0, bs->len);
    }

    return ret;
}

void _z_bytes_decode_na(_z_zbuf_t *zbf, _z_bytes_result_t *r) {
    r->_tag = _Z_RES_OK;
    _z_zint_result_t r_zint = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT);

    if (_z_zbuf_len(zbf) >= r_zint._value) {                              // Check if we have enought bytes to read
        r->_value = _z_bytes_wrap(_z_zbuf_get_rptr(zbf), r_zint._value);  // Decode without allocating
        _z_zbuf_set_rpos(zbf, _z_zbuf_get_rpos(zbf) + r->_value.len);     // Move the read position
    } else {
        r->_tag = _Z_ERR_PARSE_BYTES;
        _Z_DEBUG("WARNING: Not enough bytes to read\n");
    }
}

_z_bytes_result_t _z_bytes_decode(_z_zbuf_t *zbf) {
    _z_bytes_result_t r;
    _z_bytes_decode_na(zbf, &r);
    return r;
}

/*------------------ string with null terminator ------------------*/
int8_t _z_str_encode(_z_wbuf_t *wbf, const char *s) {
    size_t len = strlen(s);
    _Z_EC(_z_zint_encode(wbf, len))
    // Note that this does not put the string terminator on the wire.
    return _z_wbuf_write_bytes(wbf, (const uint8_t *)s, 0, len);
}

_z_str_result_t _z_str_decode(_z_zbuf_t *zbf) {
    _z_str_result_t r;
    r._tag = _Z_RES_OK;
    _z_zint_result_t vr = _z_zint_decode(zbf);
    _ASSURE_RESULT(vr, r, _Z_ERR_PARSE_ZINT);
    size_t len = vr._value;

    if (_z_zbuf_len(zbf) >= len) {                    // Check if we have enough bytes to read
        char *s = (char *)z_malloc(len + (size_t)1);  // Allocate space for the string terminator
        s[len] = '\0';
        _z_zbuf_read_bytes(zbf, (uint8_t *)s, 0, len);
        r._value = s;
    } else {
        r._tag = _Z_ERR_PARSE_STRING;
        _Z_DEBUG("WARNING: Not enough bytes to read\n");
    }

    return r;
}
