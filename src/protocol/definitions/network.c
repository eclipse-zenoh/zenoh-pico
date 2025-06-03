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

void _z_n_msg_request_clear(_z_n_msg_request_t *msg) {
    _z_keyexpr_clear(&msg->_key);
    switch (msg->_tag) {
        case _Z_REQUEST_QUERY: {
            _z_msg_query_clear(&msg->_body._query);
        } break;
        case _Z_REQUEST_PUT: {
            _z_msg_put_clear(&msg->_body._put);
        } break;
        case _Z_REQUEST_DEL: {
            _z_msg_del_clear(&msg->_body._del);
        } break;
    }
}

/*=============================*/
/*      Network Messages       */
/*=============================*/
void _z_push_body_clear(_z_push_body_t *msg) {
    if (msg->_is_put) {
        _z_msg_put_clear(&msg->_body._put);
    }
}
_z_push_body_t _z_push_body_steal(_z_push_body_t *msg) {
    _z_push_body_t ret = *msg;
    *msg = _z_push_body_null();
    return ret;
}

static z_result_t _z_push_body_copy(_z_push_body_t *dst, const _z_push_body_t *src) {
    if (src->_is_put) {
        _Z_RETURN_IF_ERR(_z_bytes_copy(&dst->_body._put._attachment, &src->_body._put._attachment));
        _Z_RETURN_IF_ERR(_z_bytes_copy(&dst->_body._put._payload, &src->_body._put._payload));
    } else {
        _Z_RETURN_IF_ERR(_z_bytes_copy(&dst->_body._del._attachment, &src->_body._del._attachment));
    }
    return _Z_RES_OK;
}

void _z_n_msg_response_final_clear(_z_n_msg_response_final_t *msg) { (void)(msg); }

void _z_n_msg_push_clear(_z_n_msg_push_t *msg) {
    _z_keyexpr_clear(&msg->_key);
    _z_push_body_clear(&msg->_body);
}

void _z_n_msg_response_clear(_z_n_msg_response_t *msg) {
    _z_timestamp_clear(&msg->_ext_timestamp);
    _z_keyexpr_clear(&msg->_key);
    switch (msg->_tag) {
        case _Z_RESPONSE_BODY_REPLY: {
            _z_msg_reply_clear(&msg->_body._reply);
            break;
        }
        case _Z_RESPONSE_BODY_ERR: {
            _z_msg_err_clear(&msg->_body._err);
            break;
        }
    }
}

void _z_n_msg_clear(_z_network_message_t *msg) {
    switch (msg->_tag) {
        case _Z_N_PUSH:
            _z_n_msg_push_clear(&msg->_body._push);
            break;
        case _Z_N_REQUEST:
            _z_n_msg_request_clear(&msg->_body._request);
            break;
        case _Z_N_RESPONSE:
            _z_n_msg_response_clear(&msg->_body._response);
            break;
        case _Z_N_RESPONSE_FINAL:
            _z_n_msg_response_final_clear(&msg->_body._response_final);
            break;
        case _Z_N_DECLARE:
            _z_n_msg_declare_clear(&msg->_body._declare);
            break;
        case _Z_N_INTEREST:
            _z_n_msg_interest_clear(&msg->_body._interest);
            break;
        default:
            break;
    }
}

void _z_n_msg_free(_z_network_message_t **msg) {
    _z_network_message_t *ptr = *msg;

    if (ptr != NULL) {
        _z_n_msg_clear(ptr);

        z_free(ptr);
        *msg = NULL;
    }
}

