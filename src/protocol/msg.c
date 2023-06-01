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

#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/ext.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/utils/logging.h"

/*=============================*/
/*        Message fields       */
/*=============================*/
/*------------------ Payload field ------------------*/
void _z_payload_clear(_z_payload_t *p) { _z_bytes_clear(p); }

/*------------------ Timestamp Field ------------------*/
void _z_timestamp_clear(_z_timestamp_t *ts) {}

/*------------------ ResKey Field ------------------*/
void _z_keyexpr_clear(_z_keyexpr_t *rk) {
    rk->_id = 0;
    if (rk->_suffix != NULL) {
        _z_str_clear((char *)rk->_suffix);
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

/*------------------ Locators Field ------------------*/
void _z_locators_clear(_z_locator_array_t *ls) { _z_locator_array_clear(ls); }

/*=============================*/
/*      Message decorators     */
/*=============================*/
/*------------------ Attachment Decorator ------------------*/
void _z_t_msg_clear_attachment(_z_attachment_t *a) { _z_payload_clear(&a->_payload); }

/*------------------ ReplyContext Decorator ------------------*/
_z_reply_context_t *_z_msg_make_reply_context(_z_zint_t qid, _z_bytes_t replier_id, _Bool is_final) {
    _z_reply_context_t *rctx = (_z_reply_context_t *)z_malloc(sizeof(_z_reply_context_t));
    if (rctx != NULL) {
        rctx->_qid = qid;
        rctx->_replier_id = replier_id;

        rctx->_header = _Z_MID_A_REPLY_CONTEXT;
        if (is_final == true) {
            _Z_SET_FLAG(rctx->_header, _Z_FLAG_Z_F);
        }
    }

    return rctx;
}

void _z_msg_clear_reply_context(_z_reply_context_t *rc) {
    if (_Z_HAS_FLAG(rc->_header, _Z_FLAG_Z_F) == false) {
        _z_bytes_clear(&rc->_replier_id);
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
_z_zenoh_message_t _z_msg_make_data(_z_keyexpr_t key, _z_data_info_t info, _z_payload_t payload, _Bool can_be_dropped) {
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

    msg._attachment = NULL;
    msg._reply_context = NULL;

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

    msg._attachment = NULL;
    msg._reply_context = NULL;

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

    msg._attachment = NULL;
    msg._reply_context = NULL;

    return msg;
}

void _z_msg_clear_pull(_z_msg_pull_t *msg) { _z_keyexpr_clear(&msg->_key); }

/*------------------ Query Message ------------------*/
_z_zenoh_message_t _z_msg_make_query(_z_keyexpr_t key, char *parameters, _z_zint_t qid, z_query_target_t target,
                                     z_consolidation_mode_t consolidation, _z_value_t value) {
    _z_zenoh_message_t msg;

    msg._body._query._key = key;
    msg._body._query._parameters = parameters;
    msg._body._query._qid = qid;
    msg._body._query._target = target;
    msg._body._query._consolidation = consolidation;
    (void)memset(&msg._body._query._info, 0, sizeof(msg._body._query._info));
    msg._body._query._info._encoding = value.encoding;
    msg._body._query._payload = value.payload;

    msg._header = _Z_MID_Z_QUERY;
    if (msg._body._query._target != Z_QUERY_TARGET_BEST_MATCHING) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_Z_T);
    }
    if (msg._body._query._key._suffix != NULL) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_Z_K);
    }
    if (msg._body._query._payload.len > 0) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_Z_B);
    }

    msg._attachment = NULL;
    msg._reply_context = NULL;

    return msg;
}

void _z_msg_clear_query(_z_msg_query_t *msg) {
    _z_keyexpr_clear(&msg->_key);
    _z_str_clear(msg->_parameters);
    _z_bytes_clear(&msg->_info._encoding.suffix);
    _z_bytes_clear(&msg->_payload);
}

