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

#include "zenoh-pico/protocol/core.h"
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

        rctx->_header = _Z_MID_REPLY_CONTEXT;
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

void _z_msg_clear_declaration_resource(_z_res_decl_t *dcl) { _z_keyexpr_clear(&dcl->_key); }

/*------------------ Forget Resource Declaration ------------------*/
_z_declaration_t _z_msg_make_declaration_forget_resource(_z_zint_t rid) {
    _z_declaration_t decl;

    decl._body._forget_res._rid = rid;

    decl._header = _Z_DECL_FORGET_RESOURCE;

    return decl;
}

void _z_msg_clear_declaration_forget_resource(_z_forget_res_decl_t *dcl) { (void)(dcl); }

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

void _z_msg_clear_declaration_publisher(_z_pub_decl_t *dcl) { _z_keyexpr_clear(&dcl->_key); }

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

void _z_msg_clear_declaration_forget_publisher(_z_forget_pub_decl_t *dcl) { _z_keyexpr_clear(&dcl->_key); }

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

void _z_msg_clear_declaration_subscriber(_z_sub_decl_t *dcl) {
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

void _z_msg_clear_declaration_forget_subscriber(_z_forget_sub_decl_t *dcl) { _z_keyexpr_clear(&dcl->_key); }

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

void _z_msg_clear_declaration_queryable(_z_qle_decl_t *dcl) { _z_keyexpr_clear(&dcl->_key); }

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

void _z_msg_clear_declaration_forget_queryable(_z_forget_qle_decl_t *dcl) { _z_keyexpr_clear(&dcl->_key); }

/*------------------ Declare ------------------*/
_z_zenoh_message_t _z_msg_make_declare(_z_declaration_array_t declarations) {
    _z_zenoh_message_t msg;

    msg._body._declare._declarations = declarations;

    msg._header = _Z_MID_DECLARE;

    msg._attachment = NULL;
    msg._reply_context = NULL;

    return msg;
}

void _z_msg_clear_declaration(_z_declaration_t *dcl) {
    uint8_t did = _Z_MID(dcl->_header);
    switch (did) {
        case _Z_DECL_RESOURCE:
            _z_msg_clear_declaration_resource(&dcl->_body._res);
            break;
        case _Z_DECL_PUBLISHER:
            _z_msg_clear_declaration_publisher(&dcl->_body._pub);
            break;
        case _Z_DECL_SUBSCRIBER:
            _z_msg_clear_declaration_subscriber(&dcl->_body._sub);
            break;
        case _Z_DECL_QUERYABLE:
            _z_msg_clear_declaration_queryable(&dcl->_body._qle);
            break;
        case _Z_DECL_FORGET_RESOURCE:
            _z_msg_clear_declaration_forget_resource(&dcl->_body._forget_res);
            break;
        case _Z_DECL_FORGET_PUBLISHER:
            _z_msg_clear_declaration_forget_publisher(&dcl->_body._forget_pub);
            break;
        case _Z_DECL_FORGET_SUBSCRIBER:
            _z_msg_clear_declaration_forget_subscriber(&dcl->_body._forget_sub);
            break;
        case _Z_DECL_FORGET_QUERYABLE:
            _z_msg_clear_declaration_forget_queryable(&dcl->_body._forget_qle);
            break;
        default:
            _Z_DEBUG("WARNING: Trying to free declaration with unknown ID(%d)\n", did);
            break;
    }
}

void _z_msg_clear_declare(_z_msg_declare_t *msg) { _z_declaration_array_clear(&msg->_declarations); }

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

    msg._header = _Z_MID_DATA;
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

    msg._header = _Z_MID_UNIT;
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

    msg._header = _Z_MID_PULL;
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

    msg._header = _Z_MID_QUERY;
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
        case _Z_MID_DECLARE:
            _z_msg_clear_declare(&msg->_body._declare);
            break;
        case _Z_MID_DATA:
            _z_msg_clear_data(&msg->_body._data);
            break;
        case _Z_MID_PULL:
            _z_msg_clear_pull(&msg->_body._pull);
            break;
        case _Z_MID_QUERY:
            _z_msg_clear_query(&msg->_body._query);
            break;
        case _Z_MID_UNIT:
            _z_msg_clear_unit(&msg->_body._unit);
            break;
        default:
            _Z_DEBUG("WARNING: Trying to clear message with unknown ID(%d)\n", mid);
            break;
    }
}

