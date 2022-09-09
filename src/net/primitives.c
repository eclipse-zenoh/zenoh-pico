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

#include <stddef.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/net/primitives.h"
#include "zenoh-pico/net/memory.h"
#include "zenoh-pico/net/logger.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/protocol/keyexpr.h"

/*------------------ Scouting ------------------*/
_z_hello_array_t _z_scout(const _z_zint_t what, const _z_config_t *config, uint32_t timeout) {
    return _z_scout_inner(what, config, timeout, 0);
}

/*------------------ Resource Declaration ------------------*/
_z_zint_t _z_declare_resource(_z_session_t *zn, _z_keyexpr_t keyexpr) {
    // FIXME: remove when resource declaration is implemented for multicast transport
#if Z_MULTICAST_TRANSPORT == 1
    if (zn->_tp->_type == _Z_TRANSPORT_MULTICAST_TYPE) {
        goto ERR_1;
    }
#endif  // Z_MULTICAST_TRANSPORT == 1

    _z_resource_t *r = (_z_resource_t *)z_malloc(sizeof(_z_resource_t));
    r->_id = _z_get_resource_id(zn);
    r->_key = _z_keyexpr_duplicate(&keyexpr);
    if (_z_register_resource(zn, _Z_RESOURCE_IS_LOCAL, r) < 0) {
        goto ERR_2;
    }

    // Build the declare message to send on the wire
    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations._val[0] = _z_msg_make_declaration_resource(r->_id, _z_keyexpr_duplicate(&keyexpr));
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);
    if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0) {
        goto ERR_3;
    }
    _z_msg_clear(&z_msg);

    return r->_id;

ERR_3:
    _z_msg_clear(&z_msg);

ERR_2:
    _z_resource_clear(r);
    z_free(r);

ERR_1:
    return Z_RESOURCE_ID_NONE;
}

int8_t _z_undeclare_resource(_z_session_t *zn, const _z_zint_t rid) {
    _z_resource_t *r = _z_get_resource_by_id(zn, _Z_RESOURCE_IS_LOCAL, rid);
    if (r == NULL) {
        goto ERR_1;
    }

    // Build the declare message to send on the wire
    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations._val[0] = _z_msg_make_declaration_forget_resource(rid);
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);
    if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0) {
        goto ERR_2;
    }
    _z_msg_clear(&z_msg);

    // Only if message is successfully send, local resource state can be removed
    _z_unregister_resource(zn, _Z_RESOURCE_IS_LOCAL, r);

    return 0;

ERR_2:
    _z_msg_clear(&z_msg);

ERR_1:
    return -1;
}

/*------------------  Publisher Declaration ------------------*/
_z_publisher_t *_z_declare_publisher(_z_session_t *zn, _z_keyexpr_t keyexpr, int8_t local_routing,
                                     z_congestion_control_t congestion_control, z_priority_t priority) {
    // Build the declare message to send on the wire
    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations._val[0] = _z_msg_make_declaration_publisher(_z_keyexpr_duplicate(&keyexpr));
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);
    if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0) {
        goto ERR_1;
    }
    _z_msg_clear(&z_msg);

    _z_publisher_t *pub = (_z_publisher_t *)z_malloc(sizeof(_z_publisher_t));
    pub->_zn = zn;
    pub->_key = _z_keyexpr_duplicate(&keyexpr);
    pub->_id = _z_get_entity_id(zn);
    pub->_local_routing = local_routing;
    pub->_congestion_control = congestion_control;
    pub->_priority = priority;

    return pub;

ERR_1:
    _z_msg_clear(&z_msg);
    return NULL;
}

int8_t _z_undeclare_publisher(_z_publisher_t *pub) {
    // Build the declare message to send on the wire
    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations._val[0] = _z_msg_make_declaration_forget_publisher(_z_keyexpr_duplicate(&pub->_key));
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);
    if (_z_send_z_msg(pub->_zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0) {
        goto ERR_1;
    }
    _z_msg_clear(&z_msg);

    return 0;

ERR_1:
    _z_msg_clear(&z_msg);
    return -1;
}

/*------------------ Subscriber Declaration ------------------*/
_z_subscriber_t *_z_declare_subscriber(_z_session_t *zn, _z_keyexpr_t keyexpr, _z_subinfo_t sub_info,
                                       _z_data_handler_t callback, _z_drop_handler_t dropper, void *arg) {
    _z_subscription_t s;
    s._id = _z_get_entity_id(zn);
    s._key = _z_get_expanded_key_from_key(zn, _Z_RESOURCE_IS_LOCAL, &keyexpr);
    s._info = sub_info;
    s._callback = callback;
    s._dropper = dropper;
    s._arg = arg;
    if (_z_register_subscription(zn, _Z_RESOURCE_IS_LOCAL, &s) < 0) {
        goto ERR_1;
    }

    // Build the declare message to send on the wire
    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations._val[0] = _z_msg_make_declaration_subscriber(_z_keyexpr_duplicate(&keyexpr), sub_info);
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);
    if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0) {
        goto ERR_2;
    }
    _z_msg_clear(&z_msg);

    _z_subscriber_t *subscriber = (_z_subscriber_t *)z_malloc(sizeof(_z_subscriber_t));
    subscriber->_zn = zn;
    subscriber->_id = s._id;

    return subscriber;

