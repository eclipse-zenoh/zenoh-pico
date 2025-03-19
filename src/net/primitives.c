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
#include <stdint.h>
#include <string.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/net/filtering.h"
#include "zenoh-pico/net/logger.h"
#include "zenoh-pico/net/matching.h"
#include "zenoh-pico/net/sample.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/declarations.h"
#include "zenoh-pico/protocol/definitions/interest.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/interest.h"
#include "zenoh-pico/session/liveliness.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"

/*------------------ Declaration Helpers ------------------*/
z_result_t _z_send_declare(_z_session_t *zn, const _z_network_message_t *n_msg) {
    z_result_t ret = _Z_RES_OK;
    ret = _z_send_n_msg(zn, n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);

#if Z_FEATURE_AUTO_RECONNECT == 1
    if (ret == _Z_RES_OK) {
        _z_cache_declaration(zn, n_msg);
    }
#endif

    return ret;
}

z_result_t _z_send_undeclare(_z_session_t *zn, const _z_network_message_t *n_msg) {
    z_result_t ret = _Z_RES_OK;
    ret = _z_send_n_msg(zn, n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);

#if Z_FEATURE_AUTO_RECONNECT == 1
    if (ret == _Z_RES_OK) {
        _z_prune_declaration(zn, n_msg);
    }
#endif

    return ret;
}
/*------------------ Scouting ------------------*/
void _z_scout(const z_what_t what, const _z_id_t zid, _z_string_t *locator, const uint32_t timeout,
              _z_closure_hello_callback_t callback, void *arg_call, _z_drop_handler_t dropper, void *arg_drop) {
    _z_hello_list_t *hellos = _z_scout_inner(what, zid, locator, timeout, false);

    while (hellos != NULL) {
        _z_hello_t *hello = NULL;
        hellos = _z_hello_list_pop(hellos, &hello);
        (*callback)(hello, arg_call);
        _z_hello_free(&hello);
    }

    if (dropper != NULL) {
        (*dropper)(arg_drop);
    }
    _z_hello_list_free(&hellos);
}

/*------------------ Resource Declaration ------------------*/
uint16_t _z_declare_resource(_z_session_t *zn, const _z_keyexpr_t *keyexpr) {
    uint16_t ret = Z_RESOURCE_ID_NONE;

    // TODO: Implement interest protocol for multicast transports, unicast p2p
    if (zn->_mode == Z_WHATAMI_CLIENT) {
        uint16_t id = _z_register_resource(zn, keyexpr, 0, _Z_KEYEXPR_MAPPING_LOCAL);
        if (id != 0) {
            // Build the declare message to send on the wire
            _z_keyexpr_t alias = _z_keyexpr_alias(keyexpr);
            _z_declaration_t declaration = _z_make_decl_keyexpr(id, &alias);
            _z_network_message_t n_msg = _z_n_msg_make_declare(declaration, false, 0);
            if (_z_send_declare(zn, &n_msg) == _Z_RES_OK) {
                ret = id;
                // Invalidate cache
                _z_subscription_cache_invalidate(zn);
                _z_queryable_cache_invalidate(zn);
            } else {
                _z_unregister_resource(zn, id, _Z_KEYEXPR_MAPPING_LOCAL);
            }
            _z_n_msg_clear(&n_msg);
        }
    }
    return ret;
}

z_result_t _z_undeclare_resource(_z_session_t *zn, uint16_t rid) {
    z_result_t ret = _Z_RES_OK;
    _Z_DEBUG("Undeclaring local keyexpr %d", rid);
    _z_resource_t *r = _z_get_resource_by_id(zn, _Z_KEYEXPR_MAPPING_LOCAL, rid);
    if (r != NULL) {
        // Build the declare message to send on the wire
        _z_declaration_t declaration = _z_make_undecl_keyexpr(rid);
        _z_network_message_t n_msg = _z_n_msg_make_declare(declaration, false, 0);
        if (_z_send_undeclare(zn, &n_msg) == _Z_RES_OK) {
            // Remove local resource
            _z_unregister_resource(zn, rid, _Z_KEYEXPR_MAPPING_LOCAL);
            // Invalidate cache
            _z_subscription_cache_invalidate(zn);
            _z_queryable_cache_invalidate(zn);
        } else {
            ret = _Z_ERR_TRANSPORT_TX_FAILED;
        }
        _z_n_msg_clear(&n_msg);
    } else {
        ret = _Z_ERR_KEYEXPR_UNKNOWN;
    }

    return ret;
}

