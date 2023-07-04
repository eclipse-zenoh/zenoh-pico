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

#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/protocol/definitions/message.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/protocol/msgcodec.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/logging.h"

/*------------------ Handle message ------------------*/
int8_t _z_handle_zenoh_message(_z_session_t *zn, _z_zenoh_message_t *msg) {
    int8_t ret = _Z_RES_OK;
    switch (msg->_tag) {
        case _Z_N_DECLARE: {
        } break;
        case _Z_N_PUSH: {
            _z_n_msg_push_t push = msg->_body._push;
            _z_bytes_t payload = push._body._is_put ? push._body._body._put._payload : _z_bytes_empty();
            _z_encoding_t encoding = push._body._is_put ? push._body._body._put._encoding : z_encoding_default();
            int kind = push._body._is_put ? Z_SAMPLE_KIND_PUT : Z_SAMPLE_KIND_DELETE;
            ret = _z_trigger_subscriptions(zn, push._key, payload, encoding, kind, push._timestamp);
        } break;
        case _Z_N_REQUEST: {
            _z_n_msg_request_t req = msg->_body._request;
            switch (req._tag) {
                case _Z_REQUEST_QUERY: {
                    _z_msg_query_t *query = &req._body._query;
                    ret = _z_trigger_queryables(zn, query, req._key, req._rid);
                } break;
                case _Z_REQUEST_PUT: {
                    _z_msg_put_t put = req._body.put;
                    ret = _z_trigger_subscriptions(zn, req._key, put._payload, put._encoding, Z_SAMPLE_KIND_PUT,
                                                   put._commons._timestamp);
                    if (ret == _Z_RES_OK) {
                        _z_network_message_t ack = _z_n_msg_make_ack(req._rid, &req._key);
                        ret = _z_send_n_msg(zn, &ack, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);
                        _z_network_message_t final = _z_n_msg_make_response_final(req._rid);
                        ret |= _z_send_n_msg(zn, &final, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);
                    }
                } break;
                case _Z_REQUEST_DEL: {
                    _z_msg_del_t del = req._body.del;
                    int8_t result = _z_trigger_subscriptions(zn, req._key, _z_bytes_empty(), z_encoding_default(),
                                                             Z_SAMPLE_KIND_PUT, del._commons._timestamp);
                    if (ret == _Z_RES_OK) {
                        _z_network_message_t ack = _z_n_msg_make_ack(req._rid, &req._key);
                        ret = _z_send_n_msg(zn, &ack, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);
                        _z_network_message_t final = _z_n_msg_make_response_final(req._rid);
                        ret |= _z_send_n_msg(zn, &final, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);
                    }
                } break;
                case _Z_REQUEST_PULL: {  // TODO
                } break;
            }
        } break;
        case _Z_N_RESPONSE: {
        } break;
        case _Z_N_RESPONSE_FINAL: {
        } break;
    }
    _z_msg_clear(msg);
    return ret;
}
