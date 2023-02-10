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

#include "zenoh-pico/protocol/extcodec.h"

#include "zenoh-pico/protocol/codec.h"
#include "zenoh-pico/protocol/ext.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"

int8_t _z_msg_ext_encode_unit(_z_wbuf_t *wbf, const _z_msg_ext_unit_t *ext) {
    int8_t ret = _Z_RES_OK;
    (void)(wbf);
    (void)(ext);
    return ret;
}

int8_t _z_msg_ext_decode_unit(_z_msg_ext_unit_t *ext, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;
    (void)(zbf);
    (void)(ext);
    return ret;
}

int8_t _z_msg_ext_decode_unit_na(_z_msg_ext_unit_t *ext, _z_zbuf_t *zbf) { return _z_msg_ext_decode_unit(ext, zbf); }

int8_t _z_msg_ext_encode_zint(_z_wbuf_t *wbf, const _z_msg_ext_zint_t *ext) {
    int8_t ret = _Z_RES_OK;
    _Z_EC(_z_zint_encode(wbf, ext->_val))
    return ret;
}

int8_t _z_msg_ext_decode_zint(_z_msg_ext_zint_t *ext, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;
    ret |= _z_zint_decode(&ext->_val, zbf);
    return ret;
}

int8_t _z_msg_ext_decode_zint_na(_z_msg_ext_zint_t *ext, _z_zbuf_t *zbf) { return _z_msg_ext_decode_zint(ext, zbf); }

int8_t _z_msg_ext_encode_zbuf(_z_wbuf_t *wbf, const _z_msg_ext_zbuf_t *ext) {
    int8_t ret = _Z_RES_OK;
    _Z_EC(_z_bytes_encode(wbf, &ext->_val))
    return ret;
}

int8_t _z_msg_ext_decode_zbuf(_z_msg_ext_zbuf_t *ext, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;
    ret |= _z_bytes_decode(&ext->_val, zbf);
    return ret;
}

int8_t _z_msg_ext_decode_zbuf_na(_z_msg_ext_zbuf_t *ext, _z_zbuf_t *zbf) { return _z_msg_ext_decode_zbuf(ext, zbf); }

/*------------------ Message Extension ------------------*/
int8_t _z_msg_ext_encode(_z_wbuf_t *wbf, const _z_msg_ext_t *ext) {
    int8_t ret = _Z_RES_OK;

    _Z_EC(_z_wbuf_write(wbf, ext->_header))

    uint8_t enc = _Z_EXT_ENC(ext->_header);
    switch (enc) {
        case _Z_MSG_EXT_ENC_UNIT: {
            _z_msg_ext_encode_unit(wbf, &ext->_body._unit);
        } break;

        case _Z_MSG_EXT_ENC_ZINT: {
            _z_msg_ext_encode_zint(wbf, &ext->_body._zint);
        } break;

        case _Z_MSG_EXT_ENC_ZBUF: {
            _z_msg_ext_encode_zbuf(wbf, &ext->_body._zbuf);
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to copy message extension with unknown encoding(%d)\n", enc);
        } break;
    }

    return ret;
}

int8_t _z_msg_ext_decode(_z_msg_ext_t *ext, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    ret |= _z_uint8_decode(&ext->_header, zbf);  // Decode the header
    if (ret == _Z_RES_OK) {
        uint8_t enc = _Z_EXT_ENC(ext->_header);
        switch (enc) {
            case _Z_MSG_EXT_ENC_UNIT: {
                ret |= _z_msg_ext_decode_unit(&ext->_body._unit, zbf);
            } break;

            case _Z_MSG_EXT_ENC_ZINT: {
                ret |= _z_msg_ext_decode_zint(&ext->_body._zint, zbf);
            } break;

            case _Z_MSG_EXT_ENC_ZBUF: {
                ret |= _z_msg_ext_decode_zbuf(&ext->_body._zbuf, zbf);
            } break;

            default: {
                _Z_DEBUG("WARNING: Trying to copy message extension with unknown encoding(%d)\n", enc);
            } break;
        }
    }

    return ret;
}

int8_t _z_msg_ext_decode_na(_z_msg_ext_t *ext, _z_zbuf_t *zbf) { return _z_msg_ext_decode(ext, zbf); }