/*=============================*/
/*     Transport Messages      */
/*=============================*/
/*------------------ Scout Message ------------------*/
_z_transport_message_t _z_t_msg_make_scout(z_whatami_t what, _Bool request_zid) {
    _z_transport_message_t msg;

    msg._body._scout._what = what;

    msg._header = _Z_MID_SCOUT;
    if (request_zid == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_I);
    }

    if (what != Z_WHATAMI_ROUTER) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_W);
    }

    msg._attachment = NULL;

    return msg;
}

void _z_t_msg_copy_scout(_z_t_msg_scout_t *clone, _z_t_msg_scout_t *msg) { clone->_what = msg->_what; }

void _z_t_msg_clear_scout(_z_t_msg_scout_t *msg) {
    // NOTE: scout does not involve any heap allocation
    (void)(msg);
}

/*------------------ Hello Message ------------------*/
_z_transport_message_t _z_t_msg_make_hello(z_whatami_t whatami, _z_bytes_t zid, _z_locator_array_t locators) {
    _z_transport_message_t msg;

    msg._body._hello._whatami = whatami;
    msg._body._hello._zid = zid;
    msg._body._hello._locators = locators;

    msg._header = _Z_MID_HELLO;
    if (whatami != Z_WHATAMI_ROUTER) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_W);
    }
    if (_z_bytes_is_empty(&zid) == false) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_I);
    }
    if (_z_locator_array_is_empty(&locators) == false) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_L);
    }

    msg._attachment = NULL;

    return msg;
}

void _z_t_msg_copy_hello(_z_t_msg_hello_t *clone, _z_t_msg_hello_t *msg) {
    _z_locator_array_copy(&clone->_locators, &msg->_locators);
    _z_bytes_copy(&clone->_zid, &msg->_zid);
    clone->_whatami = msg->_whatami;
}

void _z_t_msg_clear_hello(_z_t_msg_hello_t *msg) {
    _z_bytes_clear(&msg->_zid);
    _z_locators_clear(&msg->_locators);
}

/*------------------ Join Message ------------------*/
_z_transport_message_t _z_t_msg_make_join(uint8_t version, z_whatami_t whatami, _z_zint_t lease,
                                          _z_zint_t sn_resolution, _z_bytes_t zid, _z_conduit_sn_list_t next_sns) {
    _z_transport_message_t msg;

    msg._body._join._options = 0;
    if (next_sns._is_qos == true) {
        _Z_SET_FLAG(msg._body._join._options, _Z_OPT_JOIN_QOS);
    }
    msg._body._join._version = version;
    msg._body._join._whatami = whatami;
    msg._body._join._lease = lease;
    msg._body._join._sn_resolution = sn_resolution;
    msg._body._join._next_sns = next_sns;
    msg._body._join._zid = zid;

    msg._header = _Z_MID_JOIN;
    if ((lease % 1000) == 0) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_T1);
    }
    if (sn_resolution != Z_SN_RESOLUTION) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_S);
    }
    if (msg._body._join._options != 0) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_O);
    }

    msg._attachment = NULL;

    return msg;
}

void _z_t_msg_copy_join(_z_t_msg_join_t *clone, _z_t_msg_join_t *msg) {
    clone->_options = msg->_options;
    clone->_version = msg->_version;
    clone->_whatami = msg->_whatami;
    clone->_lease = msg->_lease;
    clone->_sn_resolution = msg->_sn_resolution;
    clone->_next_sns = msg->_next_sns;
    _z_bytes_copy(&clone->_zid, &msg->_zid);
}

void _z_t_msg_clear_join(_z_t_msg_join_t *msg) { _z_bytes_clear(&msg->_zid); }

/*------------------ Init Message ------------------*/
_z_transport_message_t _z_t_msg_make_init_syn(uint8_t version, z_whatami_t whatami, _z_zint_t sn_resolution,
                                              _z_bytes_t zid, _Bool is_qos) {
    _z_transport_message_t msg;

    msg._body._init._options = 0;
    if (is_qos == true) {
        _Z_SET_FLAG(msg._body._init._options, _Z_OPT_INIT_QOS);
    }
    msg._body._init._version = version;
    msg._body._init._whatami = whatami;
    msg._body._init._sn_resolution = sn_resolution;
    msg._body._init._zid = zid;
    _z_bytes_reset(&msg._body._init._cookie);

    msg._header = _Z_MID_INIT;
    if (sn_resolution != Z_SN_RESOLUTION) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_S);
    }
    if (msg._body._init._options != 0) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_O);
    }

    msg._attachment = NULL;

    return msg;
}