// TODO(sashacmc): currently used only for liveliness, because it have little bit different behavior from others
// It seems also correct for z_declare_queryable and z_declare_publisher, but it need to be verified
_z_keyexpr_t _z_update_keyexpr_to_declared(_z_session_t *zs, _z_keyexpr_t keyexpr) {
    _z_keyexpr_t keyexpr_aliased = _z_keyexpr_alias_from_user_defined(keyexpr, true);
    _z_keyexpr_t key = keyexpr_aliased;

    // TODO: Implement interest protocol for multicast transports, unicast p2p
    if (zs->_mode == Z_WHATAMI_CLIENT) {
        _z_resource_t *r = _z_get_resource_by_key(zs, &keyexpr_aliased);
        if (r != NULL) {
            key = _z_rid_with_suffix(r->_id, NULL);
        } else {
            uint16_t id = _z_declare_resource(zs, &keyexpr_aliased);
            key = _z_rid_with_suffix(id, NULL);
        }
    }
    return key;
}

#if Z_FEATURE_PUBLICATION == 1
/*------------------  Publisher Declaration ------------------*/
_z_publisher_t _z_declare_publisher(const _z_session_rc_t *zn, _z_keyexpr_t keyexpr, _z_encoding_t *encoding,
                                    z_congestion_control_t congestion_control, z_priority_t priority, bool is_express,
                                    z_reliability_t reliability) {
    // Allocate publisher
    _z_publisher_t ret;
    // Fill publisher
    ret._key = _z_keyexpr_duplicate(&keyexpr);
    ret._id = _z_get_entity_id(_Z_RC_IN_VAL(zn));
    ret._congestion_control = congestion_control;
    ret._priority = priority;
    ret._is_express = is_express;
    ret.reliability = reliability;
    ret._zn = _z_session_rc_clone_as_weak(zn);
    ret._encoding = encoding == NULL ? _z_encoding_null() : _z_encoding_steal(encoding);
    return ret;
}

z_result_t _z_undeclare_publisher(_z_publisher_t *pub) {
    if (pub == NULL || _Z_RC_IS_NULL(&pub->_zn)) {
        return _Z_ERR_ENTITY_UNKNOWN;
    }
#if Z_FEATURE_MATCHING == 1
    _z_matching_listener_entity_undeclare(_Z_RC_IN_VAL(&pub->_zn), pub->_id);
#endif
    // Clear publisher
    // TODO: Rework write filters to work with non-aggregated interests
    if (_Z_RC_IN_VAL(&pub->_zn)->_mode == Z_WHATAMI_CLIENT) {
        _z_write_filter_destroy(_Z_RC_IN_VAL(&pub->_zn), &pub->_filter);
    }
    _z_undeclare_resource(_Z_RC_IN_VAL(&pub->_zn), pub->_key._id);
    return _Z_RES_OK;
}

