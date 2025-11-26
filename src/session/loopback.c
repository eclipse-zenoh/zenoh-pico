//
// Copyright (c) 2025 ZettaScale Technology
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

#include "zenoh-pico/session/loopback.h"

#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/interest.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/reply.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/locality.h"

#if defined(Z_LOOPBACK_TESTING)
static _z_session_transport_override_fn _z_transport_common_override = NULL;

void _z_session_set_transport_common_override(_z_session_transport_override_fn fn) {
    _z_transport_common_override = fn;
}
#endif

#if Z_FEATURE_SUBSCRIPTION == 1 || Z_FEATURE_QUERYABLE == 1
static _z_transport_common_t *_z_session_get_transport_common(_z_session_t *zn) {
#if defined(Z_LOOPBACK_TESTING)
    if (_z_transport_common_override != NULL) {
        _z_transport_common_t *override = _z_transport_common_override(zn);
        if (override != NULL) {
            return override;
        }
    }
#endif
    switch (zn->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            return &zn->_tp._transport._unicast._common;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            return &zn->_tp._transport._multicast._common;
        case _Z_TRANSPORT_RAWETH_TYPE:
            return &zn->_tp._transport._raweth._common;
        default:
            break;
    }
    return NULL;
}

#else
static _z_transport_common_t *_z_session_get_transport_common(_z_session_t *zn) {
    _ZP_UNUSED(zn);
    return NULL;
}
#endif  // Z_FEATURE_SUBSCRIPTION == 1 || Z_FEATURE_QUERYABLE == 1

#if Z_FEATURE_SUBSCRIPTION == 1 && Z_FEATURE_LOCAL_SUBSCRIBER == 1
static bool _z_session_has_local_subscriber(_z_session_t *zn, const _z_keyexpr_t *keyexpr) {
    bool found = false;

    _z_session_mutex_lock(zn);
    _z_keyexpr_t expanded = __unsafe_z_get_expanded_key_from_key(zn, keyexpr, true, NULL);
    if (_z_keyexpr_has_suffix(&expanded)) {
        _z_subscription_rc_slist_t *xs = zn->_subscriptions;
        while (xs != NULL) {
            _z_subscription_rc_t *sub = _z_subscription_rc_slist_value(xs);
            const _z_subscription_t *sub_val = _Z_RC_IN_VAL(sub);
            if (_z_locality_allows_local(sub_val->_allowed_origin) &&
                _z_keyexpr_suffix_intersects(&sub_val->_key, &expanded)) {
                found = true;
                break;
            }
            xs = _z_subscription_rc_slist_next(xs);
        }
    }
    _z_keyexpr_clear(&expanded);
    _z_session_mutex_unlock(zn);
    return found;
}

bool _z_session_deliver_push_locally(_z_session_t *zn, const _z_keyexpr_t *keyexpr, const _z_bytes_t *payload,
                                     const _z_encoding_t *encoding, z_sample_kind_t kind, _z_n_qos_t qos,
                                     const _z_timestamp_t *timestamp, const _z_bytes_t *attachment,
                                     z_reliability_t reliability, const _z_source_info_t *source_info,
                                     z_locality_t allowed_destination) {
    if (!_z_locality_allows_local(allowed_destination)) {
        return false;
    }
    if (!_z_session_has_local_subscriber(zn, keyexpr)) {
        return false;
    }

    _z_transport_common_t *transport = _z_session_get_transport_common(zn);
    if (transport == NULL) {
        return false;
    }

    switch (kind) {
        case Z_SAMPLE_KIND_PUT: {
            _z_bytes_t payload_copy = _z_bytes_null();
            _z_bytes_t attachment_copy = _z_bytes_null();
            _z_encoding_t encoding_copy = _z_encoding_null();
            z_result_t ret = _Z_RES_OK;

            if (payload != NULL) {
                _Z_SET_IF_OK(ret, _z_bytes_copy(&payload_copy, payload));
            }
            if (attachment != NULL) {
                _Z_SET_IF_OK(ret, _z_bytes_copy(&attachment_copy, attachment));
            }
            if (encoding != NULL) {
                _Z_SET_IF_OK(ret, _z_encoding_copy(&encoding_copy, encoding));
            }

            if (ret != _Z_RES_OK) {
                _z_bytes_drop(&payload_copy);
                _z_bytes_drop(&attachment_copy);
                _z_encoding_clear(&encoding_copy);
                return false;
            }

            _z_network_message_t msg;
            _z_n_msg_make_push_put(&msg, keyexpr, (payload == NULL) ? NULL : &payload_copy,
                                   (encoding == NULL) ? NULL : &encoding_copy, qos, timestamp,
                                   (attachment == NULL) ? NULL : &attachment_copy, reliability, source_info);
            ret = _z_handle_network_message(transport, &msg, NULL);
            _z_n_msg_clear(&msg);
            return ret == _Z_RES_OK;
        }
        case Z_SAMPLE_KIND_DELETE: {
            _z_network_message_t msg;
            _z_n_msg_make_push_del(&msg, keyexpr, qos, timestamp, reliability, source_info);
            z_result_t ret = _z_handle_network_message(transport, &msg, NULL);
            _z_n_msg_clear(&msg);
            return ret == _Z_RES_OK;
        }
        default:
            return false;
    }
}
#else
bool _z_session_deliver_push_locally(_z_session_t *zn, const _z_keyexpr_t *keyexpr, const _z_bytes_t *payload,
                                     const _z_encoding_t *encoding, z_sample_kind_t kind, _z_n_qos_t qos,
                                     const _z_timestamp_t *timestamp, const _z_bytes_t *attachment,
                                     z_reliability_t reliability, const _z_source_info_t *source_info,
                                     z_locality_t allowed_destination) {
    _ZP_UNUSED(allowed_destination);
    _ZP_UNUSED(zn);
    _ZP_UNUSED(keyexpr);
    _ZP_UNUSED(payload);
    _ZP_UNUSED(encoding);
    _ZP_UNUSED(kind);
    _ZP_UNUSED(qos);
    _ZP_UNUSED(timestamp);
    _ZP_UNUSED(attachment);
    _ZP_UNUSED(reliability);
    _ZP_UNUSED(source_info);
    return false;
}
#endif  // Z_FEATURE_SUBSCRIPTION == 1

