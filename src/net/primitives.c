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

#include "zenoh-pico/net/primitives.h"

#include <stddef.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/net/logger.h"
#include "zenoh-pico/net/memory.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/utils.h"

/*------------------ Scouting ------------------*/
void _z_scout(const z_whatami_t what, const char *locator, const uint32_t timeout, _z_hello_handler_t callback,
              void *arg_call, _z_drop_handler_t dropper, void *arg_drop) {
    _z_hello_list_t *hellos = _z_scout_inner(what, locator, timeout, false);

    while (hellos != NULL) {
        _z_hello_t *hello = NULL;
        hellos = _z_hello_list_pop(hellos, &hello);
        (*callback)(hello, arg_call); // callback takes ownership of hello
    }

    if (dropper != NULL) {
        (*dropper)(arg_drop);
    }
}

/*------------------ Resource Declaration ------------------*/
_z_zint_t _z_declare_resource(_z_session_t *zn, _z_keyexpr_t keyexpr) {
    _z_zint_t ret = Z_RESOURCE_ID_NONE;

    if (zn->_tp._type ==
        _Z_TRANSPORT_UNICAST_TYPE) {  // FIXME: remove when resource declaration is implemented for multicast transport

        _z_resource_t *r = (_z_resource_t *)z_malloc(sizeof(_z_resource_t));
        if (r != NULL) {
            r->_id = _z_get_resource_id(zn);
            r->_key = _z_keyexpr_duplicate(&keyexpr);
            if (_z_register_resource(zn, _Z_RESOURCE_IS_LOCAL, r) == _Z_RES_OK) {
                // Build the declare message to send on the wire
                _z_declaration_array_t declarations = _z_declaration_array_make(1);
                declarations._val[0] = _z_msg_make_declaration_resource(r->_id, _z_keyexpr_duplicate(&keyexpr));
                _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);
                if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) == _Z_RES_OK) {
                    ret = r->_id;
                } else {
                    _z_unregister_resource(zn, _Z_RESOURCE_IS_LOCAL, r);
                }
                _z_msg_clear(&z_msg);
            } else {
                _z_resource_free(&r);
            }
        }
    }

    return ret;
}

int8_t _z_undeclare_resource(_z_session_t *zn, const _z_zint_t rid) {
    int8_t ret = _Z_RES_OK;

    _z_resource_t *r = _z_get_resource_by_id(zn, _Z_RESOURCE_IS_LOCAL, rid);
    if (r != NULL) {
        // Build the declare message to send on the wire
        _z_declaration_array_t declarations = _z_declaration_array_make(1);
        declarations._val[0] = _z_msg_make_declaration_forget_resource(rid);
        _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);
        if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) == _Z_RES_OK) {
            _z_unregister_resource(zn, _Z_RESOURCE_IS_LOCAL, r);  // Only if message is send, local resource is removed
        } else {
            ret = _Z_ERR_TRANSPORT_TX_FAILED;
        }
        _z_msg_clear(&z_msg);
    } else {
        ret = _Z_ERR_KEYEXPR_UNKNOWN;
    }

    return ret;
}

/*------------------  Publisher Declaration ------------------*/
_z_publisher_t *_z_declare_publisher(_z_session_t *zn, _z_keyexpr_t keyexpr, z_congestion_control_t congestion_control,
                                     z_priority_t priority) {
    _z_publisher_t *ret = (_z_publisher_t *)z_malloc(sizeof(_z_publisher_t));
    if (ret != NULL) {
        ret->_zn = zn;
        ret->_key = _z_keyexpr_duplicate(&keyexpr);
        ret->_id = _z_get_entity_id(zn);
        ret->_congestion_control = congestion_control;
        ret->_priority = priority;

        // Build the declare message to send on the wire
        _z_declaration_array_t declarations = _z_declaration_array_make(1);
        declarations._val[0] = _z_msg_make_declaration_publisher(_z_keyexpr_duplicate(&keyexpr));
        _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);
        if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
            // ret = _Z_ERR_TRANSPORT_TX_FAILED;
            _z_publisher_free(&ret);
        }
        _z_msg_clear(&z_msg);
    }

    return ret;
}

