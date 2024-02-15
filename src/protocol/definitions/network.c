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

#include "zenoh-pico/protocol/definitions/network.h"

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/message.h"
#include "zenoh-pico/utils/logging.h"

_z_n_msg_request_exts_t _z_n_msg_request_needed_exts(const _z_n_msg_request_t *msg) {
    _z_n_msg_request_exts_t ret = {.n = 0,
                                   .ext_budget = msg->_ext_budget != 0,
                                   .ext_target = msg->_ext_target != Z_QUERY_TARGET_BEST_MATCHING,
                                   .ext_qos = msg->_ext_qos._val != _Z_N_QOS_DEFAULT._val,
                                   .ext_timeout_ms = msg->_ext_timeout_ms != 0,
                                   .ext_tstamp = _z_timestamp_check(&msg->_ext_timestamp)};
    if (ret.ext_budget) {
        ret.n += 1;
    }
    if (ret.ext_target) {
        ret.n += 1;
    }
    if (ret.ext_qos) {
        ret.n += 1;
    }
    if (ret.ext_timeout_ms) {
        ret.n += 1;
    }
    if (ret.ext_tstamp) {
        ret.n += 1;
    }
    return ret;
}

void _z_n_msg_request_clear(_z_n_msg_request_t *msg) {
    _z_keyexpr_clear(&msg->_key);
    switch (msg->_tag) {
        case _Z_REQUEST_QUERY: {
            _z_msg_query_clear(&msg->_body._query);
        } break;
        case _Z_REQUEST_PUT: {
            _z_msg_put_clear(&msg->_body._put);
        } break;
        case _Z_REQUEST_DEL: {
            _z_msg_del_clear(&msg->_body._del);
        } break;
        case _Z_REQUEST_PULL: {
            _z_msg_pull_clear(&msg->_body._pull);
        } break;
    }
}

/*=============================*/
/*      Network Messages       */
/*=============================*/
void _z_push_body_clear(_z_push_body_t *msg) {
    if (msg->_is_put) {
        _z_msg_put_clear(&msg->_body._put);
    }
}
_z_push_body_t _z_push_body_steal(_z_push_body_t *msg) {
    _z_push_body_t ret = *msg;
    *msg = _z_push_body_null();
    return ret;
}
_z_push_body_t _z_push_body_null() {
    return (_z_push_body_t){
        ._is_put = false,
        ._body._del._commons = {._timestamp = _z_timestamp_null(), ._source_info = _z_source_info_null()}};
}

void _z_n_msg_response_final_clear(_z_n_msg_response_final_t *msg) { (void)(msg); }

void _z_n_msg_push_clear(_z_n_msg_push_t *msg) {
    _z_keyexpr_clear(&msg->_key);
    _z_push_body_clear(&msg->_body);
}

void _z_n_msg_response_clear(_z_n_msg_response_t *msg) {
    _z_timestamp_clear(&msg->_ext_timestamp);
    _z_keyexpr_clear(&msg->_key);
    switch (msg->_tag) {
        case _Z_RESPONSE_BODY_REPLY: {
            _z_msg_reply_clear(&msg->_body._reply);
            break;
        }
        case _Z_RESPONSE_BODY_ERR: {
            _z_msg_err_clear(&msg->_body._err);
            break;
        }
        case _Z_RESPONSE_BODY_ACK: {
            break;
        }
        case _Z_RESPONSE_BODY_PUT: {
            _z_msg_put_clear(&msg->_body._put);
            break;
        }
        case _Z_RESPONSE_BODY_DEL: {
            break;
        }
    }
}

void _z_n_msg_clear(_z_network_message_t *msg) {
    switch (msg->_tag) {
        case _Z_N_PUSH:
            _z_n_msg_push_clear(&msg->_body._push);
            break;
        case _Z_N_REQUEST:
            _z_n_msg_request_clear(&msg->_body._request);
            break;
        case _Z_N_RESPONSE:
            _z_n_msg_response_clear(&msg->_body._response);
            break;
        case _Z_N_RESPONSE_FINAL:
            _z_n_msg_response_final_clear(&msg->_body._response_final);
            break;
        case _Z_N_DECLARE:
            _z_n_msg_declare_clear(&msg->_body._declare);
            break;
    }
}

