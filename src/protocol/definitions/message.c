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

#include "zenoh-pico/protocol/definitions/message.h"

#include <stdbool.h>

#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/network.h"

_z_msg_query_reqexts_t _z_msg_query_required_extensions(const _z_msg_query_t *msg) {
    const _z_value_t *ref = _z_value_view_deref(&msg->_ext_value);
    return (_z_msg_query_reqexts_t){
        .body = _z_bytes_check(&ref->payload) || _z_encoding_check(&ref->encoding),
        .info = _z_id_check(msg->_ext_info._source_id.zid) || msg->_ext_info._source_id.eid != 0 ||
                msg->_ext_info._source_sn != 0,
        .attachment = _z_bytes_view_check(&msg->_ext_attachment),
    };
}