int8_t _z_undeclare_publisher(_z_publisher_t *pub) {
    int8_t ret = _Z_ERR_GENERIC;

    if (pub != NULL) {
        ret = _Z_RES_OK;

        // Build the declare message to send on the wire
        _z_declaration_array_t declarations = _z_declaration_array_make(1);
        declarations._val[0] = _z_msg_make_declaration_forget_publisher(_z_keyexpr_duplicate(&pub->_key));
        _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);
        if (_z_send_z_msg(pub->_zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
            ret = _Z_ERR_TRANSPORT_TX_FAILED;
        }
        _z_msg_clear(&z_msg);
    }

    return ret;
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

    _z_subscriber_t *ret = (_z_subscriber_t *)z_malloc(sizeof(_z_subscriber_t));
    if (ret != NULL) {
        ret->_zn = zn;
        ret->_id = s._id;

        _z_subscription_sptr_t *sp_s = _z_register_subscription(
            zn, _Z_RESOURCE_IS_LOCAL, &s);  // This a pointer to the entry stored at session-level.
                                            // Do not drop it by the end of this function.
        if (sp_s != NULL) {
            // Build the declare message to send on the wire
            _z_declaration_array_t declarations = _z_declaration_array_make(1);
            declarations._val[0] = _z_msg_make_declaration_subscriber(_z_keyexpr_duplicate(&keyexpr), sub_info);
            _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);
            if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
                // ret = _Z_ERR_TRANSPORT_TX_FAILED;
                _z_unregister_subscription(zn, _Z_RESOURCE_IS_LOCAL, sp_s);
                _z_subscriber_free(&ret);
            }
            _z_msg_clear(&z_msg);
        } else {
            _z_subscriber_free(&ret);
        }
    } else {
        _z_subscription_clear(&s);
    }

    return ret;
}

int8_t _z_undeclare_subscriber(_z_subscriber_t *sub) {
    int8_t ret = _Z_ERR_GENERIC;

    if (sub != NULL) {
        ret = _Z_RES_OK;

        _z_subscription_sptr_t *s = _z_get_subscription_by_id(sub->_zn, _Z_RESOURCE_IS_LOCAL, sub->_id);
        if (s != NULL) {
            // Build the declare message to send on the wire
            _z_declaration_array_t declarations = _z_declaration_array_make(1);
            declarations._val[0] = _z_msg_make_declaration_forget_subscriber(_z_keyexpr_duplicate(&s->ptr->_key));
            _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);
            if (_z_send_z_msg(sub->_zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) == _Z_RES_OK) {
                // Only if message is successfully send, local subscription state can be removed
                _z_unregister_subscription(sub->_zn, _Z_RESOURCE_IS_LOCAL, s);
            } else {
                ret = _Z_ERR_TRANSPORT_TX_FAILED;
            }
            _z_msg_clear(&z_msg);
        } else {
            ret = _Z_ERR_ENTITY_UNKNOWN;
        }
    }
    return ret;
}

