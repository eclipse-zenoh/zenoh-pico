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

#include "zenoh-pico/session/push.h"

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/utils/logging.h"

int8_t _z_trigger_push(_z_session_t *zn, _z_n_msg_push_t *push) {
    int8_t ret = _Z_RES_OK;

    // TODO check body to know where to dispatch
    _z_bytes_t payload = push->_body._is_put ? push->_body._body._put._payload : _z_bytes_empty();
    _z_encoding_t encoding = push->_body._is_put ? push->_body._body._put._encoding : _z_encoding_null();
    int kind = push->_body._is_put ? Z_SAMPLE_KIND_PUT : Z_SAMPLE_KIND_DELETE;
#if Z_FEATURE_SUBSCRIPTION == 1
#if Z_FEATURE_ATTACHMENT == 1
    z_attachment_t att = _z_encoded_as_attachment(&push->_body._body._put._attachment);
#else
    z_attachment_t att = z_attachment_null();
#endif
    ret = _z_trigger_subscriptions(zn, push->_key, payload, encoding, kind, push->_timestamp, push->_qos, att);
#endif
    return ret;
}