#if Z_FEATURE_QUERYABLE == 1 && Z_FEATURE_LOCAL_QUERYABLE == 1
static bool _z_session_has_local_queryable(_z_session_t *zn, const _z_keyexpr_t *keyexpr) {
    bool found = false;

    _z_session_mutex_lock(zn);
    _z_keyexpr_t expanded = __unsafe_z_get_expanded_key_from_key(zn, keyexpr, true, NULL);
    if (_z_keyexpr_has_suffix(&expanded)) {
        _z_session_queryable_rc_slist_t *xs = zn->_local_queryable;
        while (xs != NULL) {
            _z_session_queryable_rc_t *qle = _z_session_queryable_rc_slist_value(xs);
            const _z_session_queryable_t *qle_val = _Z_RC_IN_VAL(qle);
            if (_z_locality_allows_local(qle_val->_allowed_origin) &&
                _z_keyexpr_suffix_intersects(&qle_val->_key, &expanded)) {
                found = true;
                break;
            }
            xs = _z_session_queryable_rc_slist_next(xs);
        }
    }
    _z_keyexpr_clear(&expanded);
    _z_session_mutex_unlock(zn);
    return found;
}

bool _z_session_deliver_query_locally(_z_session_t *zn, const _z_keyexpr_t *keyexpr, const _z_slice_t *parameters,
                                      z_consolidation_mode_t consolidation, const _z_bytes_t *payload,
                                      const _z_encoding_t *encoding, const _z_bytes_t *attachment,
                                      const _z_source_info_t *source_info, _z_zint_t qid, uint64_t timeout_ms,
                                      _z_n_qos_t qos, z_locality_t allowed_destination) {
    _z_transport_common_t *transport = _z_session_get_transport_common(zn);
    if (transport == NULL) {
        return false;
    }

    _z_slice_t params_copy = _z_slice_null();
    _z_bytes_t payload_copy = _z_bytes_null();
    _z_bytes_t attachment_copy = _z_bytes_null();
    _z_encoding_t encoding_copy = _z_encoding_null();
    z_result_t ret = _Z_RES_OK;

    if (parameters != NULL) {
        _Z_SET_IF_OK(ret, _z_slice_copy(&params_copy, parameters));
    }
    if (payload != NULL) {
        _Z_SET_IF_OK(ret, _z_bytes_copy(&payload_copy, payload));
    }
    if (attachment != NULL) {
        _Z_SET_IF_OK(ret, _z_bytes_copy(&attachment_copy, attachment));
    }
    if (encoding != NULL) {
        _Z_SET_IF_OK(ret, _z_encoding_copy(&encoding_copy, encoding));
    }

    if (ret != _Z_RES_OK) {
        _z_slice_clear(&params_copy);
        _z_bytes_drop(&payload_copy);
        _z_bytes_drop(&attachment_copy);
        _z_encoding_clear(&encoding_copy);
        return false;
    }

    _z_zenoh_message_t msg;
    _z_n_msg_make_query(&msg, keyexpr, &params_copy, qid, Z_RELIABILITY_DEFAULT, consolidation,
                        (payload == NULL) ? NULL : &payload_copy, (encoding == NULL) ? NULL : &encoding_copy,
                        timeout_ms, (attachment == NULL) ? NULL : &attachment_copy, qos, source_info);
    ret = _z_handle_network_message(transport, &msg, NULL);
    _z_n_msg_clear(&msg);
    return ret == _Z_RES_OK;
}
#else
bool _z_session_deliver_query_locally(_z_session_t *zn, const _z_keyexpr_t *keyexpr, const _z_slice_t *parameters,
                                      z_consolidation_mode_t consolidation, const _z_bytes_t *payload,
                                      const _z_encoding_t *encoding, const _z_bytes_t *attachment,
                                      const _z_source_info_t *source_info, _z_zint_t qid, uint64_t timeout_ms,
                                      _z_n_qos_t qos, z_locality_t allowed_destination) {
    _ZP_UNUSED(allowed_destination);
    _ZP_UNUSED(zn);
    _ZP_UNUSED(keyexpr);
    _ZP_UNUSED(parameters);
    _ZP_UNUSED(consolidation);
    _ZP_UNUSED(payload);
    _ZP_UNUSED(encoding);
    _ZP_UNUSED(attachment);
    _ZP_UNUSED(source_info);
    _ZP_UNUSED(qid);
    _ZP_UNUSED(timeout_ms);
    _ZP_UNUSED(qos);
    return false;
}
#endif  // Z_FEATURE_QUERYABLE == 1