_z_transport_message_t _z_t_msg_make_init_ack(uint8_t version, z_whatami_t whatami, _z_zint_t sn_resolution,
                                              _z_bytes_t zid, _z_bytes_t cookie, _Bool is_qos) {
    _z_transport_message_t msg;

    msg._body._init._options = 0;
    if (is_qos == true) {
        _Z_SET_FLAG(msg._body._init._options, _Z_OPT_INIT_QOS);
    }
    msg._body._init._version = version;
    msg._body._init._whatami = whatami;
    msg._body._init._sn_resolution = sn_resolution;
    msg._body._init._zid = zid;
    msg._body._init._cookie = cookie;

    msg._header = _Z_MID_INIT;
    _Z_SET_FLAG(msg._header, _Z_FLAG_T_A);
    if (sn_resolution != Z_SN_RESOLUTION) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_S);
    }
    if (msg._body._init._options != 0) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_O);
    }

    msg._attachment = NULL;

    return msg;
}

void _z_t_msg_copy_init(_z_t_msg_init_t *clone, _z_t_msg_init_t *msg) {
    clone->_options = msg->_options;
    clone->_version = msg->_version;
    clone->_whatami = msg->_whatami;
    clone->_sn_resolution = msg->_sn_resolution;
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

    msg._body._open._lease = lease;
    msg._body._open._initial_sn = initial_sn;
    msg._body._open._cookie = cookie;

    msg._header = _Z_MID_OPEN;
    if ((lease % 1000) == 0) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_T2);
    }

    msg._attachment = NULL;

    return msg;
}

_z_transport_message_t _z_t_msg_make_open_ack(_z_zint_t lease, _z_zint_t initial_sn) {
    _z_transport_message_t msg;

    msg._body._open._lease = lease;
    msg._body._open._initial_sn = initial_sn;
    _z_bytes_reset(&msg._body._open._cookie);

    msg._header = _Z_MID_OPEN;
    _Z_SET_FLAG(msg._header, _Z_FLAG_T_A);
    if ((lease % 1000) == 0) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_T2);
    }

    msg._attachment = NULL;

    return msg;
}

void _z_t_msg_copy_open(_z_t_msg_open_t *clone, _z_t_msg_open_t *msg) {
    clone->_lease = msg->_lease;
    clone->_initial_sn = msg->_initial_sn;
    _z_bytes_copy(&clone->_cookie, &msg->_cookie);
}

void _z_t_msg_clear_open(_z_t_msg_open_t *msg) { _z_bytes_clear(&msg->_cookie); }

/*------------------ Close Message ------------------*/
_z_transport_message_t _z_t_msg_make_close(uint8_t reason, _z_bytes_t zid, _Bool link_only) {
    _z_transport_message_t msg;

    msg._body._close._reason = reason;
    msg._body._close._zid = zid;

    msg._header = _Z_MID_CLOSE;
    if (_z_bytes_is_empty(&zid) == false) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_I);
    }
    if (link_only == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_K);
    }

    msg._attachment = NULL;

    return msg;
}

void _z_t_msg_copy_close(_z_t_msg_close_t *clone, _z_t_msg_close_t *msg) {
    _z_bytes_copy(&clone->_zid, &msg->_zid);
    clone->_reason = msg->_reason;
}

void _z_t_msg_clear_close(_z_t_msg_close_t *msg) { _z_bytes_clear(&msg->_zid); }

/*------------------ Sync Message ------------------*/
_z_transport_message_t _z_t_msg_make_sync(_z_zint_t sn, _Bool is_reliable, _z_zint_t count) {
    _z_transport_message_t msg;

    msg._body._sync._sn = sn;
    msg._body._sync._count = count;

    msg._header = _Z_MID_SYNC;
    if (is_reliable == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_R);
        if (count != 0) {
            _Z_SET_FLAG(msg._header, _Z_FLAG_T_C);
        }
    }

    msg._attachment = NULL;

    return msg;
}

