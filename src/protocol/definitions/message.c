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

#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/network.h"

void _z_msg_reply_clear(_z_msg_reply_t *msg) { _z_push_body_clear(&msg->_body); }

void _z_msg_put_clear(_z_msg_put_t *msg) {
    _z_encoding_clear(&msg->_encoding);
    _z_bytes_clear(&msg->_payload);
    _z_timestamp_clear(&msg->_commons._timestamp);
}

_z_msg_query_reqexts_t _z_msg_query_required_extensions(const _z_msg_query_t *msg) {
#if Z_FEATURE_ATTACHMENT == 1
    z_attachment_t att = _z_encoded_as_attachment(&msg->_ext_attachment);
#endif
    return (_z_msg_query_reqexts_t) {
        .body = msg->_ext_value.payload.start != NULL || _z_encoding_check(&msg->_ext_value.encoding),
        .info = _z_id_check(msg->_ext_info._id) || msg->_ext_info._entity_id != 0 || msg->_ext_info._source_sn != 0,
#if Z_FEATURE_ATTACHMENT == 1
        .attachment = z_attachment_check(&att)
#else
        .attachment = false
#endif
    };
}

void _z_msg_query_clear(_z_msg_query_t *msg) {
    _z_bytes_clear(&msg->_parameters);
    _z_value_clear(&msg->_ext_value);
}
void _z_msg_err_clear(_z_msg_err_t *err) {
    _z_encoding_clear(&err->encoding);
    _z_bytes_clear(&err->_payload);
}
