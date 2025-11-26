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
z_result_t _z_session_deliver_push_locally(_z_session_t *zn, const _z_keyexpr_t *keyexpr, _z_bytes_t *payload,
                                           _z_encoding_t *encoding, z_sample_kind_t kind, _z_n_qos_t qos,
                                           const _z_timestamp_t *timestamp, _z_bytes_t *attachment,
                                           z_reliability_t reliability, const _z_source_info_t *source_info) {
    _z_transport_common_t *transport = _z_session_get_transport_common(zn);
    if (transport == NULL) {
        return _Z_ERR_INVALID;
    }

    _z_keyexpr_t keyexpr_copy;
    _Z_RETURN_IF_ERR(_z_keyexpr_copy(&keyexpr_copy, keyexpr));

    _z_network_message_t msg;
    switch (kind) {
        case Z_SAMPLE_KIND_PUT: {
            _z_n_msg_make_push_put(&msg, &keyexpr_copy, payload, encoding, qos, timestamp, attachment, reliability,
                                   source_info);
            break;
        }
        case Z_SAMPLE_KIND_DELETE: {
            _z_n_msg_make_push_del(&msg, &keyexpr_copy, qos, timestamp, reliability, source_info);
            break;
        }
        default:
            _z_keyexpr_clear(&keyexpr_copy);
            return _Z_ERR_INVALID;
    }

    z_result_t ret = _z_handle_network_message(transport, &msg, NULL);

    if (payload != NULL) {
        *payload = _z_bytes_null();
    }
    if (attachment != NULL) {
        *attachment = _z_bytes_null();
    }
    if (encoding != NULL) {
        *encoding = _z_encoding_null();
    }
    return ret;
}
#else
z_result_t _z_session_deliver_push_locally(_z_session_t *zn, const _z_keyexpr_t *keyexpr, _z_bytes_t *payload,
                                           _z_encoding_t *encoding, z_sample_kind_t kind, _z_n_qos_t qos,
                                           const _z_timestamp_t *timestamp, _z_bytes_t *attachment,
                                           z_reliability_t reliability, const _z_source_info_t *source_info) {
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
    return _Z_RES_OK;
}
#endif  // Z_FEATURE_SUBSCRIPTION == 1

#if Z_FEATURE_QUERYABLE == 1 && Z_FEATURE_LOCAL_QUERYABLE == 1
z_result_t _z_session_deliver_query_locally(_z_session_t *zn, const _z_keyexpr_t *keyexpr, const _z_slice_t *parameters,
                                            z_consolidation_mode_t consolidation, _z_bytes_t *payload,
                                            _z_encoding_t *encoding, _z_bytes_t *attachment,
                                            const _z_source_info_t *source_info, _z_zint_t qid, uint64_t timeout_ms,
                                            _z_n_qos_t qos, z_locality_t allowed_destination) {
    _z_transport_common_t *transport = _z_session_get_transport_common(zn);
    if (transport == NULL) {
        return _Z_ERR_INVALID;
    }

    _z_slice_t params_copy = _z_slice_null();
    _z_keyexpr_t keyexpr_copy = _z_keyexpr_null();
    if (parameters != NULL) {
        _Z_RETURN_IF_ERR(_z_slice_copy(&params_copy, parameters));
    }
    _Z_CLEAN_RETURN_IF_ERR(_z_keyexpr_copy(&keyexpr_copy, keyexpr), _z_slice_clear(&params_copy));

    _z_zenoh_message_t msg;
    _z_n_msg_make_query(&msg, &keyexpr_copy, &params_copy, qid, Z_RELIABILITY_DEFAULT, consolidation, payload, encoding,
                        timeout_ms, attachment, qos, source_info);

    z_result_t ret = transport == NULL ? _Z_ERR_SESSION_CLOSED : _z_handle_network_message(transport, &msg, NULL);

    if (payload != NULL) {
        *payload = _z_bytes_null();
    }
    if (attachment != NULL) {
        *attachment = _z_bytes_null();
    }
    if (encoding != NULL) {
        *encoding = _z_encoding_null();
    }
    return ret;
}
#else
z_result_t _z_session_deliver_query_locally(_z_session_t *zn, const _z_keyexpr_t *keyexpr, const _z_slice_t *parameters,
                                            z_consolidation_mode_t consolidation, _z_bytes_t *payload,
                                            _z_encoding_t *encoding, _z_bytes_t *attachment,
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
    return _Z_RES_OK;
}
#endif  // Z_FEATURE_QUERYABLE == 1