/*------------------ Reply Message ------------------*/
_z_zenoh_message_t _z_msg_make_reply(_z_keyexpr_t key, _z_data_info_t info, _z_payload_t payload, _Bool can_be_dropped,
                                     _z_reply_context_t *rctx) {
    _z_zenoh_message_t msg = _z_msg_make_data(key, info, payload, can_be_dropped);
    msg._reply_context = rctx;

    return msg;
}

/*------------------ Zenoh Message ------------------*/
void _z_msg_clear(_z_zenoh_message_t *msg) {
    if (msg->_attachment != NULL) {
        _z_t_msg_clear_attachment(msg->_attachment);
        z_free(msg->_attachment);
    }
    if (msg->_reply_context != NULL) {
        _z_msg_clear_reply_context(msg->_reply_context);
        z_free(msg->_reply_context);
    }

    uint8_t mid = _Z_MID(msg->_header);
    switch (mid) {
        case _Z_MID_Z_DATA:
            _z_msg_clear_data(&msg->_body._data);
            break;
        case _Z_MID_Z_PULL:
            _z_msg_clear_pull(&msg->_body._pull);
            break;
        case _Z_MID_Z_QUERY:
            _z_msg_clear_query(&msg->_body._query);
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

/*=============================*/
/*      Network Messages       */
/*=============================*/
void _z_push_body_clear(_z_push_body_t *msg) { (void)(msg); }

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

void _z_n_msg_clear_push(_z_n_msg_push_t *msg) {
    _z_keyexpr_clear(&msg->_key);
    _z_push_body_clear(&msg->_body);
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

void _z_n_msg_clear_request(_z_n_msg_request_t *msg) {
    _z_keyexpr_clear(&msg->_key);
    _z_request_body_clear(&msg->_body);
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

void _z_n_msg_clear_response(_z_n_msg_response_t *msg) {
    _z_keyexpr_clear(&msg->_key);
    _z_response_body_clear(&msg->_body);
}

_z_network_message_t _z_n_msg_make_response_final(_z_zint_t rid) {
    _z_network_message_t msg;
    msg._header = _Z_MID_N_RESPONSE_FINAL;

    msg._body._response_f._rid = rid;
    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}

void _z_n_msg_clear_response_final(_z_n_msg_response_final_t *msg) { (void)(msg); }

void _z_n_msg_clear(_z_network_message_t *msg) {
    uint8_t mid = _Z_MID(msg->_header);
    switch (mid) {
        case _Z_MID_N_PUSH:
            _z_n_msg_clear_push(&msg->_body._push);
            break;
        case _Z_MID_N_REQUEST:
            _z_n_msg_clear_request(&msg->_body._request);
            break;
        case _Z_MID_N_RESPONSE:
            _z_n_msg_clear_response(&msg->_body._response);
            break;
        case _Z_MID_N_RESPONSE_FINAL:
            _z_n_msg_clear_response_final(&msg->_body._response_f);
            break;
        case _Z_MID_N_DECLARE:
            _z_n_msg_clear_declare(&msg->_body._declare);
            break;
        default:
            _Z_DEBUG("WARNING: Trying to clear network message with unknown ID(%d)\n", mid);
            break;
    }

    _z_msg_ext_vec_clear(&msg->_extensions);
}

void _z_n_msg_free(_z_network_message_t **msg) {
    _z_network_message_t *ptr = *msg;

    if (ptr != NULL) {
        _z_n_msg_clear(ptr);

        z_free(ptr);
        *msg = NULL;
    }
}

/*=============================*/
/*     Transport Messages      */
/*=============================*/
/*------------------ Scout Message ------------------*/
_z_scouting_message_t _z_s_msg_make_scout(z_what_t what, _z_bytes_t zid) {
    _z_scouting_message_t msg;
    msg._header = _Z_MID_SCOUT;

    msg._body._scout._version = Z_PROTO_VERSION;
    msg._body._scout._what = what;
    msg._body._scout._zid = zid;

    msg._attachment = NULL;
    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}

void _z_s_msg_copy_scout(_z_s_msg_scout_t *clone, _z_s_msg_scout_t *msg) {
    clone->_what = msg->_what;
    clone->_version = msg->_version;
    _z_bytes_copy(&clone->_zid, &msg->_zid);
}

void _z_s_msg_clear_scout(_z_s_msg_scout_t *msg) { _z_bytes_clear(&msg->_zid); }

/*------------------ Hello Message ------------------*/
_z_scouting_message_t _z_s_msg_make_hello(z_whatami_t whatami, _z_bytes_t zid, _z_locator_array_t locators) {
    _z_scouting_message_t msg;
    msg._header = _Z_MID_HELLO;

    msg._body._hello._version = Z_PROTO_VERSION;
    msg._body._hello._whatami = whatami;
    msg._body._hello._zid = zid;
    msg._body._hello._locators = locators;

    if (_z_locator_array_is_empty(&locators) == false) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_HELLO_L);
    }

    msg._attachment = NULL;
    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}

void _z_s_msg_copy_hello(_z_s_msg_hello_t *clone, _z_s_msg_hello_t *msg) {
    _z_locator_array_copy(&clone->_locators, &msg->_locators);
    _z_bytes_copy(&clone->_zid, &msg->_zid);
    clone->_whatami = msg->_whatami;
}

void _z_s_msg_clear_hello(_z_s_msg_hello_t *msg) {
    _z_bytes_clear(&msg->_zid);
    _z_locators_clear(&msg->_locators);
}

/*------------------ Join Message ------------------*/
_z_transport_message_t _z_t_msg_make_join(z_whatami_t whatami, _z_zint_t lease, _z_bytes_t zid,
                                          _z_conduit_sn_list_t next_sn) {
    _z_transport_message_t msg;
    msg._header = _Z_MID_T_JOIN;

    msg._body._join._version = Z_PROTO_VERSION;
    msg._body._join._whatami = whatami;
    msg._body._join._lease = lease;
    msg._body._join._seq_num_res = Z_SN_RESOLUTION;
    msg._body._join._key_id_res = Z_KID_RESOLUTION;
    msg._body._join._req_id_res = Z_REQ_RESOLUTION;
    msg._body._join._batch_size = Z_BATCH_SIZE;
    msg._body._join._next_sn = next_sn;
    msg._body._join._zid = zid;

    if ((lease % 1000) == 0) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_JOIN_T);
    }

    if ((msg._body._join._batch_size != _Z_DEFAULT_BATCH_SIZE) ||
        (msg._body._join._seq_num_res != _Z_DEFAULT_RESOLUTION_SIZE) ||
        (msg._body._join._key_id_res != _Z_DEFAULT_RESOLUTION_SIZE) ||
        (msg._body._join._req_id_res != _Z_DEFAULT_RESOLUTION_SIZE)) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_JOIN_S);
    }

    msg._attachment = NULL;
    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}