void _z_n_msg_free(_z_network_message_t **msg) {
    _z_network_message_t *ptr = *msg;

    if (ptr != NULL) {
        _z_n_msg_clear(ptr);

        zp_free(ptr);
        *msg = NULL;
    }
}
_z_network_message_t _z_msg_make_pull(_z_keyexpr_t key, _z_zint_t pull_id) {
    _z_network_message_t ret = {
        ._tag = _Z_N_REQUEST,
        ._body =
            {
                ._request =
                    {
                        ._rid = pull_id,
                        ._key = key,
                        ._tag = _Z_REQUEST_PULL,
                        ._body =
                            {
                                ._pull = {._ext_source_info = _z_source_info_null()},
                            },
                        ._ext_budget = 0,
                        ._ext_qos = _Z_N_QOS_DEFAULT,
                        ._ext_target = Z_QUERY_TARGET_BEST_MATCHING,
                        ._ext_timestamp = _z_timestamp_null(),
                        ._ext_timeout_ms = 0,
                    },
            },
    };
    return ret;
}
_z_zenoh_message_t _z_msg_make_query(_Z_MOVE(_z_keyexpr_t) key, _Z_MOVE(_z_bytes_t) parameters, _z_zint_t qid,
                                     z_consolidation_mode_t consolidation, _Z_MOVE(_z_value_t) value
#if Z_FEATURE_ATTACHMENT == 1
                                     ,
                                     z_attachment_t attachment
#endif
) {
    return (_z_zenoh_message_t){
        ._tag = _Z_N_REQUEST,
        ._body._request =
            {
                ._rid = qid,
                ._key = _z_keyexpr_steal(key),
                ._tag = _Z_REQUEST_QUERY,
                ._body._query = {._parameters = _z_bytes_steal(parameters),
                                 ._ext_consolidation = consolidation,
                                 ._ext_value = _z_value_steal(value),
                                 ._ext_info = _z_source_info_null(),
#if Z_FEATURE_ATTACHMENT == 1
                                 ._ext_attachment = {.body.decoded = attachment, .is_encoded = false}
#endif
                },
                ._ext_budget = 0,
                ._ext_qos = _Z_N_QOS_DEFAULT,
                ._ext_target = Z_QUERY_TARGET_BEST_MATCHING,
                ._ext_timeout_ms = 0,
                ._ext_timestamp = _z_timestamp_null(),
            },
    };
}
_z_network_message_t _z_n_msg_make_response_final(_z_zint_t rid) {
    return (_z_zenoh_message_t){
        ._tag = _Z_N_RESPONSE_FINAL,
        ._body = {._response_final = {._request_id = rid}},
    };
}
_z_network_message_t _z_n_msg_make_declare(_z_declaration_t declaration) {
    return (_z_network_message_t){
        ._tag = _Z_N_DECLARE,
        ._body._declare =
            {
                ._decl = declaration,
                ._ext_qos = _Z_N_QOS_DEFAULT,
                ._ext_timestamp = _z_timestamp_null(),
            },
    };
}
_z_network_message_t _z_n_msg_make_push(_Z_MOVE(_z_keyexpr_t) key, _Z_MOVE(_z_push_body_t) body) {
    return (_z_network_message_t){
        ._tag = _Z_N_PUSH,
        ._body._push = {._key = _z_keyexpr_steal(key), ._body = _z_push_body_steal(body)},
    };
}
_z_network_message_t _z_n_msg_make_reply(_z_zint_t rid, _Z_MOVE(_z_keyexpr_t) key, _Z_MOVE(_z_value_t) value) {
    return (_z_network_message_t){
        ._tag = _Z_N_RESPONSE,
        ._body._response =
            {
                ._key = _z_keyexpr_steal(key),
                ._tag = _Z_RESPONSE_BODY_REPLY,
                ._request_id = rid,
                ._body._reply =
                    {
                        ._timestamp = _z_timestamp_null(),
                        ._value = _z_value_steal(value),
                        ._ext_source_info = _z_source_info_null(),
                        ._ext_consolidation = Z_CONSOLIDATION_MODE_AUTO,
                    },
                ._ext_qos = _Z_N_QOS_DEFAULT,
                ._ext_timestamp = _z_timestamp_null(),
                ._ext_responder =
                    {
                        ._eid = 0,
                        ._zid = _z_id_empty(),
                    },
            },

    };
}
_z_network_message_t _z_n_msg_make_ack(_z_zint_t rid, _Z_MOVE(_z_keyexpr_t) key) {
    return (_z_network_message_t){
        ._tag = _Z_N_RESPONSE,
        ._body._response =
            {
                ._tag = _Z_RESPONSE_BODY_ACK,
                ._request_id = rid,
                ._key = _z_keyexpr_steal(key),
                ._body._ack = {._timestamp = _z_timestamp_null(), ._ext_source_info = _z_source_info_null()},
            },
    };
}
void _z_msg_fix_mapping(_z_zenoh_message_t *msg, uint16_t mapping) {
    switch (msg->_tag) {
        case _Z_N_DECLARE: {
            _z_decl_fix_mapping(&msg->_body._declare._decl, mapping);
        } break;
        case _Z_N_PUSH: {
            _z_keyexpr_fix_mapping(&msg->_body._push._key, mapping);
        } break;
        case _Z_N_REQUEST: {
            _z_keyexpr_fix_mapping(&msg->_body._request._key, mapping);
        } break;
        case _Z_N_RESPONSE: {
            _z_keyexpr_fix_mapping(&msg->_body._response._key, mapping);
        } break;
        default:
            break;
    }
}
_z_qos_t _z_n_qos_unmake_public(_z_n_qos_t n_qos) {
    _z_qos_t qos = _z_n_qos_unmake(n_qos);
    if (qos.priority == Z_PRIORITY_REAL_TIME) {
        qos.priority = Z_PRIORITY_DEFAULT;
    }
    return qos;
}
