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

#include "zenoh-pico/protocol/msg.h"

#include <stddef.h>
#include <string.h>

#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/ext.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/utils/logging.h"

/*=============================*/
/*        Message fields       */
/*=============================*/
_z_reply_context_t *_z_msg_make_reply_context(_z_zint_t qid, _z_id_t replier_id, _Bool is_final) {
    _z_reply_context_t *rctx = (_z_reply_context_t *)z_malloc(sizeof(_z_reply_context_t));
    if (rctx != NULL) {
        rctx->_qid = qid;
        rctx->_replier_id = replier_id;

        rctx->_header = 0;
        if (is_final == true) {
            _Z_SET_FLAG(rctx->_header, _Z_FLAG_Z_F);
        }
    }

    return rctx;
}

/*------------------ Payload field ------------------*/
void _z_payload_clear(_z_bytes_t *p) { _z_bytes_clear(p); }

/*------------------ Timestamp Field ------------------*/
void _z_timestamp_clear(_z_timestamp_t *ts) {}

/*------------------ ResKey Field ------------------*/
void _z_keyexpr_clear(_z_keyexpr_t *rk) {
    rk->_id = 0;
    if (rk->_suffix != NULL && rk->is_alloc) {
        _z_str_clear((char *)rk->_suffix);
        rk->is_alloc = false;
    }
}

void _z_keyexpr_free(_z_keyexpr_t **rk) {
    _z_keyexpr_t *ptr = *rk;

    if (ptr != NULL) {
        _z_keyexpr_clear(ptr);

        z_free(ptr);
        *rk = NULL;
    }
}

/*=============================*/
/*       Zenoh Messages        */
/*=============================*/
/*------------------ Resource Declaration ------------------*/
_z_declaration_t _z_msg_make_declaration_resource(_z_zint_t id, _z_keyexpr_t key) {
    _z_declaration_t decl;

    decl._body._res._id = id;
    decl._body._res._key = key;

    decl._header = _Z_DECL_RESOURCE;
    if (decl._body._res._key._suffix != NULL) {
        _Z_SET_FLAG(decl._header, _Z_FLAG_Z_K);
    }

    return decl;
}

void _z_declaration_clear_resource(_z_res_decl_t *dcl) { _z_keyexpr_clear(&dcl->_key); }

/*------------------ Forget Resource Declaration ------------------*/
_z_declaration_t _z_msg_make_declaration_forget_resource(_z_zint_t rid) {
    _z_declaration_t decl;

    decl._body._forget_res._rid = rid;

    decl._header = _Z_DECL_FORGET_RESOURCE;

    return decl;
}

void _z_declaration_clear_forget_resource(_z_forget_res_decl_t *dcl) { (void)(dcl); }

/*------------------ Publisher Declaration ------------------*/
_z_declaration_t _z_msg_make_declaration_publisher(_z_keyexpr_t key) {
    _z_declaration_t decl;

    decl._body._pub._key = key;

    decl._header = _Z_DECL_PUBLISHER;
    if (key._suffix != NULL) {
        _Z_SET_FLAG(decl._header, _Z_FLAG_Z_K);
    }

    return decl;
}

void _z_declaration_clear_publisher(_z_pub_decl_t *dcl) { _z_keyexpr_clear(&dcl->_key); }

/*------------------ Forget Publisher Declaration ------------------*/
_z_declaration_t _z_msg_make_declaration_forget_publisher(_z_keyexpr_t key) {
    _z_declaration_t decl;

    decl._body._forget_pub._key = key;

    decl._header = _Z_DECL_FORGET_PUBLISHER;
    if (key._suffix != NULL) {
        _Z_SET_FLAG(decl._header, _Z_FLAG_Z_K);
    }

    return decl;
}

void _z_declaration_clear_forget_publisher(_z_forget_pub_decl_t *dcl) { _z_keyexpr_clear(&dcl->_key); }

/*------------------ Subscriber Declaration ------------------*/
_z_declaration_t _z_msg_make_declaration_subscriber(_z_keyexpr_t key, _z_subinfo_t subinfo) {
    _z_declaration_t decl;

    decl._body._sub._key = key;
    decl._body._sub._subinfo = subinfo;

    decl._header = _Z_DECL_SUBSCRIBER;
    if (key._suffix != NULL) {
        _Z_SET_FLAG(decl._header, _Z_FLAG_Z_K);
    }
    if (subinfo.mode != Z_SUBMODE_PUSH) {
        _Z_SET_FLAG(decl._header, _Z_FLAG_Z_S);
    }
    if (subinfo.reliability == Z_RELIABILITY_RELIABLE) {
        _Z_SET_FLAG(decl._header, _Z_FLAG_Z_R);
    }

    return decl;
}

void _z_subinfo_clear(_z_subinfo_t *si) {
    (void)(si);
    // Nothing to clear
}

void _z_declaration_clear_subscriber(_z_sub_decl_t *dcl) {
    _z_keyexpr_clear(&dcl->_key);
    _z_subinfo_clear(&dcl->_subinfo);
}