_z_network_message_t _z_n_msg_make_response_final(_z_zint_t rid) {
    return (_z_zenoh_message_t){
        ._tag = _Z_N_RESPONSE_FINAL,
        ._reliability = Z_RELIABILITY_DEFAULT,
        ._body = {._response_final = {._request_id = rid}},
    };
}
_z_network_message_t _z_n_msg_make_declare(_z_declaration_t declaration, bool has_interest_id, uint32_t interest_id) {
    return (_z_network_message_t){
        ._tag = _Z_N_DECLARE,
        ._reliability = Z_RELIABILITY_DEFAULT,
        ._body._declare =
            {
                .has_interest_id = has_interest_id,
                ._interest_id = interest_id,
                ._decl = declaration,
                ._ext_qos = _Z_N_QOS_DEFAULT,
                ._ext_timestamp = _z_timestamp_null(),
            },
    };
}

void _z_n_msg_make_query(_z_zenoh_message_t *msg, const _z_keyexpr_t *key, const _z_slice_t *parameters, _z_zint_t qid,
                         z_consolidation_mode_t consolidation, const _z_bytes_t *payload, const _z_encoding_t *encoding,
                         uint64_t timeout_ms, const _z_bytes_t *attachment, _z_n_qos_t qos,
                         const _z_source_info_t *source_info) {
    msg->_tag = _Z_N_REQUEST, msg->_reliability = Z_RELIABILITY_DEFAULT,
    msg->_body._request = (_z_n_msg_request_t){
        ._rid = qid,
        ._key = *key,
        ._tag = _Z_REQUEST_QUERY,
        ._body._query =
            {
                ._parameters = *parameters,
                ._consolidation = consolidation,
                ._ext_value = {.payload = (payload == NULL) ? _z_bytes_null() : *payload,
                               .encoding = (encoding == NULL) ? _z_encoding_null() : *encoding},
                ._ext_info = *source_info,
                ._ext_attachment = (attachment == NULL) ? _z_bytes_null() : *attachment,
            },
        ._ext_budget = 0,
        ._ext_qos = qos,
        ._ext_target = Z_QUERY_TARGET_BEST_MATCHING,
        ._ext_timeout_ms = timeout_ms,
        ._ext_timestamp = _z_timestamp_null(),
    };
}

void _z_n_msg_make_push_put(_z_network_message_t *dst, const _z_keyexpr_t *key, const _z_bytes_t *payload,
                            const _z_encoding_t *encoding, _z_n_qos_t qos, const _z_timestamp_t *timestamp,
                            const _z_bytes_t *attachment, z_reliability_t reliability,
                            const _z_source_info_t *source_info) {
    dst->_tag = _Z_N_PUSH;
    dst->_reliability = reliability;
    dst->_body._push = (_z_n_msg_push_t){
        ._key = *key,
        ._qos = qos,
        ._timestamp = _z_timestamp_null(),
        ._body._is_put = true,
        ._body._body._put =
            {
                ._commons = {._timestamp = ((timestamp == NULL) ? _z_timestamp_null() : *timestamp),
                             ._source_info = ((source_info == NULL) ? _z_source_info_null() : *source_info)},
                ._payload = (payload == NULL) ? _z_bytes_null() : *payload,
                ._encoding = (encoding == NULL) ? _z_encoding_null() : *encoding,
                ._attachment = (attachment == NULL) ? _z_bytes_null() : *attachment,
            },
    };
}

void _z_n_msg_make_push_del(_z_network_message_t *dst, const _z_keyexpr_t *key, _z_n_qos_t qos,
                            const _z_timestamp_t *timestamp, z_reliability_t reliability,
                            const _z_source_info_t *source_info) {
    dst->_tag = _Z_N_PUSH;
    dst->_reliability = reliability;
    dst->_body._push = (_z_n_msg_push_t){
        ._key = *key,
        ._qos = qos,
        ._timestamp = _z_timestamp_null(),
        ._body._is_put = false,
        ._body._body._del = {._commons = {._timestamp = ((timestamp == NULL) ? _z_timestamp_null() : *timestamp),
                                          ._source_info =
                                              ((source_info == NULL) ? _z_source_info_null() : *source_info)}},
    };
}

