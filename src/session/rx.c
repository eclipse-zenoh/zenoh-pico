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
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/declarations.h"
#include "zenoh-pico/protocol/definitions/message.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/logging.h"

/*------------------ Handle message ------------------*/
int8_t _z_handle_network_message(_z_session_t *zn, _z_zenoh_message_t *msg, uint16_t local_peer_id) {
    int8_t ret = _Z_RES_OK;

    switch (msg->_tag) {
        case _Z_N_DECLARE: {
            _Z_DEBUG("Handling _Z_N_DECLARE\n");
            _z_n_msg_declare_t decl = msg->_body._declare;
            switch (decl._decl._tag) {
                case _Z_DECL_KEXPR: {
                    if (_z_register_resource(zn, decl._decl._body._decl_kexpr._keyexpr,
                                             decl._decl._body._decl_kexpr._id, local_peer_id) == 0) {
                        ret = _Z_ERR_ENTITY_DECLARATION_FAILED;
                    }
                } break;
                case _Z_UNDECL_KEXPR: {
                    _z_unregister_resource(zn, decl._decl._body._undecl_kexpr._id, local_peer_id);
                } break;
                case _Z_DECL_SUBSCRIBER: {
                    // TODO: add support or explicitly discard
                } break;
                case _Z_UNDECL_SUBSCRIBER: {
                    // TODO: add support or explicitly discard
                } break;
                case _Z_DECL_QUERYABLE: {
                    // TODO: add support or explicitly discard
                } break;
                case _Z_UNDECL_QUERYABLE: {
                    // TODO: add support or explicitly discard
                } break;
                case _Z_DECL_TOKEN: {
                    // TODO: add support or explicitly discard
                } break;
                case _Z_UNDECL_TOKEN: {
                    // TODO: add support or explicitly discard
                } break;
                case _Z_DECL_INTEREST: {
                    // TODO: add support or explicitly discard
                } break;
                case _Z_FINAL_INTEREST: {
                    // TODO: add support or explicitly discard
                } break;
                case _Z_UNDECL_INTEREST: {
                    // TODO: add support or explicitly discard
                } break;
            }
        } break;
        case _Z_N_PUSH: {
            _Z_DEBUG("Handling _Z_N_PUSH\n");
            _z_n_msg_push_t push = msg->_body._push;
            _z_bytes_t payload = push._body._is_put ? push._body._body._put._payload : _z_bytes_empty();
            _z_encoding_t encoding = push._body._is_put ? push._body._body._put._encoding : z_encoding_default();
            int kind = push._body._is_put ? Z_SAMPLE_KIND_PUT : Z_SAMPLE_KIND_DELETE;
            ret = _z_trigger_subscriptions(zn, push._key, payload, encoding, kind, push._timestamp);
        } break;
        case _Z_N_REQUEST: {
            _Z_DEBUG("Handling _Z_N_REQUEST\n");
            _z_n_msg_request_t req = msg->_body._request;
            switch (req._tag) {
                case _Z_REQUEST_QUERY: {
                    _z_msg_query_t *query = &req._body._query;
                    ret = _z_trigger_queryables(zn, query, req._key, req._rid);
                } break;
                case _Z_REQUEST_PUT: {
                    _z_msg_put_t put = req._body._put;
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
                    _z_msg_del_t del = req._body._del;
                    ret = _z_trigger_subscriptions(zn, req._key, _z_bytes_empty(), z_encoding_default(),
                                                   Z_SAMPLE_KIND_DELETE, del._commons._timestamp);
                    if (ret == _Z_RES_OK) {
                        _z_network_message_t ack = _z_n_msg_make_ack(req._rid, &req._key);
                        ret = _z_send_n_msg(zn, &ack, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);
                        _z_network_message_t final = _z_n_msg_make_response_final(req._rid);
                        ret |= _z_send_n_msg(zn, &final, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);
                    }
                } break;
                case _Z_REQUEST_PULL: {
                    // @TODO: define behaviour
                } break;
            }
        } break;
        case _Z_N_RESPONSE: {
            _Z_DEBUG("Handling _Z_N_RESPONSE\n");
            _z_n_msg_response_t response = msg->_body._response;
            switch (response._tag) {
                case _Z_RESPONSE_BODY_REPLY: {
                    _z_msg_reply_t reply = response._body._reply;
                    ret = _z_trigger_query_reply_partial(zn, response._request_id, response._key, reply._value.payload,
                                                         reply._value.encoding, Z_SAMPLE_KIND_PUT, reply._timestamp);
                } break;
                case _Z_RESPONSE_BODY_ERR: {
                    // @TODO: expose errors to the user
                    _z_msg_err_t error = response._body._err;
                    _z_bytes_t payload = error._ext_value.payload;
                    _Z_ERROR("Received Err for query %zu: code=%d, message=%.*s\n", response._request_id, error._code,
                             (int)payload.len, payload.start);
                } break;
                case _Z_RESPONSE_BODY_ACK: {
                    // @TODO: implement ACKs for puts/dels
                } break;
                case _Z_RESPONSE_BODY_PUT: {
                    _z_msg_put_t put = response._body._put;
                    ret = _z_trigger_subscriptions(zn, response._key, put._payload, put._encoding, Z_SAMPLE_KIND_PUT,
                                                   put._commons._timestamp);
                } break;
                case _Z_RESPONSE_BODY_DEL: {
                    _z_msg_del_t del = response._body._del;
                    ret = _z_trigger_subscriptions(zn, response._key, _z_bytes_empty(), z_encoding_default(),
                                                   Z_SAMPLE_KIND_DELETE, del._commons._timestamp);
                } break;
            }
        } break;
        case _Z_N_RESPONSE_FINAL: {
            _Z_DEBUG("Handling _Z_N_RESPONSE_FINAL\n");
            _z_zint_t id = msg->_body._response_final._request_id;
            _z_trigger_query_reply_final(zn, id);
        } break;
    }
    _z_msg_clear(msg);
    return ret;
}