/*------------------ Queryable Declaration ------------------*/
_z_queryable_t *_z_declare_queryable(_z_session_t *zn, _z_keyexpr_t keyexpr, _Bool complete,
                                     _z_questionable_handler_t callback, _z_drop_handler_t dropper, void *arg) {
    _z_questionable_t q;
    q._id = _z_get_entity_id(zn);
    q._key = _z_get_expanded_key_from_key(zn, _Z_RESOURCE_IS_LOCAL, &keyexpr);
    q._complete = complete;
    q._callback = callback;
    q._dropper = dropper;
    q._arg = arg;

    _z_queryable_t *ret = (_z_queryable_t *)z_malloc(sizeof(_z_queryable_t));
    if (ret != NULL) {
        ret->_zn = zn;
        ret->_id = q._id;

        _z_questionable_sptr_t *sp_q =
            _z_register_questionable(zn, &q);  // This a pointer to the entry stored at session-level.
                                               // Do not drop it by the end of this function.
        if (sp_q != NULL) {
            // Build the declare message to send on the wire
            _z_declaration_array_t declarations = _z_declaration_array_make(1);
            declarations._val[0] = _z_msg_make_declaration_queryable(_z_keyexpr_duplicate(&keyexpr), q._complete,
                                                                     _Z_QUERYABLE_DISTANCE_DEFAULT);
            _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);
            if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
                // ret = _Z_ERR_TRANSPORT_TX_FAILED;
                _z_unregister_questionable(zn, sp_q);
                _z_queryable_free(&ret);
            }
            _z_msg_clear(&z_msg);
        } else {
            _z_queryable_free(&ret);
        }
    } else {
        _z_questionable_clear(&q);
    }

    return ret;
}

int8_t _z_undeclare_queryable(_z_queryable_t *qle) {
    int8_t ret = _Z_ERR_GENERIC;

    if (qle != NULL) {
        _z_questionable_sptr_t *q = _z_get_questionable_by_id(qle->_zn, qle->_id);
        if (q != NULL) {
            ret = _Z_RES_OK;
            // Build the declare message to send on the wire
            _z_declaration_array_t declarations = _z_declaration_array_make(1);
            declarations._val[0] = _z_msg_make_declaration_forget_queryable(_z_keyexpr_duplicate(&q->ptr->_key));
            _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);
            if (_z_send_z_msg(qle->_zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) == _Z_RES_OK) {
                // Only if message is successfully send, local queryable state can be removed
                _z_unregister_questionable(qle->_zn, q);
            } else {
                ret = _Z_ERR_TRANSPORT_TX_FAILED;
            }
            _z_msg_clear(&z_msg);
        } else {
            ret = _Z_ERR_ENTITY_UNKNOWN;
        }
    }

    return ret;
}

int8_t _z_send_reply(const z_query_t *query, _z_keyexpr_t keyexpr, const uint8_t *payload, const size_t len) {
    int8_t ret = _Z_RES_OK;

    _z_keyexpr_t q_ke;
    _z_keyexpr_t r_ke;
    if (query->_anyke == false) {
        q_ke = _z_get_expanded_key_from_key(query->_zn, _Z_RESOURCE_IS_LOCAL, &query->_key);
        r_ke = _z_get_expanded_key_from_key(query->_zn, _Z_RESOURCE_IS_LOCAL, &keyexpr);
        if (_z_keyexpr_intersects(q_ke._suffix, strlen(q_ke._suffix), r_ke._suffix, strlen(r_ke._suffix)) == false) {
            ret = _Z_ERR_KEYEXPR_NOT_MATCH;
        }
        _z_keyexpr_clear(&q_ke);
        _z_keyexpr_clear(&r_ke);
    }

    if (ret == _Z_RES_OK) {
        // Build the reply context decorator. This is NOT the final reply.
        _z_bytes_t zid =
            _z_bytes_wrap(((_z_session_t *)query->_zn)->_local_zid.start, ((_z_session_t *)query->_zn)->_local_zid.len);
        _z_reply_context_t *rctx = _z_msg_make_reply_context(query->_qid, zid, false);

        _z_data_info_t di = {._flags = 0};                  // Empty data info
        _z_payload_t pld = {.len = len, .start = payload};  // Payload
        _Bool can_be_dropped = false;                       // Congestion control
        _z_zenoh_message_t z_msg = _z_msg_make_reply(keyexpr, di, pld, can_be_dropped, rctx);

        if (_z_send_z_msg(query->_zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
            ret = _Z_ERR_TRANSPORT_TX_FAILED;
        }

        z_free(rctx);
    }

    return ret;
}

/*------------------ Write ------------------*/
int8_t _z_write(_z_session_t *zn, const _z_keyexpr_t keyexpr, const uint8_t *payload, const size_t len,
                const _z_encoding_t encoding, const z_sample_kind_t kind, const z_congestion_control_t cong_ctrl) {
    int8_t ret = _Z_RES_OK;

    // @TODO: accept `const z_priority_t priority` as additional parameter

    // Data info
    _z_data_info_t info = {._flags = 0, ._encoding = encoding, ._kind = kind};
    _Z_SET_FLAG(info._flags, _Z_DATA_INFO_ENC);
    _Z_SET_FLAG(info._flags, _Z_DATA_INFO_KIND);

    _z_payload_t pld = {.len = len, .start = payload};              // Payload
    _Bool can_be_dropped = cong_ctrl == Z_CONGESTION_CONTROL_DROP;  // Congestion control
    _z_zenoh_message_t z_msg = _z_msg_make_data(keyexpr, info, pld, can_be_dropped);

    if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, cong_ctrl) != _Z_RES_OK) {
        ret = _Z_ERR_TRANSPORT_TX_FAILED;
    }

    return ret;
}