/*------------------ Write ------------------*/
z_result_t _z_write(_z_session_t *zn, const _z_keyexpr_t keyexpr, const _z_bytes_t payload,
                    const _z_encoding_t *encoding, const z_sample_kind_t kind, const z_congestion_control_t cong_ctrl,
                    z_priority_t priority, bool is_express, const _z_timestamp_t *timestamp,
                    const _z_bytes_t attachment, z_reliability_t reliability, const _z_source_info_t *source_info) {
    z_result_t ret = _Z_RES_OK;
    _z_network_message_t msg;
    switch (kind) {
        case Z_SAMPLE_KIND_PUT:
            // TODO(refactor): use z_n_make_push
            msg = (_z_network_message_t){
                ._tag = _Z_N_PUSH,
                ._reliability = Z_RELIABILITY_DEFAULT,
                ._body._push =
                    {
                        ._key = keyexpr,
                        ._qos = _z_n_qos_make(is_express, cong_ctrl == Z_CONGESTION_CONTROL_BLOCK, priority),
                        ._timestamp = _z_timestamp_null(),
                        ._body._is_put = true,
                        ._body._body._put =
                            {
                                ._commons = {._timestamp = ((timestamp != NULL) ? *timestamp : _z_timestamp_null()),
                                             ._source_info =
                                                 ((source_info != NULL) ? *source_info : _z_source_info_null())},
                                ._payload = payload,
                                ._encoding = encoding == NULL ? _z_encoding_null() : *encoding,
                                ._attachment = attachment,
                            },
                    },
            };
            break;
        case Z_SAMPLE_KIND_DELETE:
            // TODO(refactor): use z_n_make_push
            msg = (_z_network_message_t){
                ._tag = _Z_N_PUSH,
                ._reliability = Z_RELIABILITY_DEFAULT,
                ._body._push =
                    {
                        ._key = keyexpr,
                        ._qos = _z_n_qos_make(is_express, cong_ctrl == Z_CONGESTION_CONTROL_BLOCK, priority),
                        ._timestamp = _z_timestamp_null(),
                        ._body._is_put = false,
                        ._body._body._del =
                            {._commons = {._timestamp = ((timestamp != NULL) ? *timestamp : _z_timestamp_null()),
                                          ._source_info =
                                              ((source_info != NULL) ? *source_info : _z_source_info_null())}},
                    },
            };
            break;
        default:
            return _Z_ERR_GENERIC;
    }

    if (_z_send_n_msg(zn, &msg, reliability, cong_ctrl) != _Z_RES_OK) {
        ret = _Z_ERR_TRANSPORT_TX_FAILED;
    }

    // Freeing z_msg is unnecessary, as all of its components are aliased

    return ret;
}
#endif

#if Z_FEATURE_SUBSCRIPTION == 1
/*------------------ Subscriber Declaration ------------------*/
_z_subscriber_t _z_declare_subscriber(const _z_session_rc_t *zn, _z_keyexpr_t keyexpr,
                                      _z_closure_sample_callback_t callback, _z_drop_handler_t dropper, void *arg) {
    _z_subscription_t s;
    s._id = _z_get_entity_id(_Z_RC_IN_VAL(zn));
    s._key_id = keyexpr._id;
    s._key = _z_get_expanded_key_from_key(_Z_RC_IN_VAL(zn), &keyexpr);
    s._callback = callback;
    s._dropper = dropper;
    s._arg = arg;

    _z_subscriber_t ret = _z_subscriber_null();
    // Register subscription, stored at session-level, do not drop it by the end of this function.
    _z_subscription_rc_t *sp_s = _z_register_subscription(_Z_RC_IN_VAL(zn), _Z_SUBSCRIBER_KIND_SUBSCRIBER, &s);
    if (sp_s == NULL) {
        _z_subscriber_clear(&ret);
        return ret;
    }
    // Build the declare message to send on the wire
    _z_declaration_t declaration = _z_make_decl_subscriber(&keyexpr, s._id);
    _z_network_message_t n_msg = _z_n_msg_make_declare(declaration, false, 0);
    if (_z_send_declare(_Z_RC_IN_VAL(zn), &n_msg) != _Z_RES_OK) {
        _z_unregister_subscription(_Z_RC_IN_VAL(zn), _Z_SUBSCRIBER_KIND_SUBSCRIBER, sp_s);
        _z_subscriber_clear(&ret);
        return ret;
    }
    _z_n_msg_clear(&n_msg);
    // Fill subscriber
    ret._entity_id = s._id;
    ret._zn = _z_session_rc_clone_as_weak(zn);
    // Invalidate cache
    _z_subscription_cache_invalidate(_Z_RC_IN_VAL(zn));
    return ret;
}

