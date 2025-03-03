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
#include "zenoh-pico/session/liveliness.h"
#include "zenoh-pico/session/push.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/reply.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/utils/logging.h"

/*------------------ Handle message ------------------*/

static z_result_t _z_handle_declare_inner(_z_session_t *zn, _z_n_msg_declare_t *decl, uint16_t local_peer_id) {
    switch (decl->_decl._tag) {
        case _Z_DECL_KEXPR:
            if (_z_register_resource(zn, &decl->_decl._body._decl_kexpr._keyexpr, decl->_decl._body._decl_kexpr._id,
                                     local_peer_id) == 0) {
                return _Z_ERR_ENTITY_DECLARATION_FAILED;
            }
            break;

        case _Z_UNDECL_KEXPR:
            _z_unregister_resource(zn, decl->_decl._body._undecl_kexpr._id, local_peer_id);
            break;

        case _Z_DECL_SUBSCRIBER:
            return _z_interest_process_declares(zn, &decl->_decl);

        case _Z_DECL_QUERYABLE:
            return _z_interest_process_declares(zn, &decl->_decl);

        case _Z_DECL_TOKEN:
            _Z_RETURN_IF_ERR(_z_liveliness_process_token_declare(zn, decl));
            return _z_interest_process_declares(zn, &decl->_decl);

        case _Z_UNDECL_SUBSCRIBER:
            return _z_interest_process_undeclares(zn, &decl->_decl);

        case _Z_UNDECL_QUERYABLE:
            return _z_interest_process_undeclares(zn, &decl->_decl);

        case _Z_UNDECL_TOKEN:
            _Z_RETURN_IF_ERR(_z_liveliness_process_token_undeclare(zn, decl));
            return _z_interest_process_undeclares(zn, &decl->_decl);

        case _Z_DECL_FINAL:
            _Z_RETURN_IF_ERR(_z_liveliness_process_declare_final(zn, decl));
            // Check that interest id is valid
            if (!decl->has_interest_id) {
                return _Z_ERR_MESSAGE_ZENOH_DECLARATION_UNKNOWN;
            }
            return _z_interest_process_declare_final(zn, decl->_interest_id);

        default:
            _Z_INFO("Received unknown declare tag: %d\n", decl->_decl._tag);
            break;
    }
    return _Z_RES_OK;
}

static z_result_t _z_handle_declare(_z_session_t *zn, _z_n_msg_declare_t *decl, uint16_t local_peer_id) {
    z_result_t ret = _z_handle_declare_inner(zn, decl, local_peer_id);
    _z_n_msg_declare_clear(decl);
    return ret;
}

static z_result_t _z_handle_request(_z_session_rc_t *zsrc, _z_session_t *zn, _z_n_msg_request_t *req,
                                    z_reliability_t reliability) {
    switch (req->_tag) {
        case _Z_REQUEST_QUERY:
#if Z_FEATURE_QUERYABLE == 1
            // Memory cleaning must be done in the feature layer
            return _z_trigger_queryables(zsrc, &req->_body._query, &req->_key, (uint32_t)req->_rid);
#else
            _Z_DEBUG("_Z_REQUEST_QUERY dropped, queryables not supported");
            _z_n_msg_request_clear(req);
            break;
#endif

        case _Z_REQUEST_PUT: {
#if Z_FEATURE_SUBSCRIPTION == 1
            _z_msg_put_t put = req->_body._put;
            // Memory cleaning must be done in the feature layer
            _Z_RETURN_IF_ERR(_z_trigger_subscriptions_put(zn, &req->_key, &put._payload, &put._encoding,
                                                          &put._commons._timestamp, req->_ext_qos, &put._attachment,
                                                          reliability, &put._commons._source_info));
#endif
            _z_network_message_t final = _z_n_msg_make_response_final(req->_rid);
            z_result_t ret = _z_send_n_msg(zn, &final, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);
#if Z_FEATURE_SUBSCRIPTION == 0
            _z_n_msg_request_clear(req);
#endif
            return ret;
        }
        case _Z_REQUEST_DEL: {
#if Z_FEATURE_SUBSCRIPTION == 1
            _z_msg_del_t del = req->_body._del;
            // Memory cleaning must be done in the feature layer
            _Z_RETURN_IF_ERR(_z_trigger_subscriptions_del(zn, &req->_key, &del._commons._timestamp, req->_ext_qos,
                                                          &del._attachment, reliability, &del._commons._source_info));
#endif
            _z_network_message_t final = _z_n_msg_make_response_final(req->_rid);
            z_result_t ret = _z_send_n_msg(zn, &final, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);
#if Z_FEATURE_SUBSCRIPTION == 0
            _z_n_msg_request_clear(req);
#endif
            return ret;
        }

        default:
            _Z_INFO("Received unknown request tag: %d\n", req->_tag);
            _z_n_msg_request_clear(req);
            break;
    }
    return _Z_RES_OK;
}

