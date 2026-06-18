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

const _z_qos_t _Z_N_QOS_DEFAULT = {._val = 5};

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

/*=============================*/
/*      Network Messages       */
/*=============================*/

void _z_n_msg_oam_clear(_z_n_msg_oam_t *oam) {
    _z_timestamp_clear(&oam->_ext_timestamp);
    switch (oam->_enc) {
        case _Z_OAM_BODY_UNIT: {
            break;
        }
        case _Z_OAM_BODY_ZINT: {
            break;
        }
        case _Z_OAM_BODY_ZBUF: {
            break;
        }
    }
}

void _z_n_msg_clear(_z_network_message_t *msg) {
    switch (msg->_tag) {
        case _Z_N_PUSH:
            break;
        case _Z_N_REQUEST:
            break;
        case _Z_N_RESPONSE:
            break;
        case _Z_N_RESPONSE_FINAL:
            break;
        case _Z_N_DECLARE:
            break;
        case _Z_N_INTEREST:
            break;
        case _Z_N_OAM:
            _z_n_msg_oam_clear(&msg->_body._oam);
            break;
        default:
            break;
    }
}

void _z_n_msg_make_response_final(_z_network_message_t *msg, _z_zint_t rid) {
    msg->_tag = _Z_N_RESPONSE_FINAL;
    msg->_reliability = Z_RELIABILITY_DEFAULT;
    msg->_body._response_final._request_id = rid;
}

void _z_n_msg_make_declare(_z_network_message_t *msg, _z_declaration_t declaration, _z_optional_id_t interest_id) {
    msg->_tag = _Z_N_DECLARE;
    msg->_reliability = Z_RELIABILITY_DEFAULT;
    msg->_body._declare._interest_id = interest_id;
    msg->_body._declare._decl = declaration;
    msg->_body._declare._ext_qos = _Z_N_QOS_DEFAULT;
    msg->_body._declare._ext_timestamp = _z_timestamp_null();
}

void _z_n_msg_make_query(_z_zenoh_message_t *msg, const _z_wireexpr_t *key, const _z_slice_t *parameters, _z_zint_t qid,
                         z_reliability_t reliability, z_consolidation_mode_t consolidation, const _z_bytes_t *payload,
                         const _z_encoding_t *encoding, uint64_t timeout_ms, const _z_bytes_t *attachment,
                         _z_n_qos_t qos, const _z_source_info_t *source_info, bool implicit_anyke) {
    msg->_tag = _Z_N_REQUEST;
    msg->_reliability = reliability;
    msg->_body._request._tag = _Z_REQUEST_QUERY;
    msg->_body._request._rid = qid;
    msg->_body._request._key = *key;
    msg->_body._request._body._query._parameters = _z_slice_view_from_slice(parameters);
    msg->_body._request._body._query._implicit_anyke = implicit_anyke;
    msg->_body._request._body._query._consolidation = consolidation;
    _z_value_view_create_from_data(&msg->_body._request._body._query._ext_value, payload, encoding);
    msg->_body._request._body._query._ext_info = (source_info == NULL) ? _z_source_info_null() : *source_info;
    msg->_body._request._body._query._ext_attachment = _z_bytes_view_from_bytes(attachment);
    msg->_body._request._ext_budget = 0;
    msg->_body._request._ext_qos = qos;
    msg->_body._request._ext_target = Z_QUERY_TARGET_BEST_MATCHING;
    msg->_body._request._ext_timeout_ms = timeout_ms;
    msg->_body._request._ext_timestamp = _z_timestamp_null();
}

void _z_n_msg_make_push_put(_z_network_message_t *dst, const _z_wireexpr_t *key, const _z_bytes_t *payload,
                            const _z_encoding_t *encoding, _z_n_qos_t qos, const _z_timestamp_t *timestamp,
                            const _z_bytes_t *attachment, z_reliability_t reliability,
                            const _z_source_info_t *source_info) {
    dst->_tag = _Z_N_PUSH;
    dst->_reliability = reliability;
    dst->_body._push._key = *key;
    dst->_body._push._qos = qos;
    dst->_body._push._timestamp = _z_timestamp_null();
    dst->_body._push._body._is_put = true;
    dst->_body._push._body._body._put._commons._timestamp = (timestamp == NULL) ? _z_timestamp_null() : *timestamp;
    dst->_body._push._body._body._put._commons._source_info =
        (source_info == NULL) ? _z_source_info_null() : *source_info;
    dst->_body._push._body._body._put._payload = _z_bytes_view_from_bytes(payload);
    dst->_body._push._body._body._put._encoding = _z_encoding_view_from_encoding(encoding);
    dst->_body._push._body._body._put._attachment = _z_bytes_view_from_bytes(attachment);
}