void _z_t_msg_copy_sync(_z_t_msg_sync_t *clone, _z_t_msg_sync_t *msg) {
    clone->_sn = msg->_sn;
    clone->_count = msg->_count;
}

void _z_t_msg_clear_sync(_z_t_msg_sync_t *msg) {
    // NOTE: sync does not involve any heap allocation
    (void)(msg);
}

/*------------------ AckNack Message ------------------*/
_z_transport_message_t _z_t_msg_make_ack_nack(_z_zint_t sn, _z_zint_t mask) {
    _z_transport_message_t msg;

    msg._body._ack_nack._sn = sn;
    msg._body._ack_nack._mask = mask;

    msg._header = _Z_MID_ACK_NACK;
    if (mask != 0) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_M);
    }

    msg._attachment = NULL;

    return msg;
}

void _z_t_msg_copy_ack_nack(_z_t_msg_ack_nack_t *clone, _z_t_msg_ack_nack_t *msg) {
    clone->_sn = msg->_sn;
    clone->_mask = msg->_mask;
}

void _z_t_msg_clear_ack_nack(_z_t_msg_ack_nack_t *msg) {
    // NOTE: ack_nack does not involve any heap allocation
    (void)(msg);
}

/*------------------ Keep Alive Message ------------------*/
_z_transport_message_t _z_t_msg_make_keep_alive(_z_bytes_t zid) {
    _z_transport_message_t msg;

    msg._body._keep_alive._zid = zid;

    msg._header = _Z_MID_KEEP_ALIVE;
    if (_z_bytes_is_empty(&zid) == false) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_I);
    }

    msg._attachment = NULL;

    return msg;
}

void _z_t_msg_copy_keep_alive(_z_t_msg_keep_alive_t *clone, _z_t_msg_keep_alive_t *msg) {
    _z_bytes_copy(&clone->_zid, &msg->_zid);
}

void _z_t_msg_clear_keep_alive(_z_t_msg_keep_alive_t *msg) { _z_bytes_clear(&msg->_zid); }

/*------------------ PingPong Messages ------------------*/
_z_transport_message_t _z_t_msg_make_ping(_z_zint_t hash) {
    _z_transport_message_t msg;

    msg._body._ping_pong._hash = hash;

    msg._header = _Z_MID_PING_PONG;

    msg._attachment = NULL;

    return msg;
}

_z_transport_message_t _z_t_msg_make_pong(_z_zint_t hash) {
    _z_transport_message_t msg;

    msg._body._ping_pong._hash = hash;

    msg._header = _Z_MID_PING_PONG;
    _Z_SET_FLAG(msg._header, _Z_FLAG_T_P);

    msg._attachment = NULL;

    return msg;
}

void _z_t_msg_copy_ping_pong(_z_t_msg_ping_pong_t *clone, _z_t_msg_ping_pong_t *msg) { clone->_hash = msg->_hash; }

void _z_t_msg_clear_ping_pong(_z_t_msg_ping_pong_t *msg) {
    // NOTE: ping_pong does not involve any heap allocation
    (void)(msg);
}

/*------------------ Frame Message ------------------*/
_z_transport_message_t _z_t_msg_make_frame_header(_z_zint_t sn, _Bool is_reliable, _Bool is_fragment, _Bool is_final) {
    _z_transport_message_t msg;

    msg._body._frame._sn = sn;

    // Reset payload content
    (void)memset(&msg._body._frame._payload, 0, sizeof(_z_frame_payload_t));

    msg._header = _Z_MID_FRAME;
    if (is_reliable == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_R);
    }
    if (is_fragment == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_F);
        if (is_final == true) {
            _Z_SET_FLAG(msg._header, _Z_FLAG_T_E);
        }
    }

    msg._attachment = NULL;

    return msg;
}

_z_transport_message_t _z_t_msg_make_frame(_z_zint_t sn, _z_frame_payload_t payload, _Bool is_reliable,
                                           _Bool is_fragment, _Bool is_final) {
    _z_transport_message_t msg;

    msg._body._frame._sn = sn;
    msg._body._frame._payload = payload;

    msg._header = _Z_MID_FRAME;
    if (is_reliable == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_R);
    }
    if (is_fragment == true) {
        _Z_SET_FLAG(msg._header, _Z_FLAG_T_F);
        if (is_final == true) {
            _Z_SET_FLAG(msg._header, _Z_FLAG_T_E);
        }
    }

    msg._attachment = NULL;

    return msg;
}