static z_result_t _z_handle_response(_z_session_t *zn, _z_n_msg_response_t *resp) {
#if Z_FEATURE_QUERY == 1
    switch (resp->_tag) {
        case _Z_RESPONSE_BODY_REPLY:
            // Memory cleaning must be done in the feature layer
            return _z_trigger_reply_partial(zn, resp->_request_id, &resp->_key, &resp->_body._reply);
        case _Z_RESPONSE_BODY_ERR:
            // Memory cleaning must be done in the feature layer
            return _z_trigger_reply_err(zn, resp->_request_id, &resp->_body._err);
        default:
            _Z_INFO("Received unknown response tag: %d\n", resp->_tag);
            _z_n_msg_response_clear(resp);
            break;
    }
#else
    _z_n_msg_response_clear(resp);
#endif
    return _Z_RES_OK;
}

z_result_t _z_handle_network_message(_z_session_rc_t *zsrc, _z_zenoh_message_t *msg, uint16_t local_peer_id) {
    z_result_t ret = _Z_RES_OK;
    _z_session_t *zn = _Z_RC_IN_VAL(zsrc);

    switch (msg->_tag) {
        case _Z_N_DECLARE:
            _Z_DEBUG("Handling _Z_N_DECLARE: %i", msg->_body._declare._decl._tag);
            ret = _z_handle_declare(zn, &msg->_body._declare, local_peer_id);
            break;

        case _Z_N_PUSH:
            _Z_DEBUG("Handling _Z_N_PUSH");
            ret = _z_trigger_push(zn, &msg->_body._push, msg->_reliability);
            break;

        case _Z_N_REQUEST:
            _Z_DEBUG("Handling _Z_N_REQUEST");
            ret = _z_handle_request(zsrc, zn, &msg->_body._request, msg->_reliability);
            break;

        case _Z_N_RESPONSE:
            _Z_DEBUG("Handling _Z_N_RESPONSE");
            ret = _z_handle_response(zn, &msg->_body._response);
            break;

        case _Z_N_RESPONSE_FINAL:
            _Z_DEBUG("Handling _Z_N_RESPONSE_FINAL");
            ret = _z_trigger_reply_final(zn, &msg->_body._response_final);
            _z_n_msg_response_final_clear(&msg->_body._response_final);
            break;

        case _Z_N_INTEREST: {
            _Z_DEBUG("Handling _Z_N_INTEREST");
            _z_n_msg_interest_t *interest = &msg->_body._interest;
            if ((interest->_interest.flags & _Z_INTEREST_NOT_FINAL_MASK) != 0) {
                _z_interest_process_interest(zn, interest->_interest._keyexpr, interest->_interest._id,
                                             interest->_interest.flags);
            } else {
                _z_interest_process_interest_final(zn, interest->_interest._id);
            }
            _z_n_msg_interest_clear(&msg->_body._interest);
        } break;

        default:
            _Z_ERROR("Unknown network message ID");
            _z_n_msg_clear(msg);
            break;
    }
    return ret;
}