z_result_t _z_undeclare_subscriber(_z_subscriber_t *sub) {
    if (sub == NULL || _Z_RC_IS_NULL(&sub->_zn)) {
        return _Z_ERR_ENTITY_UNKNOWN;
    }
    // Find subscription entry
    _z_subscription_rc_t *s =
        _z_get_subscription_by_id(_Z_RC_IN_VAL(&sub->_zn), _Z_SUBSCRIBER_KIND_SUBSCRIBER, sub->_entity_id);
    if (s == NULL) {
        return _Z_ERR_ENTITY_UNKNOWN;
    }
    // Build the declare message to send on the wire
    _z_declaration_t declaration;
    if (_Z_RC_IN_VAL(&sub->_zn)->_mode == Z_WHATAMI_CLIENT) {
        declaration = _z_make_undecl_subscriber(sub->_entity_id, NULL);
    } else {
        declaration = _z_make_undecl_subscriber(sub->_entity_id, &_Z_RC_IN_VAL(s)->_key);
    }
    _z_network_message_t n_msg = _z_n_msg_make_declare(declaration, false, 0);
    if (_z_send_undeclare(_Z_RC_IN_VAL(&sub->_zn), &n_msg) != _Z_RES_OK) {
        return _Z_ERR_TRANSPORT_TX_FAILED;
    }
    _z_n_msg_clear(&n_msg);
    // Only if message is successfully send, local subscription state can be removed
    _z_undeclare_resource(_Z_RC_IN_VAL(&sub->_zn), _Z_RC_IN_VAL(s)->_key_id);
    _z_unregister_subscription(_Z_RC_IN_VAL(&sub->_zn), _Z_SUBSCRIBER_KIND_SUBSCRIBER, s);
    // Invalidate cache
    _z_subscription_cache_invalidate(_Z_RC_IN_VAL(&sub->_zn));
    return _Z_RES_OK;
}
#endif

#if Z_FEATURE_QUERYABLE == 1
/*------------------ Queryable Declaration ------------------*/
_z_queryable_t _z_declare_queryable(const _z_session_rc_t *zn, _z_keyexpr_t keyexpr, bool complete,
                                    _z_closure_query_callback_t callback, _z_drop_handler_t dropper, void *arg) {
    _z_session_queryable_t q;
    q._id = _z_get_entity_id(_Z_RC_IN_VAL(zn));
    q._key = _z_get_expanded_key_from_key(_Z_RC_IN_VAL(zn), &keyexpr);
    q._complete = complete;
    q._callback = callback;
    q._dropper = dropper;
    q._arg = arg;

    _z_queryable_t ret;
    // Create session_queryable entry, stored at session-level, do not drop it by the end of this function.
    _z_session_queryable_rc_t *sp_q = _z_register_session_queryable(_Z_RC_IN_VAL(zn), &q);
    if (sp_q == NULL) {
        _z_queryable_clear(&ret);
        return ret;
    }
    // Build the declare message to send on the wire
    _z_declaration_t declaration = _z_make_decl_queryable(&keyexpr, q._id, q._complete, _Z_QUERYABLE_DISTANCE_DEFAULT);
    _z_network_message_t n_msg = _z_n_msg_make_declare(declaration, false, 0);
    if (_z_send_declare(_Z_RC_IN_VAL(zn), &n_msg) != _Z_RES_OK) {
        _z_unregister_session_queryable(_Z_RC_IN_VAL(zn), sp_q);
        _z_queryable_clear(&ret);
        return ret;
    }
    _z_n_msg_clear(&n_msg);
    // Fill queryable
    ret._entity_id = q._id;
    ret._zn = _z_session_rc_clone_as_weak(zn);
    // Invalidate cache
    _z_queryable_cache_invalidate(_Z_RC_IN_VAL(zn));
    return ret;
}

z_result_t _z_undeclare_queryable(_z_queryable_t *qle) {
    if (qle == NULL || _Z_RC_IS_NULL(&qle->_zn)) {
        return _Z_ERR_ENTITY_UNKNOWN;
    }
    // Find session_queryable entry
    _z_session_queryable_rc_t *q = _z_get_session_queryable_by_id(_Z_RC_IN_VAL(&qle->_zn), qle->_entity_id);
    if (q == NULL) {
        return _Z_ERR_ENTITY_UNKNOWN;
    }
    // Build the declare message to send on the wire
    _z_declaration_t declaration;
    if (_Z_RC_IN_VAL(&qle->_zn)->_mode == Z_WHATAMI_CLIENT) {
        declaration = _z_make_undecl_queryable(qle->_entity_id, NULL);
    } else {
        declaration = _z_make_undecl_queryable(qle->_entity_id, &_Z_RC_IN_VAL(q)->_key);
    }
    _z_network_message_t n_msg = _z_n_msg_make_declare(declaration, false, 0);
    if (_z_send_undeclare(_Z_RC_IN_VAL(&qle->_zn), &n_msg) != _Z_RES_OK) {
        return _Z_ERR_TRANSPORT_TX_FAILED;
    }
    _z_n_msg_clear(&n_msg);
    // Only if message is successfully send, local queryable state can be removed
    _z_unregister_session_queryable(_Z_RC_IN_VAL(&qle->_zn), q);
    // Invalidate cache
    _z_queryable_cache_invalidate(_Z_RC_IN_VAL(&qle->_zn));
    return _Z_RES_OK;
}