ERR_2:
    _z_msg_clear(&z_msg);

ERR_1:
    _z_subscription_clear(&s);
    return NULL;
}

int8_t _z_undeclare_subscriber(_z_subscriber_t *sub) {
    _z_subscription_sptr_t *s = _z_get_subscription_by_id(sub->_zn, _Z_RESOURCE_IS_LOCAL, sub->_id);
    if (s == NULL) {
        goto ERR_1;
    }

    // Build the declare message to send on the wire
    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations._val[0] = _z_msg_make_declaration_forget_subscriber(_z_keyexpr_duplicate(&s->ptr->_key));
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);
    if (_z_send_z_msg(sub->_zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0) {
        goto ERR_2;
    }
    _z_msg_clear(&z_msg);

    // Only if message is successfully send, local subscription state can be removed
    _z_unregister_subscription(sub->_zn, _Z_RESOURCE_IS_LOCAL, s);

ERR_2:
    _z_msg_clear(&z_msg);

ERR_1:
    return -1;
}

/*------------------ Queryable Declaration ------------------*/
_z_queryable_t *_z_declare_queryable(_z_session_t *zn, _z_keyexpr_t keyexpr, bool complete,
                                     _z_questionable_handler_t callback, _z_drop_handler_t dropper, void *arg) {
    _z_questionable_t q;
    q._id = _z_get_entity_id(zn);
    q._key = _z_get_expanded_key_from_key(zn, _Z_RESOURCE_IS_LOCAL, &keyexpr);
    q._complete = complete;
    q._callback = callback;
    q._dropper = dropper;
    q._arg = arg;
    if (_z_register_questionable(zn, &q) < 0) {
        goto ERR_1;
    }

    // Build the declare message to send on the wire
    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations._val[0] =
        _z_msg_make_declaration_queryable(_z_keyexpr_duplicate(&keyexpr), q._complete, _Z_QUERYABLE_DISTANCE_DEFAULT);
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);
    if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0) {
        goto ERR_2;
    }
    _z_msg_clear(&z_msg);

    _z_queryable_t *queryable = (_z_queryable_t *)z_malloc(sizeof(_z_queryable_t));
    queryable->_zn = zn;
    queryable->_id = q._id;

    return queryable;

ERR_2:
    _z_msg_clear(&z_msg);

ERR_1:
    _z_questionable_clear(&q);
    return NULL;
}

int8_t _z_undeclare_queryable(_z_queryable_t *qle) {
    _z_questionable_sptr_t *q = _z_get_questionable_by_id(qle->_zn, qle->_id);
    if (q == NULL) {
        goto ERR_1;
    }

    // Build the declare message to send on the wire
    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations._val[0] = _z_msg_make_declaration_forget_queryable(_z_keyexpr_duplicate(&q->ptr->_key));
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);
    if (_z_send_z_msg(qle->_zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0) {
        goto ERR_2;
    }
    _z_msg_clear(&z_msg);

    // Only if message is successfully send, local queryable state can be removed
    _z_unregister_questionable(qle->_zn, q);

    return 0;

ERR_2:
    _z_msg_clear(&z_msg);

ERR_1:
    return -1;
}

int8_t _z_send_reply(const z_query_t *query, _z_keyexpr_t keyexpr, const uint8_t *payload, const size_t len) {
    // Build the reply context decorator. This is NOT the final reply._
    _z_bytes_t pid = _z_bytes_wrap(((_z_session_t *)query->_zn)->_tp_manager->_local_pid.start,
                                   ((_z_session_t *)query->_zn)->_tp_manager->_local_pid.len);
    _z_reply_context_t *rctx = _z_msg_make_reply_context(query->_qid, pid, 0);

    // Empty data info
    _z_data_info_t di;
    di._flags = 0;

    // Payload
    _z_payload_t pld;
    pld.len = len;
    pld.start = payload;

    // Congestion control
    int can_be_dropped = 0;

    _z_zenoh_message_t z_msg = _z_msg_make_reply(keyexpr, di, pld, can_be_dropped, rctx);

    int8_t ret = _z_send_z_msg(query->_zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);

    z_free(rctx);

    return ret;
}