/*------------------ Query ------------------*/
int8_t _z_query(_z_session_t *zn, _z_keyexpr_t keyexpr, const char *parameters, const z_query_target_t target,
                const z_consolidation_mode_t consolidation, _z_value_t value, _z_reply_handler_t callback,
                void *arg_call, _z_drop_handler_t dropper, void *arg_drop) {
    int8_t ret = _Z_RES_OK;

    // Create the pending query object
    _z_pending_query_t *pq = (_z_pending_query_t *)z_malloc(sizeof(_z_pending_query_t));
    if (pq != NULL) {
        pq->_id = _z_get_query_id(zn);
        pq->_key = _z_get_expanded_key_from_key(zn, _Z_RESOURCE_IS_LOCAL, &keyexpr);
        pq->_parameters = _z_str_clone(parameters);
        pq->_target = target;
        pq->_consolidation = consolidation;
        pq->_anykey = (strstr(pq->_parameters, Z_SELECTOR_QUERY_MATCH) == NULL) ? false : true;
        pq->_callback = callback;
        pq->_dropper = dropper;
        pq->_pending_replies = NULL;
        pq->_call_arg = arg_call;
        pq->_drop_arg = arg_drop;

        ret = _z_register_pending_query(zn, pq);  // Add the pending query to the current session
        if (ret == _Z_RES_OK) {
            _z_zenoh_message_t z_msg =
                _z_msg_make_query(keyexpr, pq->_parameters, pq->_id, pq->_target, pq->_consolidation, value);

            if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
                _z_unregister_pending_query(zn, pq);
                ret = _Z_ERR_TRANSPORT_TX_FAILED;
            }
        } else {
            _z_pending_query_clear(pq);
        }
    }

    return ret;
}

/*------------------ Pull ------------------*/
int8_t _z_subscriber_pull(const _z_subscriber_t *sub) {
    int8_t ret = _Z_RES_OK;

    _z_subscription_sptr_t *s = _z_get_subscription_by_id(sub->_zn, _Z_RESOURCE_IS_LOCAL, sub->_id);
    if (s != NULL) {
        _z_zint_t pull_id = _z_get_pull_id(sub->_zn);
        _z_zint_t max_samples = 0;  // @TODO: get the correct value for max_sample
        _Bool is_final = true;
        _z_zenoh_message_t z_msg = _z_msg_make_pull(s->ptr->_key, pull_id, max_samples, is_final);

        if (_z_send_z_msg(sub->_zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
            ret = _Z_ERR_TRANSPORT_TX_FAILED;
        }
    } else {
        ret = _Z_ERR_ENTITY_UNKNOWN;
    }

    return ret;
}