void _z_t_msg_copy_frame(_z_t_msg_frame_t *clone, _z_t_msg_frame_t *msg, uint8_t header) {
    clone->_sn = msg->_sn;
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_F) == true) {
        _z_bytes_copy(&clone->_payload._fragment, &msg->_payload._fragment);
    } else {
        _z_zenoh_message_vec_copy(&clone->_payload._messages, &msg->_payload._messages);
    }
}

void _z_t_msg_clear_frame(_z_t_msg_frame_t *msg, uint8_t header) {
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_F) == true) {
        _z_payload_clear(&msg->_payload._fragment);
    } else {
        _z_zenoh_message_vec_clear(&msg->_payload._messages);
    }
}

/*------------------ Transport Message ------------------*/
void _z_t_msg_copy(_z_transport_message_t *clone, _z_transport_message_t *msg) {
    clone->_header = msg->_header;
    clone->_attachment = msg->_attachment;

    uint8_t mid = _Z_MID(msg->_header);
    switch (mid) {
        case _Z_MID_SCOUT: {
            _z_t_msg_copy_scout(&clone->_body._scout, &msg->_body._scout);
        } break;

        case _Z_MID_HELLO: {
            _z_t_msg_copy_hello(&clone->_body._hello, &msg->_body._hello);
        } break;

        case _Z_MID_JOIN: {
            _z_t_msg_copy_join(&clone->_body._join, &msg->_body._join);
        } break;

        case _Z_MID_INIT: {
            _z_t_msg_copy_init(&clone->_body._init, &msg->_body._init);
        } break;

        case _Z_MID_OPEN: {
            _z_t_msg_copy_open(&clone->_body._open, &msg->_body._open);
        } break;

        case _Z_MID_CLOSE: {
            _z_t_msg_copy_close(&clone->_body._close, &msg->_body._close);
        } break;

        case _Z_MID_SYNC: {
            _z_t_msg_copy_sync(&clone->_body._sync, &msg->_body._sync);
        } break;

        case _Z_MID_ACK_NACK: {
            _z_t_msg_copy_ack_nack(&clone->_body._ack_nack, &msg->_body._ack_nack);
        } break;

        case _Z_MID_KEEP_ALIVE: {
            _z_t_msg_copy_keep_alive(&clone->_body._keep_alive, &msg->_body._keep_alive);
        } break;

        case _Z_MID_PING_PONG: {
            _z_t_msg_copy_ping_pong(&clone->_body._ping_pong, &msg->_body._ping_pong);
        } break;

        case _Z_MID_FRAME: {
            _z_t_msg_copy_frame(&clone->_body._frame, &msg->_body._frame, clone->_header);
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to free session message with unknown ID(%d)\n", mid);
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
        case _Z_MID_SCOUT: {
            _z_t_msg_clear_scout(&msg->_body._scout);
        } break;

        case _Z_MID_HELLO: {
            _z_t_msg_clear_hello(&msg->_body._hello);
        } break;

        case _Z_MID_JOIN: {
            _z_t_msg_clear_join(&msg->_body._join);
        } break;

        case _Z_MID_INIT: {
            _z_t_msg_clear_init(&msg->_body._init);
        } break;

        case _Z_MID_OPEN: {
            _z_t_msg_clear_open(&msg->_body._open);
        } break;

        case _Z_MID_CLOSE: {
            _z_t_msg_clear_close(&msg->_body._close);
        } break;

        case _Z_MID_SYNC: {
            _z_t_msg_clear_sync(&msg->_body._sync);
        } break;

        case _Z_MID_ACK_NACK: {
            _z_t_msg_clear_ack_nack(&msg->_body._ack_nack);
        } break;

        case _Z_MID_KEEP_ALIVE: {
            _z_t_msg_clear_keep_alive(&msg->_body._keep_alive);
        } break;

        case _Z_MID_PING_PONG: {
            _z_t_msg_clear_ping_pong(&msg->_body._ping_pong);
        } break;

        case _Z_MID_FRAME: {
            _z_t_msg_clear_frame(&msg->_body._frame, msg->_header);
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to free session message with unknown ID(%d)\n", mid);
        } break;
    }
}