/*------------------ Forget Subscriber Declaration ------------------*/
_z_declaration_t _z_msg_make_declaration_forget_subscriber(_z_keyexpr_t key) {
    _z_declaration_t decl;

    decl._body._forget_sub._key = key;

    decl._header = _Z_DECL_FORGET_SUBSCRIBER;
    if (key._suffix != NULL) {
        _Z_SET_FLAG(decl._header, _Z_FLAG_Z_K);
    }

    return decl;
}

void _z_declaration_clear_forget_subscriber(_z_forget_sub_decl_t *dcl) { _z_keyexpr_clear(&dcl->_key); }

/*------------------ Queryable Declaration ------------------*/
_z_declaration_t _z_msg_make_declaration_queryable(_z_keyexpr_t key, _z_zint_t complete, _z_zint_t distance) {
    _z_declaration_t decl;

    decl._body._qle._key = key;

    decl._header = _Z_DECL_QUERYABLE;
    if (key._suffix != NULL) {
        _Z_SET_FLAG(decl._header, _Z_FLAG_Z_K);
    }

    decl._body._qle._complete = complete;
    decl._body._qle._distance = distance;
    if ((decl._body._qle._complete != _Z_QUERYABLE_COMPLETE_DEFAULT) ||
        (decl._body._qle._distance != _Z_QUERYABLE_DISTANCE_DEFAULT)) {
        _Z_SET_FLAG(decl._header, _Z_FLAG_Z_Q);
    }

    return decl;
}

void _z_declaration_clear_queryable(_z_qle_decl_t *dcl) { _z_keyexpr_clear(&dcl->_key); }

/*------------------ Forget Queryable Declaration ------------------*/
_z_declaration_t _z_msg_make_declaration_forget_queryable(_z_keyexpr_t key) {
    _z_declaration_t decl;

    decl._body._forget_qle._key = key;

    decl._header = _Z_DECL_FORGET_QUERYABLE;
    if (key._suffix != NULL) {
        _Z_SET_FLAG(decl._header, _Z_FLAG_Z_K);
    }

    return decl;
}

void _z_declaration_clear_forget_queryable(_z_forget_qle_decl_t *dcl) { _z_keyexpr_clear(&dcl->_key); }

/*------------------ Declare ------------------*/
void _z_declaration_clear(_z_declaration_t *dcl) {
    uint8_t did = _Z_MID(dcl->_header);
    switch (did) {
        case _Z_DECL_RESOURCE:
            _z_declaration_clear_resource(&dcl->_body._res);
            break;
        case _Z_DECL_PUBLISHER:
            _z_declaration_clear_publisher(&dcl->_body._pub);
            break;
        case _Z_DECL_SUBSCRIBER:
            _z_declaration_clear_subscriber(&dcl->_body._sub);
            break;
        case _Z_DECL_QUERYABLE:
            _z_declaration_clear_queryable(&dcl->_body._qle);
            break;
        case _Z_DECL_FORGET_RESOURCE:
            _z_declaration_clear_forget_resource(&dcl->_body._forget_res);
            break;
        case _Z_DECL_FORGET_PUBLISHER:
            _z_declaration_clear_forget_publisher(&dcl->_body._forget_pub);
            break;
        case _Z_DECL_FORGET_SUBSCRIBER:
            _z_declaration_clear_forget_subscriber(&dcl->_body._forget_sub);
            break;
        case _Z_DECL_FORGET_QUERYABLE:
            _z_declaration_clear_forget_queryable(&dcl->_body._forget_qle);
            break;
        default:
            _Z_DEBUG("WARNING: Trying to clear declaration with unknown ID(%d)\n", did);
            break;
    }
}

/*------------------ Data Info Field ------------------*/
// @TODO: implement builder for _z_data_info_t

void _z_data_info_clear(_z_data_info_t *di) {
    _z_bytes_clear(&di->_encoding.suffix);
    _z_bytes_clear(&di->_source_id);
    _z_timestamp_clear(&di->_tstamp);
}

/*------------------ Data Message ------------------*/
_z_zenoh_message_t _z_msg_make_data(_z_keyexpr_t key, _z_data_info_t info, _z_bytes_t payload, _Bool can_be_dropped) {
    _z_zenoh_message_t msg;

    msg._body._data._key = key;
    msg._body._data._info = info;
    msg._body._data._payload = payload;

    msg._header = _Z_MID_Z_DATA;
    if (msg._body._data._info._flags != 0) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_Z_I);
    }
    if (msg._body._data._key._suffix != NULL) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_Z_K);
    }
    if (can_be_dropped == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_Z_D);
    }

    return msg;
}

void _z_msg_clear_data(_z_msg_data_t *msg) {
    _z_keyexpr_clear(&msg->_key);
    _z_data_info_clear(&msg->_info);
    _z_payload_clear(&msg->_payload);
}

/*------------------ Unit Message ------------------*/
_z_zenoh_message_t _z_msg_make_unit(_Bool can_be_dropped) {
    _z_zenoh_message_t msg;

    msg._header = _Z_MID_Z_UNIT;
    if (can_be_dropped == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_Z_D);
    }

    return msg;
}

void _z_msg_clear_unit(_z_msg_unit_t *unt) { (void)(unt); }