void _z_n_msg_make_reply_ok_put(_z_network_message_t *dst, const _z_id_t *zid, _z_zint_t rid, const _z_keyexpr_t *key,
                                z_reliability_t reliability, z_consolidation_mode_t consolidation, _z_n_qos_t qos,
                                const _z_timestamp_t *timestamp, const _z_source_info_t *source_info,
                                const _z_bytes_t *payload, const _z_encoding_t *encoding,
                                const _z_bytes_t *attachment) {
    dst->_tag = _Z_N_RESPONSE;
    dst->_reliability = reliability;
    dst->_body._response = (_z_n_msg_response_t){
        ._key = *key,
        ._tag = _Z_RESPONSE_BODY_REPLY,
        ._request_id = rid,
        ._body._reply =
            {
                ._consolidation = consolidation,
                ._body._is_put = true,
                ._body._body._put =
                    {
                        ._commons = {._timestamp = (timestamp == NULL) ? _z_timestamp_null() : *timestamp,
                                     ._source_info = *source_info},
                        ._payload = (payload == NULL) ? _z_bytes_null() : *payload,
                        ._encoding = (encoding == NULL) ? _z_encoding_null() : *encoding,
                        ._attachment = (attachment == NULL) ? _z_bytes_null() : *attachment,
                    },
            },
        ._ext_qos = qos,
        ._ext_timestamp = _z_timestamp_null(),
        ._ext_responder = {._eid = 0, ._zid = *zid},
    };
}
void _z_n_msg_make_reply_ok_del(_z_network_message_t *dst, const _z_id_t *zid, _z_zint_t rid, const _z_keyexpr_t *key,
                                z_reliability_t reliability, z_consolidation_mode_t consolidation, _z_n_qos_t qos,
                                const _z_timestamp_t *timestamp, const _z_source_info_t *source_info,
                                const _z_bytes_t *attachment) {
    dst->_tag = _Z_N_RESPONSE;
    dst->_reliability = reliability;
    dst->_body._response = (_z_n_msg_response_t){
        ._request_id = rid,
        ._key = *key,
        ._tag = _Z_RESPONSE_BODY_REPLY,
        ._body._reply =
            {
                ._consolidation = consolidation,
                ._body._is_put = false,
                ._body._body._del =
                    {
                        ._commons =
                            {
                                ._timestamp = (timestamp == NULL) ? _z_timestamp_null() : *timestamp,
                                ._source_info = *source_info,
                            },
                        ._attachment = (attachment == NULL) ? _z_bytes_null() : *attachment,
                    },
            },
        ._ext_timestamp = _z_timestamp_null(),
        ._ext_qos = qos,
        ._ext_responder = {._zid = *zid, ._eid = 0},
    };
}

void _z_n_msg_make_reply_err(_z_network_message_t *dst, const _z_id_t *zid, _z_zint_t rid, z_reliability_t reliability,
                             _z_n_qos_t qos, const _z_bytes_t *payload, const _z_encoding_t *encoding,
                             const _z_source_info_t *source_info) {
    dst->_tag = _Z_N_RESPONSE;
    dst->_reliability = reliability;
    dst->_body._response = (_z_n_msg_response_t){
        ._request_id = rid,
        ._tag = _Z_RESPONSE_BODY_ERR,
        ._body._err =
            {
                ._payload = (payload == NULL) ? _z_bytes_null() : *payload,
                ._encoding = (encoding == NULL) ? _z_encoding_null() : *encoding,
                ._ext_source_info = *source_info,
            },
        ._ext_timestamp = _z_timestamp_null(),
        ._ext_qos = qos,
        ._ext_responder = {._zid = *zid, ._eid = 0},
    };
}

_z_network_message_t _z_n_msg_make_interest(_z_interest_t interest) {
    return (_z_network_message_t){
        ._tag = _Z_N_INTEREST,
        ._reliability = Z_RELIABILITY_DEFAULT,
        ._body._interest =
            {
                ._interest = interest,
            },
    };
}