void _z_t_msg_copy_join(_z_t_msg_join_t *clone, _z_t_msg_join_t *msg) {
    clone->_version = msg->_version;
    clone->_whatami = msg->_whatami;
    clone->_lease = msg->_lease;
    clone->_seq_num_res = msg->_seq_num_res;
    clone->_key_id_res = msg->_key_id_res;
    clone->_req_id_res = msg->_req_id_res;
    clone->_batch_size = msg->_batch_size;
    clone->_next_sn = msg->_next_sn;
    _z_bytes_copy(&clone->_zid, &msg->_zid);
}

void _z_t_msg_clear_join(_z_t_msg_join_t *msg) { _z_bytes_clear(&msg->_zid); }

/*------------------ Init Message ------------------*/
_z_transport_message_t _z_t_msg_make_init_syn(z_whatami_t whatami, _z_bytes_t zid) {
    _z_transport_message_t msg;
    msg._header = _Z_MID_T_INIT;

    msg._body._init._version = Z_PROTO_VERSION;
    msg._body._init._whatami = whatami;
    msg._body._init._zid = zid;
    msg._body._init._seq_num_res = Z_SN_RESOLUTION;
    msg._body._init._key_id_res = Z_KID_RESOLUTION;
    msg._body._init._req_id_res = Z_REQ_RESOLUTION;
    msg._body._init._batch_size = Z_BATCH_SIZE;
    _z_bytes_reset(&msg._body._init._cookie);

    if ((msg._body._init._batch_size != _Z_DEFAULT_BATCH_SIZE) ||
        (msg._body._init._seq_num_res != _Z_DEFAULT_RESOLUTION_SIZE) ||
        (msg._body._init._key_id_res != _Z_DEFAULT_RESOLUTION_SIZE) ||
        (msg._body._init._req_id_res != _Z_DEFAULT_RESOLUTION_SIZE)) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_INIT_S);
    }

    msg._attachment = NULL;
    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}