/*------------------ Pull Message ------------------*/
_z_zenoh_message_t _z_msg_make_pull(_z_keyexpr_t key, _z_zint_t pull_id, _z_zint_t max_samples, _Bool is_final) {
    _z_zenoh_message_t msg;

    msg._body._pull._key = key;
    msg._body._pull._pull_id = pull_id;
    msg._body._pull._max_samples = max_samples;

    msg._header = _Z_MID_Z_PULL;
    if (is_final == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_Z_F);
    }
    if (max_samples == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_Z_N);
    }
    if (msg._body._pull._key._suffix != NULL) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_Z_K);
    }

    return msg;
}

void _z_msg_clear_pull(_z_msg_pull_t *msg) { _z_keyexpr_clear(&msg->_key); }

/*------------------ Query Message ------------------*/
_z_zenoh_message_t _z_msg_make_query(_z_keyexpr_t key, _z_bytes_t parameters, _z_zint_t qid,
                                     z_consolidation_mode_t consolidation, _z_value_t value) {
    _z_zenoh_message_t msg;

    msg._body._query._parameters = parameters;
    msg._body._query._consolidation = consolidation;
    (void)memset(&msg._body._query._info, 0, sizeof(msg._body._query._info));
    msg._body._query._value = value;

    msg._header = _Z_MID_Z_QUERY;
    if (msg._body._query._value.payload.len > 0) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_Z_B);
    }

    return msg;
}

/*------------------ Reply Message ------------------*/
_z_zenoh_message_t _z_msg_make_reply(_z_keyexpr_t key, _z_data_info_t info, _z_bytes_t payload, _Bool can_be_dropped) {
    _z_zenoh_message_t msg = _z_msg_make_data(key, info, payload, can_be_dropped);

    return msg;
}

/*------------------ Zenoh Message ------------------*/
void _z_msg_clear(_z_zenoh_message_t *msg) {
    uint8_t mid = _Z_MID(msg->_header);
    switch (mid) {
        case _Z_MID_Z_DATA:
            _z_msg_clear_data(&msg->_body._data);
            break;
        case _Z_MID_Z_PULL:
            _z_msg_clear_pull(&msg->_body._pull);
            break;
        case _Z_MID_Z_QUERY:
            _z_msg_query_clear(&msg->_body._query);
            break;
        case _Z_MID_Z_UNIT:
            _z_msg_clear_unit(&msg->_body._unit);
            break;
        default:
            _Z_DEBUG("WARNING: Trying to clear message with unknown ID(%d)\n", mid);
            break;
    }
}

void _z_msg_free(_z_zenoh_message_t **msg) {
    _z_zenoh_message_t *ptr = *msg;

    if (ptr != NULL) {
        _z_msg_clear(ptr);

        z_free(ptr);
        *msg = NULL;
    }
}

void _z_request_body_clear(_z_request_body_t *msg) { (void)(msg); }

void _z_response_body_clear(_z_response_body_t *msg) { (void)(msg); }

_z_network_message_t _z_n_msg_make_declare(_z_declaration_t declaration) {
    _z_network_message_t msg;
    msg._header = _Z_MID_N_DECLARE;

    msg._body._declare._declaration = declaration;
    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}

void _z_n_msg_clear_declare(_z_n_msg_declare_t *msg) { _z_declaration_clear(&msg->_declaration); }

_z_network_message_t _z_n_msg_make_push(_z_keyexpr_t key, _z_push_body_t body, _Bool is_remote_mapping) {
    _z_network_message_t msg;
    msg._header = _Z_MID_N_PUSH;

    msg._body._push._key = key;
    if (key._suffix != NULL) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_N_PUSH_N);
    }
    if (is_remote_mapping == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_N_PUSH_M);
    }
    msg._body._push._body = body;

    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}

_z_network_message_t _z_n_msg_make_request(_z_zint_t rid, _z_keyexpr_t key, _z_request_body_t body,
                                           _Bool is_remote_mapping) {
    _z_network_message_t msg;
    msg._header = _Z_MID_N_REQUEST;

    msg._body._request._rid = rid;
    msg._body._request._key = key;
    if (key._suffix != NULL) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_N_REQUEST_N);
    }
    if (is_remote_mapping == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_N_REQUEST_M);
    }
    msg._body._request._body = body;

    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}

_z_network_message_t _z_n_msg_make_response(_z_zint_t rid, _z_keyexpr_t key, _z_response_body_t body,
                                            _Bool is_remote_mapping) {
    _z_network_message_t msg;
    msg._header = _Z_MID_N_RESPONSE;

    msg._body._response._rid = rid;
    msg._body._response._key = key;
    if (key._suffix != NULL) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_N_RESPONSE_N);
    }
    if (is_remote_mapping == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_N_RESPONSE_M);
    }
    msg._body._response._body = body;

    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}

_z_network_message_t _z_n_msg_make_response_final(_z_zint_t rid) {
    _z_network_message_t msg;
    msg._header = _Z_MID_N_RESPONSE_FINAL;

    msg._body._response_f._requestid = rid;
    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}