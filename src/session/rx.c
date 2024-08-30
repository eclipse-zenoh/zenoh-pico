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
#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/declarations.h"
#include "zenoh-pico/protocol/definitions/message.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/interest.h"
#include "zenoh-pico/session/push.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/reply.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/logging.h"

/*------------------ Handle message ------------------*/
int8_t _z_handle_network_message(_z_session_rc_t *zsrc, _z_zenoh_message_t *msg, uint16_t local_peer_id) {
    int8_t ret = _Z_RES_OK;
    _z_session_t *zn = _Z_RC_IN_VAL(zsrc);

    switch (msg->_tag) {
        case _Z_N_DECLARE: {
            _Z_DEBUG("Handling _Z_N_DECLARE");
            _z_n_msg_declare_t *decl = &msg->_body._declare;
            switch (decl->_decl._tag) {
                case _Z_DECL_KEXPR: {
                    if (_z_register_resource(zn, decl->_decl._body._decl_kexpr._keyexpr,
                                             decl->_decl._body._decl_kexpr._id, local_peer_id) == 0) {
                        ret = _Z_ERR_ENTITY_DECLARATION_FAILED;
                    }
                } break;
                case _Z_UNDECL_KEXPR: {
                    _z_unregister_resource(zn, decl->_decl._body._undecl_kexpr._id, local_peer_id);
                } break;
                case _Z_DECL_SUBSCRIBER: {
                    _z_interest_process_declares(zn, &decl->_decl);
                } break;
                case _Z_DECL_QUERYABLE: {
                    _z_interest_process_declares(zn, &decl->_decl);
                } break;
                case _Z_UNDECL_SUBSCRIBER: {
                    _z_interest_process_undeclares(zn, &decl->_decl);
                } break;
                case _Z_UNDECL_QUERYABLE: {
                    _z_interest_process_undeclares(zn, &decl->_decl);
                } break;
                case _Z_DECL_TOKEN: {
                    // TODO: add support or explicitly discard
                } break;
                case _Z_UNDECL_TOKEN: {
                    // TODO: add support or explicitly discard
                } break;
                case _Z_DECL_FINAL: {
                    // Check that interest id is valid
                    if (!decl->has_interest_id) {
                        return _Z_ERR_MESSAGE_ZENOH_DECLARATION_UNKNOWN;
                    }
                    _z_interest_process_declare_final(zn, decl->_interest_id);
                } break;
            }
        } break;
        case _Z_N_PUSH: {
            _Z_DEBUG("Handling _Z_N_PUSH");
            _z_n_msg_push_t *push = &msg->_body._push;
            ret = _z_trigger_push(zn, push);
        } break;
        case _Z_N_REQUEST: {
            _Z_DEBUG("Handling _Z_N_REQUEST");
            _z_n_msg_request_t *req = &msg->_body._request;
            switch (req->_tag) {
                case _Z_REQUEST_QUERY: {
#if Z_FEATURE_QUERYABLE == 1
                    _z_msg_query_t *query = &req->_body._query;
                    ret = _z_trigger_queryables(zsrc, query, req->_key, (uint32_t)req->_rid,
                                                req->_body._query._ext_attachment);
#else
                    _Z_DEBUG("_Z_REQUEST_QUERY dropped, queryables not supported");
#endif
                } break;
                case _Z_REQUEST_PUT: {
#if Z_FEATURE_SUBSCRIPTION == 1
                    _z_msg_put_t put = req->_body._put;
                    ret = _z_trigger_subscriptions(zn, req->_key, put._payload, &put._encoding, Z_SAMPLE_KIND_PUT,
                                                   &put._commons._timestamp, req->_ext_qos, put._attachment);
#endif
                    if (ret == _Z_RES_OK) {
                        _z_network_message_t final = _z_n_msg_make_response_final(req->_rid);
                        ret |= _z_send_n_msg(zn, &final, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);
                    }
                } break;
                case _Z_REQUEST_DEL: {
#if Z_FEATURE_SUBSCRIPTION == 1
                    _z_msg_del_t del = req->_body._del;
                    _z_encoding_t encoding = _z_encoding_null();
                    ret = _z_trigger_subscriptions(zn, req->_key, _z_bytes_null(), &encoding, Z_SAMPLE_KIND_DELETE,
                                                   &del._commons._timestamp, req->_ext_qos, del._attachment);
#endif
                    if (ret == _Z_RES_OK) {
                        _z_network_message_t final = _z_n_msg_make_response_final(req->_rid);
                        ret |= _z_send_n_msg(zn, &final, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);
                    }
                } break;
            }
        } break;
        case _Z_N_RESPONSE: {
            _Z_DEBUG("Handling _Z_N_RESPONSE");
            _z_n_msg_response_t *response = &msg->_body._response;
            switch (response->_tag) {
                case _Z_RESPONSE_BODY_REPLY: {
                    _z_msg_reply_t *reply = &response->_body._reply;
                    ret = _z_trigger_reply_partial(zn, response->_request_id, response->_key, reply);
                } break;
                case _Z_RESPONSE_BODY_ERR: {
                    _z_msg_err_t *error = &response->_body._err;
                    ret = _z_trigger_reply_err(zn, response->_request_id, error);
                } break;
            }
        } break;
        case _Z_N_RESPONSE_FINAL: {
            _Z_DEBUG("Handling _Z_N_RESPONSE_FINAL");
            ret = _z_trigger_reply_final(zn, &msg->_body._response_final);
        } break;

        case _Z_N_INTEREST: {
            _Z_DEBUG("Handling _Z_N_INTEREST");
            _z_n_msg_interest_t *interest = &msg->_body._interest;

            _Bool not_final = ((interest->_interest.flags & _Z_INTEREST_NOT_FINAL_MASK) != 0);
            if (not_final) {
                _z_interest_process_interest(zn, interest->_interest._keyexpr, interest->_interest._id,
                                             interest->_interest.flags);
            } else {
                _z_interest_process_interest_final(zn, interest->_interest._id);
            }
        }
    }
    _z_msg_clear(msg);
    return ret;
}