_z_transport_message_t _z_t_msg_make_init_ack(z_whatami_t whatami, _z_bytes_t zid, _z_bytes_t cookie) {
    _z_transport_message_t msg;
    msg._header = _Z_MID_T_INIT;
    _Z_SET_FLAG(msg._header, _Z_FLAG_T_INIT_A);

    msg._body._init._version = Z_PROTO_VERSION;
    msg._body._init._whatami = whatami;
    msg._body._init._zid = zid;
    msg._body._init._seq_num_res = Z_SN_RESOLUTION;
    msg._body._init._key_id_res = Z_KID_RESOLUTION;
    msg._body._init._req_id_res = Z_REQ_RESOLUTION;
    msg._body._init._batch_size = Z_BATCH_SIZE;
    msg._body._init._cookie = cookie;

    if ((msg._body._init._batch_size != _Z_DEFAULT_BATCH_SIZE) ||
        (msg._body._init._seq_num_res != _Z_DEFAULT_RESOLUTION_SIZE) ||
        (msg._body._init._key_id_res != _Z_DEFAULT_RESOLUTION_SIZE) ||
        (msg._body._init._req_id_res != _Z_DEFAULT_RESOLUTION_SIZE)) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_INIT_S);
    }

    msg._attachment = NULL;
    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}

void _z_t_msg_copy_init(_z_t_msg_init_t *clone, _z_t_msg_init_t *msg) {
    clone->_version = msg->_version;
    clone->_whatami = msg->_whatami;
    clone->_seq_num_res = msg->_seq_num_res;
    clone->_key_id_res = msg->_key_id_res;
    clone->_req_id_res = msg->_req_id_res;
    clone->_batch_size = msg->_batch_size;
    _z_bytes_copy(&clone->_zid, &msg->_zid);
    _z_bytes_copy(&clone->_cookie, &msg->_cookie);
}

void _z_t_msg_clear_init(_z_t_msg_init_t *msg) {
    _z_bytes_clear(&msg->_zid);
    _z_bytes_clear(&msg->_cookie);
}

/*------------------ Open Message ------------------*/
_z_transport_message_t _z_t_msg_make_open_syn(_z_zint_t lease, _z_zint_t initial_sn, _z_bytes_t cookie) {
    _z_transport_message_t msg;
    msg._header = _Z_MID_T_OPEN;

    msg._body._open._lease = lease;
    msg._body._open._initial_sn = initial_sn;
    msg._body._open._cookie = cookie;

    if ((lease % 1000) == 0) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_OPEN_T);
    }

    msg._attachment = NULL;
    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}

_z_transport_message_t _z_t_msg_make_open_ack(_z_zint_t lease, _z_zint_t initial_sn) {
    _z_transport_message_t msg;
    msg._header = _Z_MID_T_OPEN;
    _Z_SET_FLAG(msg._header, _Z_FLAG_T_OPEN_A);

    msg._body._open._lease = lease;
    msg._body._open._initial_sn = initial_sn;
    _z_bytes_reset(&msg._body._open._cookie);

    if ((lease % 1000) == 0) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_OPEN_T);
    }

    msg._attachment = NULL;
    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}