static z_result_t _z_n_msg_push_copy(_z_network_message_t *dst, const _z_network_message_t *src) {
    memcpy(dst, src, sizeof(_z_network_message_t));
    _Z_RETURN_IF_ERR(_z_keyexpr_copy(&dst->_body._push._key, &src->_body._push._key));
    return _z_push_body_copy(&dst->_body._push._body, &src->_body._push._body);
}

static z_result_t _z_n_msg_request_copy(_z_network_message_t *dst, const _z_network_message_t *src) {
    memcpy(dst, src, sizeof(_z_network_message_t));
    _Z_RETURN_IF_ERR(_z_keyexpr_copy(&dst->_body._request._key, &src->_body._request._key));
    switch (src->_body._request._tag) {
        case _Z_REQUEST_QUERY:
            _Z_RETURN_IF_ERR(_z_slice_copy(&dst->_body._request._body._query._parameters,
                                           &src->_body._request._body._query._parameters));
            _Z_RETURN_IF_ERR(_z_bytes_copy(&dst->_body._request._body._query._ext_attachment,
                                           &src->_body._request._body._query._ext_attachment));
            _Z_RETURN_IF_ERR(_z_bytes_copy(&dst->_body._request._body._query._ext_value.payload,
                                           &src->_body._request._body._query._ext_value.payload));
            break;
        case _Z_REQUEST_PUT:
            _Z_RETURN_IF_ERR(_z_bytes_copy(&dst->_body._request._body._put._attachment,
                                           &src->_body._request._body._put._attachment));
            _Z_RETURN_IF_ERR(
                _z_bytes_copy(&dst->_body._request._body._put._payload, &src->_body._request._body._put._payload));
            break;
        case _Z_REQUEST_DEL:
            _Z_RETURN_IF_ERR(_z_bytes_copy(&dst->_body._request._body._del._attachment,
                                           &src->_body._request._body._del._attachment));
            break;
    }
    return _Z_RES_OK;
}

static z_result_t _z_n_msg_response_copy(_z_network_message_t *dst, const _z_network_message_t *src) {
    memcpy(dst, src, sizeof(_z_network_message_t));
    _Z_RETURN_IF_ERR(_z_keyexpr_copy(&dst->_body._response._key, &src->_body._response._key));
    switch (src->_body._response._tag) {
        case _Z_RESPONSE_BODY_REPLY:
            _Z_RETURN_IF_ERR(
                _z_push_body_copy(&dst->_body._response._body._reply._body, &src->_body._response._body._reply._body));
            break;
        case _Z_RESPONSE_BODY_ERR:
            _Z_RETURN_IF_ERR(
                _z_bytes_copy(&dst->_body._response._body._err._payload, &src->_body._response._body._err._payload));
            break;
    }
    return _Z_RES_OK;
}

static z_result_t _z_n_msg_response_final_copy(_z_network_message_t *dst, const _z_network_message_t *src) {
    memcpy(dst, src, sizeof(_z_network_message_t));
    return _Z_RES_OK;
}