#if Z_FEATURE_QUERY == 1
bool _z_session_deliver_reply_locally(const _z_query_t *query, const _z_session_rc_t *responder,
                                      const _z_keyexpr_t *keyexpr, const _z_bytes_t *payload,
                                      const _z_encoding_t *encoding, z_sample_kind_t kind, _z_n_qos_t qos,
                                      const _z_timestamp_t *timestamp, const _z_bytes_t *attachment,
                                      const _z_source_info_t *source_info) {
    if (query == NULL || responder == NULL) {
        return false;
    }

    _z_session_rc_t dst_rc = _z_session_weak_upgrade_if_open(&query->_zn);
    if (_Z_RC_IS_NULL(&dst_rc)) {
        return false;
    }

    _z_session_t *dst = _Z_RC_IN_VAL(&dst_rc);
    _z_session_t *src = _Z_RC_IN_VAL(responder);
    if (dst != src) {
        goto err_drop_rc;
    }

    if (_z_get_pending_query_by_id(dst, query->_request_id) == NULL) {
        goto err_drop_rc;
    }

    _z_transport_common_t *transport = _z_session_get_transport_common(dst);
    if (transport == NULL) {
        goto err_drop_rc;
    }

    _z_bytes_t payload_copy = _z_bytes_null();
    _z_encoding_t encoding_copy = _z_encoding_null();
    _z_bytes_t attachment_copy = _z_bytes_null();
    z_result_t ret = _Z_RES_OK;

    if (kind == Z_SAMPLE_KIND_PUT && payload != NULL) {
        _Z_SET_IF_OK(ret, _z_bytes_copy(&payload_copy, payload));
    }
    if (kind == Z_SAMPLE_KIND_PUT && encoding != NULL) {
        _Z_SET_IF_OK(ret, _z_encoding_copy(&encoding_copy, encoding));
    }
    if (attachment != NULL) {
        _Z_SET_IF_OK(ret, _z_bytes_copy(&attachment_copy, attachment));
    }

    if (ret != _Z_RES_OK) {
        goto err_cleanup;
    }

    _z_network_message_t msg;
    switch (kind) {
        case Z_SAMPLE_KIND_PUT:
            _z_n_msg_make_reply_ok_put(
                &msg, &src->_local_zid, query->_request_id, keyexpr, Z_RELIABILITY_DEFAULT,
                Z_CONSOLIDATION_MODE_DEFAULT, qos, timestamp, source_info, (payload == NULL) ? NULL : &payload_copy,
                (encoding == NULL) ? NULL : &encoding_copy, (attachment == NULL) ? NULL : &attachment_copy);
            break;
        case Z_SAMPLE_KIND_DELETE:
            _z_n_msg_make_reply_ok_del(&msg, &src->_local_zid, query->_request_id, keyexpr, Z_RELIABILITY_DEFAULT,
                                       Z_CONSOLIDATION_MODE_DEFAULT, qos, timestamp, source_info,
                                       (attachment == NULL) ? NULL : &attachment_copy);
            break;
        default:
            goto err_cleanup;
    }

    ret = _z_handle_network_message(transport, &msg, NULL);
    _z_n_msg_clear(&msg);
    _z_session_rc_drop(&dst_rc);
    return ret == _Z_RES_OK;

err_cleanup:
    _z_bytes_drop(&payload_copy);
    _z_encoding_clear(&encoding_copy);
    _z_bytes_drop(&attachment_copy);
err_drop_rc:
    _z_session_rc_drop(&dst_rc);
    return false;
}