z_result_t _z_send_reply(const _z_query_t *query, const _z_session_rc_t *zsrc, const _z_keyexpr_t *keyexpr,
                         const _z_value_t payload, const z_sample_kind_t kind, const z_congestion_control_t cong_ctrl,
                         z_priority_t priority, bool is_express, const _z_timestamp_t *timestamp,
                         const _z_bytes_t att) {
    z_result_t ret = _Z_RES_OK;
    _z_session_t *zn = _Z_RC_IN_VAL(zsrc);

    _z_keyexpr_t q_ke;
    _z_keyexpr_t r_ke;
    if (query->_anyke == false) {
        q_ke = _z_get_expanded_key_from_key(zn, &query->_key);
        r_ke = _z_get_expanded_key_from_key(zn, keyexpr);
        if (_z_keyexpr_suffix_intersects(&q_ke, &r_ke) == false) {
            ret = _Z_ERR_KEYEXPR_NOT_MATCH;
        }
        _z_keyexpr_clear(&q_ke);
        _z_keyexpr_clear(&r_ke);
    }

    if (ret == _Z_RES_OK) {
        // Build the reply context decorator. This is NOT the final reply.
        _z_id_t zid = zn->_local_zid;
        _z_keyexpr_t ke = _z_keyexpr_alias(keyexpr);
        _z_zenoh_message_t z_msg;
        switch (kind) {
            case Z_SAMPLE_KIND_PUT:
                // TODO(refactor): use z_n_make_reply
                z_msg = (_z_zenoh_message_t){
                    ._tag = _Z_N_RESPONSE,
                    ._reliability = Z_RELIABILITY_DEFAULT,
                    ._body._response =
                        {
                            ._request_id = query->_request_id,
                            ._key = ke,
                            ._ext_responder = {._zid = zid, ._eid = 0},
                            ._ext_qos = _z_n_qos_make(is_express, cong_ctrl == Z_CONGESTION_CONTROL_BLOCK, priority),
                            ._ext_timestamp = _z_timestamp_null(),
                            ._tag = _Z_RESPONSE_BODY_REPLY,
                            ._body._reply =
                                {
                                    ._consolidation = Z_CONSOLIDATION_MODE_DEFAULT,
                                    ._body._is_put = true,
                                    ._body._body._put =
                                        {
                                            ._payload = payload.payload,
                                            ._encoding = payload.encoding,
                                            ._commons =
                                                {
                                                    ._timestamp =
                                                        (timestamp != NULL) ? *timestamp : _z_timestamp_null(),
                                                    ._source_info = _z_source_info_null(),
                                                },
                                            ._attachment = att,
                                        },
                                },
                        },
                };
                break;
            case Z_SAMPLE_KIND_DELETE:
                // TODO(refactor): use z_n_make_reply
                z_msg = (_z_zenoh_message_t){
                    ._tag = _Z_N_RESPONSE,
                    ._reliability = Z_RELIABILITY_DEFAULT,
                    ._body._response =
                        {
                            ._request_id = query->_request_id,
                            ._key = ke,
                            ._ext_responder = {._zid = zid, ._eid = 0},
                            ._ext_qos = _z_n_qos_make(is_express, cong_ctrl == Z_CONGESTION_CONTROL_BLOCK, priority),
                            ._ext_timestamp = _z_timestamp_null(),
                            ._tag = _Z_RESPONSE_BODY_REPLY,
                            ._body._reply =
                                {
                                    ._consolidation = Z_CONSOLIDATION_MODE_DEFAULT,
                                    ._body._is_put = false,
                                    ._body._body._del =
                                        {
                                            ._commons =
                                                {
                                                    ._timestamp =
                                                        (timestamp != NULL) ? *timestamp : _z_timestamp_null(),
                                                    ._source_info = _z_source_info_null(),
                                                },
                                            ._attachment = att,
                                        },
                                },
                        },
                };
                break;
            default:
                return _Z_ERR_GENERIC;
        }
        if (_z_send_n_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
            ret = _Z_ERR_TRANSPORT_TX_FAILED;
        }

        // Freeing z_msg is unnecessary, as all of its components are aliased
    }

    return ret;
}