void _z_t_msg_copy_open(_z_t_msg_open_t *clone, _z_t_msg_open_t *msg) {
    clone->_lease = msg->_lease;
    clone->_initial_sn = msg->_initial_sn;
    _z_bytes_copy(&clone->_cookie, &msg->_cookie);
}

void _z_t_msg_clear_open(_z_t_msg_open_t *msg) { _z_bytes_clear(&msg->_cookie); }

/*------------------ Close Message ------------------*/
_z_transport_message_t _z_t_msg_make_close(uint8_t reason, _Bool link_only) {
    _z_transport_message_t msg;
    msg._header = _Z_MID_T_CLOSE;

    msg._body._close._reason = reason;
    if (link_only == false) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_CLOSE_S);
    }

    msg._attachment = NULL;
    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}

void _z_t_msg_copy_close(_z_t_msg_close_t *clone, _z_t_msg_close_t *msg) { clone->_reason = msg->_reason; }

void _z_t_msg_clear_close(_z_t_msg_close_t *msg) { (void)(msg); }

/*------------------ Keep Alive Message ------------------*/
_z_transport_message_t _z_t_msg_make_keep_alive(void) {
    _z_transport_message_t msg;
    msg._header = _Z_MID_T_KEEP_ALIVE;

    msg._attachment = NULL;
    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}

void _z_t_msg_copy_keep_alive(_z_t_msg_keep_alive_t *clone, _z_t_msg_keep_alive_t *msg) {
    (void)(clone);
    (void)(msg);
}

void _z_t_msg_clear_keep_alive(_z_t_msg_keep_alive_t *msg) { (void)(msg); }

/*------------------ Frame Message ------------------*/
_z_transport_message_t _z_t_msg_make_frame_header(_z_zint_t sn, _Bool is_reliable) {
    _z_transport_message_t msg;
    msg._header = _Z_MID_T_FRAME;

    msg._body._frame._sn = sn;
    if (is_reliable == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_FRAME_R);
    }

    msg._body._frame._messages = _z_network_message_vec_make(0);

    msg._attachment = NULL;
    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}

_z_transport_message_t _z_t_msg_make_frame(_z_zint_t sn, _z_network_message_vec_t messages, _Bool is_reliable) {
    _z_transport_message_t msg;
    msg._header = _Z_MID_T_FRAME;

    msg._body._frame._sn = sn;
    if (is_reliable == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_FRAME_R);
    }

    msg._body._frame._messages = messages;

    msg._attachment = NULL;
    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}

void _z_t_msg_copy_frame(_z_t_msg_frame_t *clone, _z_t_msg_frame_t *msg) {
    clone->_sn = msg->_sn;
    _z_network_message_vec_copy(&clone->_messages, &msg->_messages);
}

void _z_t_msg_clear_frame(_z_t_msg_frame_t *msg) { _z_network_message_vec_clear(&msg->_messages); }

/*------------------ Fragment Message ------------------*/
_z_transport_message_t _z_t_msg_make_fragment(_z_zint_t sn, _z_payload_t payload, _Bool is_reliable, _Bool is_last) {
    _z_transport_message_t msg;
    msg._header = _Z_MID_T_FRAGMENT;
    if (is_last == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_FRAGMENT_M);
    }
    if (is_reliable == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_FRAGMENT_R);
    }

    msg._body._fragment._sn = sn;
    msg._body._fragment._payload = payload;

    msg._attachment = NULL;
    msg._extensions = _z_msg_ext_vec_make(0);

    return msg;
}

void _z_t_msg_copy_fragment(_z_t_msg_fragment_t *clone, _z_t_msg_fragment_t *msg) {
    _z_bytes_copy(&clone->_payload, &msg->_payload);
}

void _z_t_msg_clear_fragment(_z_t_msg_fragment_t *msg) { _z_bytes_clear(&msg->_payload); }

