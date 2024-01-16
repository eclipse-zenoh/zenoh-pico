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

#include "zenoh-pico/session/reply.h"

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/utils/logging.h"

int8_t _z_trigger_reply_partial(_z_session_t *zn, _z_zint_t id, _z_keyexpr_t key, _z_msg_reply_t *reply) {
    int8_t ret = _Z_RES_OK;

    // TODO check id to know where to dispatch

#if Z_FEATURE_QUERY == 1
    ret = _z_trigger_query_reply_partial(zn, id, key, reply->_value.payload, reply->_value.encoding, Z_SAMPLE_KIND_PUT,
                                         reply->_timestamp);
#endif
    return ret;
}

int8_t _z_trigger_reply_final(_z_session_t *zn, _z_n_msg_response_final_t *final) {
    int8_t ret = _Z_RES_OK;

    // TODO check id to know where to dispatch
    _z_zint_t id = final->_request_id;

#if Z_FEATURE_QUERY == 1
    _z_trigger_query_reply_final(zn, id);
#endif
    return ret;
}
