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

#include <stdint.h>

#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/utils/logging.h"

_z_msg_ext_t _z_msg_ext_make_unit(uint8_t id) {
    _z_msg_ext_t ext;

    ext._header = id & _Z_EXT_ID_MASK;
    ext._header |= _Z_MSG_EXT_ENC_UNIT;

    return ext;
}

_z_msg_ext_t _z_msg_ext_make_zint(uint8_t id, _z_zint_t zid) {
    _z_msg_ext_t ext;

    ext._header = id & _Z_EXT_ID_MASK;
    ext._header |= _Z_MSG_EXT_ENC_ZINT;

    ext._body._zint._val = zid;

    return ext;
}

_z_msg_ext_t _z_msg_ext_make_zbuf(uint8_t id, const _z_slice_t *zbuf) {
    _z_msg_ext_t ext;

    ext._header = id & _Z_EXT_ID_MASK;
    ext._header |= _Z_MSG_EXT_ENC_ZBUF;

    ext._body._zbuf._val = _z_slice_view_from_slice(zbuf);

    return ext;
}
