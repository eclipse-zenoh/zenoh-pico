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

#include "zenoh-pico/protocol/ext.h"

#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/utils/logging.h"

_z_msg_ext_t _z_msg_ext_make_unit(uint8_t id) {
    _z_msg_ext_t ext;

    ext._header = 0;
    ext._header |= (id & 0x1F);
    ext._header |= (_Z_MSG_EXT_ENC_UNIT & 0x03) << 5;

    return ext;
}

void _z_msg_ext_clear_unit(_z_msg_ext_unit_t *ext) { (void)(ext); }

void _z_msg_ext_copy_unit(_z_msg_ext_unit_t *clone, _z_msg_ext_unit_t *ext) {
    (void)(clone);
    (void)(ext);
}

_z_msg_ext_t _z_msg_ext_make_zint(uint8_t id, _z_zint_t zid) {
    _z_msg_ext_t ext;

    ext._header = 0;
    ext._header |= (id & 0x1F);
    ext._header |= (_Z_MSG_EXT_ENC_ZINT & 0x03) << 5;

    ext._body._zint._val = zid;

    return ext;
}

void _z_msg_ext_clear_zint(_z_msg_ext_zint_t *ext) { (void)(ext); }

void _z_msg_ext_copy_zint(_z_msg_ext_zint_t *clone, _z_msg_ext_zint_t *ext) { clone->_val = ext->_val; }

_z_msg_ext_t _z_msg_ext_make_zbuf(uint8_t id, const _z_bytes_t zbuf) {
    _z_msg_ext_t ext;

    ext._header = 0;
    ext._header |= (id & 0x1F);
    ext._header |= (_Z_MSG_EXT_ENC_ZBUF & 0x03) << 5;

    ext._body._zbuf._val = _z_bytes_wrap(zbuf.start, zbuf.len);

    return ext;
}

void _z_msg_ext_clear_zbuf(_z_msg_ext_zbuf_t *ext) { _z_bytes_clear(&ext->_val); }

void _z_msg_ext_copy_zbuf(_z_msg_ext_zbuf_t *clone, _z_msg_ext_zbuf_t *ext) { _z_bytes_copy(&clone->_val, &ext->_val); }

void _z_msg_ext_copy(_z_msg_ext_t *clone, _z_msg_ext_t *ext) {
    clone->_header = ext->_header;

    switch (_Z_EXT_ENC(clone->_header)) {
        case _Z_MSG_EXT_ENC_UNIT: {
            _z_msg_ext_copy_unit(&clone->_body._unit, &ext->_body._unit);
        } break;

        case _Z_MSG_EXT_ENC_ZINT: {
            _z_msg_ext_copy_zint(&clone->_body._zint, &ext->_body._zint);
        } break;

        case _Z_MSG_EXT_ENC_ZBUF: {
            _z_msg_ext_copy_zbuf(&clone->_body._zbuf, &ext->_body._zbuf);
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to copy message extension with unknown ID(%d)\n", mid);
        } break;
    }
}

void _z_msg_ext_clear(_z_msg_ext_t *ext) {
    switch (_Z_EXT_ENC(ext->_header)) {
        case _Z_MSG_EXT_ENC_UNIT: {
            _z_msg_ext_clear_unit(&ext->_body._unit);
        } break;

        case _Z_MSG_EXT_ENC_ZINT: {
            _z_msg_ext_clear_zint(&ext->_body._zint);
        } break;

        case _Z_MSG_EXT_ENC_ZBUF: {
            _z_msg_ext_clear_zbuf(&ext->_body._zbuf);
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to free message extension with unknown ID(%d)\n", mid);
        } break;
    }
}