void _z_n_msg_make_push_del(_z_network_message_t *dst, const _z_wireexpr_t *key, _z_n_qos_t qos,
                            const _z_timestamp_t *timestamp, z_reliability_t reliability,
                            const _z_source_info_t *source_info) {
    dst->_tag = _Z_N_PUSH;
    dst->_reliability = reliability;
    dst->_body._push._key = *key;
    dst->_body._push._qos = qos;
    dst->_body._push._timestamp = _z_timestamp_null();
    dst->_body._push._body._is_put = false;
    dst->_body._push._body._body._del._commons._timestamp = (timestamp == NULL) ? _z_timestamp_null() : *timestamp;
    dst->_body._push._body._body._del._commons._source_info =
        (source_info == NULL) ? _z_source_info_null() : *source_info;
    dst->_body._push._body._body._del._attachment = _z_bytes_view_null();
}

void _z_n_msg_make_reply_ok_put(_z_network_message_t *dst, const _z_id_t *zid, _z_zint_t rid, const _z_wireexpr_t *key,
                                z_reliability_t reliability, z_consolidation_mode_t consolidation, _z_n_qos_t qos,
                                const _z_timestamp_t *timestamp, const _z_source_info_t *source_info,
                                const _z_bytes_t *payload, const _z_encoding_t *encoding,
                                const _z_bytes_t *attachment) {
    dst->_tag = _Z_N_RESPONSE;
    dst->_reliability = reliability;
    dst->_body._response._tag = _Z_RESPONSE_BODY_REPLY;
    dst->_body._response._request_id = rid;
    dst->_body._response._key = *key;
    dst->_body._response._body._reply._consolidation = consolidation;
    dst->_body._response._body._reply._body._is_put = true;
    dst->_body._response._body._reply._body._body._put._commons._timestamp =
        (timestamp == NULL) ? _z_timestamp_null() : *timestamp;
    dst->_body._response._body._reply._body._body._put._commons._source_info =
        (source_info == NULL) ? _z_source_info_null() : *source_info;
    dst->_body._response._body._reply._body._body._put._payload = _z_bytes_view_from_bytes(payload);
    dst->_body._response._body._reply._body._body._put._encoding = _z_encoding_view_from_encoding(encoding);
    dst->_body._response._body._reply._body._body._put._attachment = _z_bytes_view_from_bytes(attachment);
    dst->_body._response._ext_qos = qos;
    dst->_body._response._ext_timestamp = _z_timestamp_null();
    dst->_body._response._ext_responder._eid = 0;
    dst->_body._response._ext_responder._zid = *zid;
}

void _z_n_msg_make_reply_ok_del(_z_network_message_t *dst, const _z_id_t *zid, _z_zint_t rid, const _z_wireexpr_t *key,
                                z_reliability_t reliability, z_consolidation_mode_t consolidation, _z_n_qos_t qos,
                                const _z_timestamp_t *timestamp, const _z_source_info_t *source_info,
                                const _z_bytes_t *attachment) {
    dst->_tag = _Z_N_RESPONSE;
    dst->_reliability = reliability;
    dst->_body._response._tag = _Z_RESPONSE_BODY_REPLY;
    dst->_body._response._request_id = rid;
    dst->_body._response._key = *key;
    dst->_body._response._body._reply._consolidation = consolidation;
    dst->_body._response._body._reply._body._is_put = false;
    dst->_body._response._body._reply._body._body._del._commons._timestamp =
        (timestamp == NULL) ? _z_timestamp_null() : *timestamp;
    dst->_body._response._body._reply._body._body._del._commons._source_info =
        (source_info == NULL) ? _z_source_info_null() : *source_info;
    dst->_body._response._body._reply._body._body._del._attachment = _z_bytes_view_from_bytes(attachment);
    dst->_body._response._ext_timestamp = _z_timestamp_null();
    dst->_body._response._ext_qos = qos;
    dst->_body._response._ext_responder._eid = 0;
    dst->_body._response._ext_responder._zid = *zid;
}

void _z_n_msg_make_reply_err(_z_network_message_t *dst, const _z_id_t *zid, _z_zint_t rid, z_reliability_t reliability,
                             _z_n_qos_t qos, const _z_bytes_t *payload, const _z_encoding_t *encoding,
                             const _z_source_info_t *source_info) {
    dst->_tag = _Z_N_RESPONSE;
    dst->_reliability = reliability;
    dst->_body._response._tag = _Z_RESPONSE_BODY_ERR;
    dst->_body._response._request_id = rid;
    dst->_body._response._key = _z_wireexpr_null();
    dst->_body._response._body._err._payload = _z_bytes_view_from_bytes(payload);
    dst->_body._response._body._err._encoding = _z_encoding_view_from_encoding(encoding);
    dst->_body._response._body._err._ext_source_info = (source_info == NULL) ? _z_source_info_null() : *source_info;
    dst->_body._response._ext_timestamp = _z_timestamp_null();
    dst->_body._response._ext_qos = qos;
    dst->_body._response._ext_responder._eid = 0;
    dst->_body._response._ext_responder._zid = *zid;
}

void _z_n_msg_make_interest(_z_network_message_t *msg, _z_interest_t interest) {
    msg->_tag = _Z_N_INTEREST;
    msg->_reliability = Z_RELIABILITY_DEFAULT;
    msg->_body._interest._interest = interest;
}