z_result_t _z_send_reply_err(const _z_query_t *query, const _z_session_rc_t *zsrc, const _z_value_t payload) {
    z_result_t ret = _Z_RES_OK;
    _z_session_t *zn = _Z_RC_IN_VAL(zsrc);

    // Build the reply context decorator. This is NOT the final reply.
    _z_id_t zid = zn->_local_zid;
    _z_zenoh_message_t msg = {
        // TODO(refactor): use z_n_make_reply
        ._tag = _Z_N_RESPONSE,
        ._reliability = Z_RELIABILITY_DEFAULT,
        ._body._response =
            {
                ._request_id = query->_request_id,
                ._ext_responder = {._zid = zid, ._eid = 0},
                ._ext_qos = _z_n_qos_make(false, true, Z_PRIORITY_DEFAULT),
                ._ext_timestamp = _z_timestamp_null(),
                ._tag = _Z_RESPONSE_BODY_ERR,
                ._body._err =
                    {
                        ._payload = payload.payload,
                        ._encoding = payload.encoding,
                        ._ext_source_info = _z_source_info_null(),
                    },
            },
    };
    if (_z_send_n_msg(zn, &msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
        ret = _Z_ERR_TRANSPORT_TX_FAILED;
    }

    return ret;
}
#endif

#if Z_FEATURE_QUERY == 1
/*------------------  Querier Declaration ------------------*/
_z_querier_t _z_declare_querier(const _z_session_rc_t *zn, _z_keyexpr_t keyexpr,
                                z_consolidation_mode_t consolidation_mode, z_congestion_control_t congestion_control,
                                z_query_target_t target, z_priority_t priority, bool is_express, uint64_t timeout_ms,
                                _z_encoding_t *encoding, z_reliability_t reliability) {
    // Allocate querier
    _z_querier_t ret;
    // Fill querier
    ret._encoding = encoding == NULL ? _z_encoding_null() : _z_encoding_steal(encoding);
    ret.reliability = reliability;
    ret._key = _z_keyexpr_duplicate(&keyexpr);
    ret._id = _z_get_entity_id(_Z_RC_IN_VAL(zn));
    ret._consolidation_mode = consolidation_mode;
    ret._congestion_control = congestion_control;
    ret._target = target;
    ret._priority = priority;
    ret._is_express = is_express;
    ret._timeout_ms = timeout_ms;
    ret._zn = _z_session_rc_clone_as_weak(zn);
    return ret;
}

z_result_t _z_undeclare_querier(_z_querier_t *querier) {
    if (querier == NULL || _Z_RC_IS_NULL(&querier->_zn)) {
        return _Z_ERR_ENTITY_UNKNOWN;
    }
#if Z_FEATURE_MATCHING == 1
    _z_matching_listener_entity_undeclare(_Z_RC_IN_VAL(&querier->_zn), querier->_id);
#endif
    // TODO: Rework write filters to work with non-aggregated interests
    if (_Z_RC_IN_VAL(&querier->_zn)->_mode == Z_WHATAMI_CLIENT) {
        _z_write_filter_destroy(_Z_RC_IN_VAL(&querier->_zn), &querier->_filter);
    }
    _z_undeclare_resource(_Z_RC_IN_VAL(&querier->_zn), querier->_key._id);
    return _Z_RES_OK;
}

/*------------------ Query ------------------*/
z_result_t _z_query(_z_session_t *zn, _z_keyexpr_t keyexpr, const char *parameters, const z_query_target_t target,
                    const z_consolidation_mode_t consolidation, _z_value_t value, _z_closure_reply_callback_t callback,
                    _z_drop_handler_t dropper, void *arg, uint64_t timeout_ms, const _z_bytes_t attachment,
                    z_congestion_control_t cong_ctrl, z_priority_t priority, bool is_express) {
    z_result_t ret = _Z_RES_OK;

    // Create the pending query object
    _z_pending_query_t *pq = (_z_pending_query_t *)z_malloc(sizeof(_z_pending_query_t));
    if (pq != NULL) {
        pq->_id = _z_get_query_id(zn);
        pq->_key = _z_get_expanded_key_from_key(zn, &keyexpr);
        pq->_target = target;
        pq->_consolidation = consolidation;
        pq->_anykey = (parameters == NULL || strstr(parameters, Z_SELECTOR_QUERY_MATCH) == NULL) ? false : true;
        pq->_callback = callback;
        pq->_dropper = dropper;
        pq->_pending_replies = NULL;
        pq->_arg = arg;
        pq->_timeout = timeout_ms;
        pq->_start_time = z_clock_now();

        ret = _z_register_pending_query(zn, pq);  // Add the pending query to the current session
        if (ret == _Z_RES_OK) {
            _z_slice_t params =
                (parameters == NULL) ? _z_slice_null() : _z_slice_alias_buf((uint8_t *)parameters, strlen(parameters));
            _z_zenoh_message_t z_msg = _z_msg_make_query(&keyexpr, &params, pq->_id, pq->_consolidation, &value,
                                                         timeout_ms, attachment, cong_ctrl, priority, is_express);

            if (_z_send_n_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, cong_ctrl) != _Z_RES_OK) {
                _z_unregister_pending_query(zn, pq);
                ret = _Z_ERR_TRANSPORT_TX_FAILED;
            }
        } else {
            _z_pending_query_clear(pq);
        }
    }

    return ret;
}
#endif

