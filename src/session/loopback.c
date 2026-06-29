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
#include "zenoh-pico/session/interest.h"
#include "zenoh-pico/session/keyexpr.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/reply.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/utils/locality.h"

#if Z_FEATURE_SUBSCRIPTION == 1 && Z_FEATURE_LOCAL_SUBSCRIBER == 1
z_result_t _z_session_deliver_push_locally(_z_session_t *zn, const _z_keyexpr_t *keyexpr, const _z_bytes_t *payload,
                                           const _z_encoding_t *encoding, z_sample_kind_t kind, _z_n_qos_t qos,
                                           const _z_timestamp_t *timestamp, const _z_bytes_t *attachment,
                                           z_reliability_t reliability, const _z_source_info_t *source_info) {
    return _z_trigger_subscriptions_impl(zn, _Z_SUBSCRIBER_KIND_SUBSCRIBER, keyexpr, payload, encoding, kind, timestamp,
                                         qos, attachment, reliability, source_info, NULL);
}
#else
z_result_t _z_session_deliver_push_locally(_z_session_t *zn, const _z_keyexpr_t *keyexpr, const _z_bytes_t *payload,
                                           const _z_encoding_t *encoding, z_sample_kind_t kind, _z_n_qos_t qos,
                                           const _z_timestamp_t *timestamp, const _z_bytes_t *attachment,
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
                                            z_consolidation_mode_t consolidation, const _z_bytes_t *payload,
                                            const _z_encoding_t *encoding, const _z_bytes_t *attachment,
                                            const _z_source_info_t *source_info, _z_zint_t qid, uint64_t timeout_ms,
                                            _z_n_qos_t qos, bool implicit_anyke) {
    (void)timeout_ms;
    _z_msg_query_t msg;
    _z_msg_query_fill(&msg, parameters, consolidation, payload, encoding, source_info, attachment, implicit_anyke);
    return _z_trigger_queryables(zn, keyexpr, &msg, (uint32_t)qid, qos, NULL);
}
#else
z_result_t _z_session_deliver_query_locally(_z_session_t *zn, const _z_keyexpr_t *keyexpr, const _z_slice_t *parameters,
                                            z_consolidation_mode_t consolidation, const _z_bytes_t *payload,
                                            const _z_encoding_t *encoding, const _z_bytes_t *attachment,
                                            const _z_source_info_t *source_info, _z_zint_t qid, uint64_t timeout_ms,
                                            _z_n_qos_t qos, bool implicit_anyke) {
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
    _ZP_UNUSED(implicit_anyke);
    return _Z_RES_OK;
}
#endif  // Z_FEATURE_QUERYABLE == 1

#if Z_FEATURE_QUERY == 1
z_result_t _z_session_deliver_reply_locally(const _z_query_t *query, _z_session_t *zn, const _z_keyexpr_t *keyexpr,
                                            const _z_bytes_t *payload, const _z_encoding_t *encoding,
                                            z_sample_kind_t kind, _z_n_qos_t qos, const _z_timestamp_t *timestamp,
                                            const _z_bytes_t *attachment, const _z_source_info_t *source_info) {
    _z_entity_global_id_t replier_id = _z_session_get_id(zn);
    _z_msg_reply_t msg;
    msg._consolidation = Z_CONSOLIDATION_MODE_DEFAULT;
    switch (kind) {
        case Z_SAMPLE_KIND_PUT:
            msg._body._is_put = true;
            _z_msg_put_fill(&msg._body._body._put, timestamp, source_info, payload, encoding, attachment);
            return _z_trigger_query_reply_partial(zn, _z_query_get_ref(query)->_id.rid, keyexpr, &msg, &replier_id, qos,
                                                  NULL);
        case Z_SAMPLE_KIND_DELETE:
            msg._body._is_put = false;
            _z_msg_del_fill(&msg._body._body._del, timestamp, source_info, attachment);
            return _z_trigger_query_reply_partial(zn, _z_query_get_ref(query)->_id.rid, keyexpr, &msg, &replier_id, qos,
                                                  NULL);
        default:
            return _Z_ERR_INVALID;
    }
}

z_result_t _z_session_deliver_reply_err_locally(const _z_query_t *query, _z_session_t *zn, const _z_bytes_t *payload,
                                                const _z_encoding_t *encoding, _z_n_qos_t qos) {
    (void)qos;

    _z_msg_err_t msg;
    _z_reply_err_fill(&msg, payload, encoding, NULL);
    _z_entity_global_id_t replier_id = _z_session_get_id(zn);

    return _z_trigger_reply_err(zn, _z_query_get_ref(query)->_id.rid, &msg, &replier_id);
}

z_result_t _z_session_deliver_reply_final_locally(_z_session_t *zn, _z_zint_t rid) {
    if (zn == NULL) {
        return _Z_ERR_INVALID;
    }
    return _z_trigger_query_reply_final(zn, rid);
}
#else
z_result_t _z_session_deliver_reply_locally(const _z_query_t *query, _z_session_t *responder,
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
    return _Z_RES_OK;
}

z_result_t _z_session_deliver_reply_err_locally(const _z_query_t *query, _z_session_t *zn, const _z_bytes_t *payload,
                                                const _z_encoding_t *encoding, _z_n_qos_t qos) {
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