/*------------------ Transport Message ------------------*/
void _z_t_msg_copy(_z_transport_message_t *clone, _z_transport_message_t *msg) {
    clone->_header = msg->_header;
    clone->_attachment = msg->_attachment;
    _z_msg_ext_vec_copy(&clone->_extensions, &msg->_extensions);

    uint8_t mid = _Z_MID(msg->_header);
    switch (mid) {
        case _Z_MID_T_JOIN: {
            _z_t_msg_copy_join(&clone->_body._join, &msg->_body._join);
        } break;

        case _Z_MID_T_INIT: {
            _z_t_msg_copy_init(&clone->_body._init, &msg->_body._init);
        } break;

        case _Z_MID_T_OPEN: {
            _z_t_msg_copy_open(&clone->_body._open, &msg->_body._open);
        } break;

        case _Z_MID_T_CLOSE: {
            _z_t_msg_copy_close(&clone->_body._close, &msg->_body._close);
        } break;

        case _Z_MID_T_KEEP_ALIVE: {
            _z_t_msg_copy_keep_alive(&clone->_body._keep_alive, &msg->_body._keep_alive);
        } break;

        case _Z_MID_T_FRAME: {
            _z_t_msg_copy_frame(&clone->_body._frame, &msg->_body._frame);
        } break;

        case _Z_MID_T_FRAGMENT: {
            _z_t_msg_copy_fragment(&clone->_body._fragment, &msg->_body._fragment);
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to copy transport message with unknown ID(%d)\n", mid);
        } break;
    }
}

void _z_t_msg_clear(_z_transport_message_t *msg) {
    if (msg->_attachment != NULL) {
        _z_t_msg_clear_attachment(msg->_attachment);
        z_free(msg->_attachment);
    }

    uint8_t mid = _Z_MID(msg->_header);
    switch (mid) {
        case _Z_MID_T_JOIN: {
            _z_t_msg_clear_join(&msg->_body._join);
        } break;

        case _Z_MID_T_INIT: {
            _z_t_msg_clear_init(&msg->_body._init);
        } break;

        case _Z_MID_T_OPEN: {
            _z_t_msg_clear_open(&msg->_body._open);
        } break;

        case _Z_MID_T_CLOSE: {
            _z_t_msg_clear_close(&msg->_body._close);
        } break;

        case _Z_MID_T_KEEP_ALIVE: {
            _z_t_msg_clear_keep_alive(&msg->_body._keep_alive);
        } break;

        case _Z_MID_T_FRAME: {
            _z_t_msg_clear_frame(&msg->_body._frame);
        } break;

        case _Z_MID_T_FRAGMENT: {
            _z_t_msg_clear_fragment(&msg->_body._fragment);
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to clear transport message with unknown ID(%d)\n", mid);
        } break;
    }

    _z_msg_ext_vec_clear(&msg->_extensions);
}

/*------------------ Scouting Message ------------------*/
void _z_s_msg_copy(_z_scouting_message_t *clone, _z_scouting_message_t *msg) {
    clone->_header = msg->_header;
    clone->_attachment = msg->_attachment;
    _z_msg_ext_vec_copy(&clone->_extensions, &msg->_extensions);

    uint8_t mid = _Z_MID(msg->_header);
    switch (mid) {
        case _Z_MID_SCOUT: {
            _z_s_msg_copy_scout(&clone->_body._scout, &msg->_body._scout);
        } break;

        case _Z_MID_HELLO: {
            _z_s_msg_copy_hello(&clone->_body._hello, &msg->_body._hello);
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to copy session message with unknown ID(%d)\n", mid);
        } break;
    }
}

void _z_s_msg_clear(_z_scouting_message_t *msg) {
    if (msg->_attachment != NULL) {
        _z_t_msg_clear_attachment(msg->_attachment);
        z_free(msg->_attachment);
    }

    uint8_t mid = _Z_MID(msg->_header);
    switch (mid) {
        case _Z_MID_SCOUT: {
            _z_s_msg_clear_scout(&msg->_body._scout);
        } break;

        case _Z_MID_HELLO: {
            _z_s_msg_clear_hello(&msg->_body._hello);
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to clear session message with unknown ID(%d)\n", mid);
        } break;
    }

    _z_msg_ext_vec_clear(&msg->_extensions);
}