static z_result_t _z_n_msg_declare_copy(_z_network_message_t *dst, const _z_network_message_t *src) {
    memcpy(dst, src, sizeof(_z_network_message_t));
    const _z_declaration_t *src_decl = &src->_body._declare._decl;
    _z_declaration_t *dst_decl = &dst->_body._declare._decl;
    switch (src_decl->_tag) {
        case _Z_DECL_KEXPR: {
            _Z_RETURN_IF_ERR(
                _z_keyexpr_copy(&dst_decl->_body._decl_kexpr._keyexpr, &src_decl->_body._decl_kexpr._keyexpr));
        } break;
        case _Z_DECL_SUBSCRIBER: {
            _Z_RETURN_IF_ERR(_z_keyexpr_copy(&dst_decl->_body._decl_subscriber._keyexpr,
                                             &src_decl->_body._decl_subscriber._keyexpr));
        } break;
        case _Z_UNDECL_SUBSCRIBER: {
            _Z_RETURN_IF_ERR(_z_keyexpr_copy(&dst_decl->_body._undecl_subscriber._ext_keyexpr,
                                             &src_decl->_body._undecl_subscriber._ext_keyexpr));
        } break;
        case _Z_DECL_QUERYABLE: {
            _Z_RETURN_IF_ERR(
                _z_keyexpr_copy(&dst_decl->_body._decl_queryable._keyexpr, &src_decl->_body._decl_queryable._keyexpr));
        } break;
        case _Z_UNDECL_QUERYABLE: {
            _Z_RETURN_IF_ERR(_z_keyexpr_copy(&dst_decl->_body._undecl_queryable._ext_keyexpr,
                                             &src_decl->_body._undecl_queryable._ext_keyexpr));
        } break;
        case _Z_DECL_TOKEN: {
            _Z_RETURN_IF_ERR(
                _z_keyexpr_copy(&dst_decl->_body._decl_token._keyexpr, &src_decl->_body._decl_token._keyexpr));
        } break;
        case _Z_UNDECL_TOKEN: {
            _Z_RETURN_IF_ERR(_z_keyexpr_copy(&dst_decl->_body._undecl_token._ext_keyexpr,
                                             &src_decl->_body._undecl_token._ext_keyexpr));
        } break;
        default:
            break;
    }
    return _Z_RES_OK;
}

static z_result_t _z_n_msg_interest_copy(_z_network_message_t *dst, const _z_network_message_t *src) {
    memcpy(dst, src, sizeof(_z_network_message_t));
    _Z_RETURN_IF_ERR(
        _z_keyexpr_copy(&dst->_body._interest._interest._keyexpr, &src->_body._interest._interest._keyexpr));
    return _Z_RES_OK;
}

z_result_t _z_n_msg_copy(_z_network_message_t *dst, const _z_network_message_t *src) {
    switch (src->_tag) {
        case _Z_N_PUSH:
            return _z_n_msg_push_copy(dst, src);
        case _Z_N_REQUEST:
            return _z_n_msg_request_copy(dst, src);
        case _Z_N_RESPONSE:
            return _z_n_msg_response_copy(dst, src);
        case _Z_N_RESPONSE_FINAL:
            return _z_n_msg_response_final_copy(dst, src);
        case _Z_N_DECLARE:
            return _z_n_msg_declare_copy(dst, src);
        case _Z_N_INTEREST:
            return _z_n_msg_interest_copy(dst, src);
        default:
            return _Z_ERR_ENTITY_UNKNOWN;
    }
}

_z_network_message_t *_z_n_msg_clone(const _z_network_message_t *src) {
    _z_network_message_t *dst = (_z_network_message_t *)z_malloc(sizeof(_z_network_message_t));
    _z_n_msg_copy(dst, src);
    return dst;
}

void _z_msg_fix_mapping(_z_zenoh_message_t *msg, uintptr_t mapping) {
    switch (msg->_tag) {
        case _Z_N_DECLARE: {
            _z_decl_fix_mapping(&msg->_body._declare._decl, mapping);
        } break;
        case _Z_N_PUSH: {
            _z_keyexpr_fix_mapping(&msg->_body._push._key, mapping);
        } break;
        case _Z_N_REQUEST: {
            _z_keyexpr_fix_mapping(&msg->_body._request._key, mapping);
        } break;
        case _Z_N_RESPONSE: {
            _z_keyexpr_fix_mapping(&msg->_body._response._key, mapping);
        } break;
        case _Z_N_INTEREST: {
            _z_keyexpr_fix_mapping(&msg->_body._interest._interest._keyexpr, mapping);
        } break;
        default:
            break;
    }
}