/*------------------ Write ------------------*/
int8_t _z_write(_z_session_t *zn, const _z_keyexpr_t keyexpr, const uint8_t *payload, const size_t len) {
    // Empty data info
    _z_data_info_t info;
    info._flags = 0;

    // Payload
    _z_payload_t pld;
    pld.len = len;
    pld.start = payload;

    // Congestion control
    int can_be_dropped = Z_CONGESTION_CONTROL_DROP;

    _z_zenoh_message_t z_msg = _z_msg_make_data(keyexpr, info, pld, can_be_dropped);

    return _z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_DROP);
}

int8_t _z_write_ext(_z_session_t *zn, const _z_keyexpr_t keyexpr, const uint8_t *payload, const size_t len,
                    const _z_encoding_t encoding, const z_sample_kind_t kind, const z_congestion_control_t cong_ctrl) {
    // @TODO: accept `const z_priority_t priority` as additional parameter
    // Data info
    _z_data_info_t info;
    info._flags = 0;
    info._encoding = encoding;
    _Z_SET_FLAG(info._flags, _Z_DATA_INFO_ENC);
    info._kind = kind;
    _Z_SET_FLAG(info._flags, _Z_DATA_INFO_KIND);

    // Payload
    _z_payload_t pld;
    pld.len = len;
    pld.start = payload;

    // Congestion control
    int can_be_dropped = cong_ctrl == Z_CONGESTION_CONTROL_DROP;

    _z_zenoh_message_t z_msg = _z_msg_make_data(keyexpr, info, pld, can_be_dropped);

    return _z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, cong_ctrl);
}

/*------------------ Query ------------------*/
int8_t _z_query(_z_session_t *zn, _z_keyexpr_t keyexpr, const char *parameters, const z_query_target_t target,
                const z_consolidation_mode_t consolidation, _z_reply_handler_t callback, void *arg_call,
                _z_drop_handler_t dropper, void *arg_drop) {
    // Create the pending query object
    _z_pending_query_t *pq = (_z_pending_query_t *)z_malloc(sizeof(_z_pending_query_t));
    pq->_id = _z_get_query_id(zn);
    pq->_key = _z_get_expanded_key_from_key(zn, _Z_RESOURCE_IS_LOCAL, &keyexpr);
    pq->_parameters = _z_str_clone(parameters);
    pq->_target = target;
    pq->_consolidation = consolidation;
    pq->_callback = callback;
    pq->_dropper = dropper;
    pq->_pending_replies = NULL;
    pq->_call_arg = arg_call;
    pq->_drop_arg = arg_drop;

    // Add the pending query to the current session
    _z_register_pending_query(zn, pq);

    _z_zenoh_message_t z_msg = _z_msg_make_query(keyexpr, pq->_parameters, pq->_id, pq->_target, pq->_consolidation);

    return _z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);
}

void _z_reply_collect_handler(const _z_reply_t *reply, const void *arg) {
    _z_pending_query_collect_t *pqc = (_z_pending_query_collect_t *)arg;
    if (reply->_tag == Z_REPLY_TAG_DATA) {
        _z_reply_data_t *rd = (_z_reply_data_t *)z_malloc(sizeof(_z_reply_data_t));
        _z_bytes_copy(&rd->replier_id, &reply->data.replier_id);
        rd->sample.keyexpr = _z_keyexpr_duplicate(&reply->data.sample.keyexpr);
        _z_bytes_copy(&rd->sample.payload, &reply->data.sample.payload);
        rd->sample.encoding.prefix = reply->data.sample.encoding.prefix;
        _z_bytes_copy(&rd->sample.encoding.suffix, &reply->data.sample.encoding.suffix);
        rd->sample.kind = reply->data.sample.kind;
        rd->sample.timestamp = _z_timestamp_duplicate(&reply->data.sample.timestamp);

        pqc->_replies = _z_reply_data_list_push(pqc->_replies, rd);
    } else {
#if Z_MULTI_THREAD == 1
        // Signal that we have received all the replies
        _z_mutex_lock(&pqc->_mutex);  // Avoid condvar signal to be triggered before wait
        _z_condvar_signal(&pqc->_cond_var);
        _z_mutex_unlock(&pqc->_mutex);
#endif  // Z_MULTI_THREAD == 1
    }
}

/*------------------ Pull ------------------*/
int8_t _z_subscriber_pull(const _z_subscriber_t *sub) {
    _z_subscription_sptr_t *s = _z_get_subscription_by_id(sub->_zn, _Z_RESOURCE_IS_LOCAL, sub->_id);
    if (s == NULL) { return -1; }

    _z_zint_t pull_id = _z_get_pull_id(sub->_zn);
    _z_zint_t max_samples = 0;  // @TODO: get the correct value for max_sample
    int is_final = 1;

    _z_zenoh_message_t z_msg = _z_msg_make_pull(s->ptr->_key, pull_id, max_samples, is_final);

    return _z_send_z_msg(sub->_zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);
}