#if Z_FEATURE_QUERY == 1
z_result_t _z_session_deliver_reply_locally(const _z_query_t *query, const _z_session_rc_t *zn,
                                            const _z_keyexpr_t *keyexpr, _z_bytes_t *payload, _z_encoding_t *encoding,
                                            z_sample_kind_t kind, _z_n_qos_t qos, const _z_timestamp_t *timestamp,
                                            _z_bytes_t *attachment, const _z_source_info_t *source_info) {
    if (_z_get_pending_query_by_id(_Z_RC_IN_VAL(zn), query->_request_id) == NULL) {
        return _Z_ERR_ENTITY_UNKNOWN;
    }

    _z_transport_common_t *transport = _z_session_get_transport_common(_Z_RC_IN_VAL(zn));
    if (transport == NULL) {
        return _Z_ERR_INVALID;
    }

    _z_keyexpr_t keyexpr_copy;
    _Z_RETURN_IF_ERR(_z_keyexpr_copy(&keyexpr_copy, keyexpr));

    _z_network_message_t msg;
    switch (kind) {
        case Z_SAMPLE_KIND_PUT:
            _z_n_msg_make_reply_ok_put(&msg, &_Z_RC_IN_VAL(zn)->_local_zid, query->_request_id, &keyexpr_copy,
                                       Z_RELIABILITY_DEFAULT, Z_CONSOLIDATION_MODE_DEFAULT, qos, timestamp, source_info,
                                       payload, encoding, attachment);
            break;
        case Z_SAMPLE_KIND_DELETE:
            _z_n_msg_make_reply_ok_del(&msg, &_Z_RC_IN_VAL(zn)->_local_zid, query->_request_id, &keyexpr_copy,
                                       Z_RELIABILITY_DEFAULT, Z_CONSOLIDATION_MODE_DEFAULT, qos, timestamp, source_info,
                                       attachment);
            break;
        default:
            _z_keyexpr_clear(&keyexpr_copy);
            return _Z_ERR_INVALID;
    }

    z_result_t ret = _z_handle_network_message(transport, &msg, NULL);

    if (payload != NULL) {
        *payload = _z_bytes_null();
    }
    if (attachment != NULL) {
        *attachment = _z_bytes_null();
    }
    if (encoding != NULL) {
        *encoding = _z_encoding_null();
    }
    return ret;
}

z_result_t _z_session_deliver_reply_err_locally(const _z_query_t *query, const _z_session_rc_t *zn, _z_bytes_t *payload,
                                                _z_encoding_t *encoding, _z_n_qos_t qos) {
    if (_z_get_pending_query_by_id(_Z_RC_IN_VAL(zn), query->_request_id) == NULL) {
        return _Z_ERR_ENTITY_UNKNOWN;
    }

    _z_transport_common_t *transport = _z_session_get_transport_common(_Z_RC_IN_VAL(zn));
    if (transport == NULL) {
        return _Z_ERR_INVALID;
    }

    _z_network_message_t msg;
    _z_n_msg_make_reply_err(&msg, &_Z_RC_IN_VAL(zn)->_local_zid, query->_request_id, Z_RELIABILITY_DEFAULT, qos,
                            payload, encoding, NULL);
    z_result_t ret = _z_handle_network_message(transport, &msg, NULL);

    if (payload != NULL) {
        *payload = _z_bytes_null();
    }
    if (encoding != NULL) {
        *encoding = _z_encoding_null();
    }
    return ret;
}

z_result_t _z_session_deliver_reply_final_locally(_z_session_t *zn, _z_zint_t rid) {
    if (zn == NULL) {
        return _Z_ERR_INVALID;
    }
    _z_pending_query_t *pending = _z_get_pending_query_by_id(zn, rid);
    if (pending == NULL) {
        return _Z_ERR_INVALID;
    }
    return _z_trigger_query_reply_final(zn, rid);
}
#else
z_result_t _z_session_deliver_reply_locally(const _z_query_t *query, const _z_session_rc_t *responder,
                                            const _z_keyexpr_t *keyexpr, _z_bytes_t *payload, _z_encoding_t *encoding,
                                            z_sample_kind_t kind, _z_n_qos_t qos, const _z_timestamp_t *timestamp,
                                            _z_bytes_t *attachment, const _z_source_info_t *source_info) {
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
    return _Z_RES_OK;
}

z_result_t _z_session_deliver_reply_err_locally(const _z_query_t *query, const _z_session_rc_t *zn, _z_bytes_t *payload,
                                                _z_encoding_t *encoding, _z_n_qos_t qos) {
    _ZP_UNUSED(query);
    _ZP_UNUSED(zn);
    _ZP_UNUSED(payload);
    _ZP_UNUSED(encoding);
    _ZP_UNUSED(qos);
    return _Z_RES_OK;
}

z_result_t _z_session_deliver_reply_final_locally(_z_session_t *zn, _z_zint_t rid) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(rid);
    return _Z_RES_OK;
}
#endif  // Z_FEATURE_QUERY == 1