#if Z_FEATURE_INTEREST == 1
/*------------------ Interest Declaration ------------------*/
uint32_t _z_add_interest(_z_session_t *zn, _z_keyexpr_t keyexpr, _z_interest_handler_t callback, uint8_t flags,
                         void *arg) {
    _z_session_interest_t intr;
    intr._id = _z_get_entity_id(zn);
    intr._key = _z_get_expanded_key_from_key(zn, &keyexpr);
    intr._flags = flags;
    intr._callback = callback;
    intr._arg = arg;

    // Create interest entry, stored at session-level, do not drop it by the end of this function.
    _z_session_interest_rc_t *sintr = _z_register_interest(zn, &intr);
    if (sintr == NULL) {
        return 0;
    }
    // Build the interest message to send on the wire (only needed in client mode)
    if (zn->_mode == Z_WHATAMI_CLIENT) {
        _z_interest_t interest = _z_make_interest(&keyexpr, intr._id, intr._flags);
        _z_network_message_t n_msg = _z_n_msg_make_interest(interest);
        if (_z_send_n_msg(zn, &n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
            _z_unregister_interest(zn, sintr);
            return 0;
        }
        _z_n_msg_clear(&n_msg);
    }
    return intr._id;
}

z_result_t _z_remove_interest(_z_session_t *zn, uint32_t interest_id) {
    // Find interest entry
    _z_session_interest_rc_t *sintr = _z_get_interest_by_id(zn, interest_id);
    if (sintr == NULL) {
        return _Z_ERR_ENTITY_UNKNOWN;
    }
    // Build the declare message to send on the wire (only needed in client mode)
    if (zn->_mode == Z_WHATAMI_CLIENT) {
        _z_interest_t interest = _z_make_interest_final(_Z_RC_IN_VAL(sintr)->_id);
        _z_network_message_t n_msg = _z_n_msg_make_interest(interest);
        if (_z_send_n_msg(zn, &n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
            return _Z_ERR_TRANSPORT_TX_FAILED;
        }
        _z_n_msg_clear(&n_msg);
    }
    // Only if message is successfully send, session interest can be removed
    _z_unregister_interest(zn, sintr);
    return _Z_RES_OK;
}
#endif