bool _z_session_deliver_reply_err_locally(const _z_query_t *query, const _z_session_rc_t *responder,
                                          const _z_bytes_t *payload, const _z_encoding_t *encoding, _z_n_qos_t qos) {
    if (query == NULL || responder == NULL) {
        return false;
    }

    _z_session_rc_t dst_rc = _z_session_weak_upgrade_if_open(&query->_zn);
    if (_Z_RC_IS_NULL(&dst_rc)) {
        return false;
    }

    _z_session_t *dst = _Z_RC_IN_VAL(&dst_rc);
    _z_session_t *src = _Z_RC_IN_VAL(responder);
    if (dst != src) {
        _z_session_rc_drop(&dst_rc);
        return false;
    }

    if (_z_get_pending_query_by_id(dst, query->_request_id) == NULL) {
        _z_session_rc_drop(&dst_rc);
        return false;
    }

    _z_transport_common_t *transport = _z_session_get_transport_common(dst);
    if (transport == NULL) {
        _z_session_rc_drop(&dst_rc);
        return false;
    }

    _z_bytes_t payload_copy = _z_bytes_null();
    _z_encoding_t encoding_copy = _z_encoding_null();
    z_result_t ret = _Z_RES_OK;
    if (payload != NULL) {
        _Z_SET_IF_OK(ret, _z_bytes_copy(&payload_copy, payload));
    }
    if (encoding != NULL) {
        _Z_SET_IF_OK(ret, _z_encoding_copy(&encoding_copy, encoding));
    }
    if (ret != _Z_RES_OK) {
        _z_bytes_drop(&payload_copy);
        _z_encoding_clear(&encoding_copy);
        _z_session_rc_drop(&dst_rc);
        return false;
    }

    _z_network_message_t msg;
    _z_n_msg_make_reply_err(&msg, &src->_local_zid, query->_request_id, Z_RELIABILITY_DEFAULT, qos,
                            (payload == NULL) ? NULL : &payload_copy, (encoding == NULL) ? NULL : &encoding_copy, NULL);
    ret = _z_handle_network_message(transport, &msg, NULL);
    _z_n_msg_clear(&msg);
    _z_session_rc_drop(&dst_rc);
    return ret == _Z_RES_OK;
}

bool _z_session_deliver_reply_final_locally(_z_session_t *zn, _z_zint_t rid, bool allow_remote) {
    if (zn == NULL) {
        return false;
    }
    _z_pending_query_t *pending = _z_get_pending_query_by_id(zn, rid);
    if (pending == NULL) {
        return false;
    }

    if (allow_remote) {
        return _z_trigger_query_reply_final(zn, rid) == _Z_RES_OK;
    }

    _z_transport_common_t *transport = _z_session_get_transport_common(zn);
    if (transport == NULL) {
        return _z_trigger_query_reply_final(zn, rid) == _Z_RES_OK;
    }
    _z_network_message_t msg;
    _z_n_msg_make_response_final(&msg, rid);
    z_result_t ret = _z_handle_network_message(transport, &msg, NULL);
    _z_n_msg_clear(&msg);
    if (ret != _Z_RES_OK) {
        return false;
    }
    return true;
}
#else
bool _z_session_deliver_reply_locally(const _z_query_t *query, const _z_session_rc_t *responder,
                                      const _z_keyexpr_t *keyexpr, const _z_bytes_t *payload,
                                      const _z_encoding_t *encoding, z_sample_kind_t kind, _z_n_qos_t qos,
                                      const _z_timestamp_t *timestamp, const _z_bytes_t *attachment,
                                      const _z_source_info_t *source_info) {
    _ZP_UNUSED(query);
    _ZP_UNUSED(responder);
    _ZP_UNUSED(keyexpr);
    _ZP_UNUSED(payload);
    _ZP_UNUSED(encoding);
    _ZP_UNUSED(kind);
    _ZP_UNUSED(qos);
    _ZP_UNUSED(timestamp);
    _ZP_UNUSED(attachment);
    _ZP_UNUSED(source_info);
    return false;
}

bool _z_session_deliver_reply_err_locally(const _z_query_t *query, const _z_session_rc_t *responder,
                                          const _z_bytes_t *payload, const _z_encoding_t *encoding, _z_n_qos_t qos) {
    _ZP_UNUSED(query);
    _ZP_UNUSED(responder);
    _ZP_UNUSED(payload);
    _ZP_UNUSED(encoding);
    _ZP_UNUSED(qos);
    return false;
}

bool _z_session_deliver_reply_final_locally(_z_session_t *zn, _z_zint_t rid, bool allow_remote) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(rid);
    _ZP_UNUSED(allow_remote);
    return false;
}
#endif  // Z_FEATURE_QUERY == 1
