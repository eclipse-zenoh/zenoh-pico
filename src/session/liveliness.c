//
// Copyright (c) 2024 ZettaScale Technology
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

#include "zenoh-pico/api/liveliness.h"

#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/net/reply.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"

#if Z_FEATURE_LIVELINESS == 1

/**************** Liveliness Token ****************/

z_result_t _z_liveliness_register_token(_z_session_t *zn, uint32_t id, const _z_keyexpr_t *keyexpr) {
    z_result_t ret = _Z_RES_OK;

    _Z_DEBUG("Register liveliness token (%i:%.*s)", (int)id, (int)_z_string_len(&keyexpr->_suffix),
             _z_string_data(&keyexpr->_suffix));

    _z_session_mutex_lock(zn);

    const _z_keyexpr_t *pkeyexpr = _z_keyexpr_intmap_get(&zn->_local_tokens, id);
    if (pkeyexpr != NULL) {
        _Z_ERROR("Duplicate token id %i", (int)id);
        ret = _Z_ERR_ENTITY_DECLARATION_FAILED;
    } else {
        _z_keyexpr_intmap_insert(&zn->_local_tokens, id, _z_keyexpr_clone(keyexpr));
    }

    _z_session_mutex_unlock(zn);

    return ret;
}

void _z_liveliness_unregister_token(_z_session_t *zn, uint32_t id) {
    _z_session_mutex_lock(zn);

    _Z_DEBUG("Unregister liveliness token (%i)", (int)id);

    _z_keyexpr_intmap_remove(&zn->_local_tokens, id);

    _z_session_mutex_unlock(zn);
}

/**************** Liveliness Subscriber ****************/

#if Z_FEATURE_SUBSCRIPTION == 1
z_result_t _z_liveliness_subscription_declare(_z_session_t *zn, uint32_t id, const _z_keyexpr_t *keyexpr,
                                              const _z_timestamp_t *timestamp) {
    z_result_t ret = _Z_RES_OK;

    _z_session_mutex_lock(zn);

    const _z_keyexpr_t *pkeyexpr = _z_keyexpr_intmap_get(&zn->_remote_tokens, id);
    if (pkeyexpr != NULL) {
        _Z_ERROR("Duplicate token id %i", (int)id);
        ret = _Z_ERR_ENTITY_DECLARATION_FAILED;
    } else {
        _z_keyexpr_intmap_insert(&zn->_remote_tokens, id, _z_keyexpr_clone(keyexpr));
    }

    _z_session_mutex_unlock(zn);

    if (ret == _Z_RES_OK) {
        _z_keyexpr_t key = _z_keyexpr_alias(keyexpr);
        ret = _z_trigger_liveliness_subscriptions_declare(zn, &key, timestamp);
    }

    return ret;
}

z_result_t _z_liveliness_subscription_undeclare(_z_session_t *zn, uint32_t id, const _z_timestamp_t *timestamp) {
    z_result_t ret = _Z_RES_OK;

    _z_keyexpr_t *key = NULL;
    _z_session_mutex_lock(zn);
    const _z_keyexpr_t *keyexpr = _z_keyexpr_intmap_get(&zn->_remote_tokens, id);
    if (keyexpr != NULL) {
        key = _z_keyexpr_clone(keyexpr);
        _z_keyexpr_intmap_remove(&zn->_remote_tokens, id);
    } else {
        ret = _Z_ERR_ENTITY_UNKNOWN;
    }
    _z_session_mutex_unlock(zn);

    if (key != NULL) {
        ret = _z_trigger_liveliness_subscriptions_undeclare(zn, key, timestamp);
        _z_keyexpr_free(&key);
    }

    return ret;
}

z_result_t _z_liveliness_subscription_undeclare_all(_z_session_t *zn) {
    z_result_t ret = _Z_RES_OK;

    _z_session_mutex_lock(zn);
    _z_keyexpr_intmap_t token_list = _z_keyexpr_intmap_clone(&zn->_remote_tokens);
    _z_keyexpr_intmap_clear(&zn->_remote_tokens);
    _z_session_mutex_unlock(zn);

    _z_keyexpr_intmap_iterator_t iter = _z_keyexpr_intmap_iterator_make(&token_list);
    _z_timestamp_t tm = _z_timestamp_null();
    while (_z_keyexpr_intmap_iterator_next(&iter)) {
        _z_keyexpr_t key = *_z_keyexpr_intmap_iterator_value(&iter);
        ret = _z_trigger_liveliness_subscriptions_undeclare(zn, &key, &tm);
        if (ret != _Z_RES_OK) {
            break;
        }
    }
    _z_keyexpr_intmap_clear(&token_list);

    return ret;
}

z_result_t _z_liveliness_subscription_trigger_history(_z_session_t *zn, const _z_keyexpr_t *keyexpr) {
    z_result_t ret = _Z_RES_OK;

    _Z_DEBUG("Retrieve liveliness history for %.*s", (int)_z_string_len(&keyexpr->_suffix),
             _z_string_data(&keyexpr->_suffix));

    _z_session_mutex_lock(zn);
    _z_keyexpr_intmap_t token_list = _z_keyexpr_intmap_clone(&zn->_remote_tokens);
    _z_session_mutex_unlock(zn);

    _z_keyexpr_intmap_iterator_t iter = _z_keyexpr_intmap_iterator_make(&token_list);
    _z_timestamp_t tm = _z_timestamp_null();
    while (_z_keyexpr_intmap_iterator_next(&iter)) {
        _z_keyexpr_t key = *_z_keyexpr_intmap_iterator_value(&iter);
        if (_z_keyexpr_suffix_intersects(&key, keyexpr)) {
            ret = _z_trigger_liveliness_subscriptions_declare(zn, &key, &tm);
            if (ret != _Z_RES_OK) {
                break;
            }
        }
    }
    _z_keyexpr_intmap_clear(&token_list);

    return ret;
}
#endif  // Z_FEATURE_SUBSCRIPTION == 1

/**************** Liveliness Query ****************/

#if Z_FEATURE_QUERY == 1

void _z_liveliness_pending_query_clear(_z_liveliness_pending_query_t *pen_qry) {
    if (pen_qry->_dropper != NULL) {
        pen_qry->_dropper(pen_qry->_arg);
    }
    _z_keyexpr_clear(&pen_qry->_key);
}

void _z_liveliness_pending_query_copy(_z_liveliness_pending_query_t *dst, const _z_liveliness_pending_query_t *src) {
    dst->_arg = src->_arg;
    dst->_callback = src->_callback;
    dst->_dropper = src->_dropper;
    _z_keyexpr_copy(&dst->_key, &src->_key);
}

_z_liveliness_pending_query_t *_z_liveliness_pending_query_clone(const _z_liveliness_pending_query_t *src) {
    _z_liveliness_pending_query_t *dst = z_malloc(sizeof(_z_liveliness_pending_query_t));
    if (dst != NULL) {
        _z_liveliness_pending_query_copy(dst, src);
    }
    return dst;
}

uint32_t _z_liveliness_get_query_id(_z_session_t *zn) { return zn->_liveliness_query_id++; }

z_result_t _z_liveliness_register_pending_query(_z_session_t *zn, uint32_t id, _z_liveliness_pending_query_t *pen_qry) {
    z_result_t ret = _Z_RES_OK;

    _Z_DEBUG("Register liveliness query for (%ju:%.*s)", (uintmax_t)pen_qry->_key._id,
             (int)_z_string_len(&pen_qry->_key._suffix), _z_string_data(&pen_qry->_key._suffix));

    _z_session_mutex_lock(zn);

    const _z_liveliness_pending_query_t *pq =
        _z_liveliness_pending_query_intmap_get(&zn->_liveliness_pending_queries, id);
    if (pq != NULL) {
        _Z_ERROR("Duplicate liveliness query id %i", (int)id);
        ret = _Z_ERR_ENTITY_DECLARATION_FAILED;
    } else {
        _z_liveliness_pending_query_intmap_insert(&zn->_liveliness_pending_queries, id, pen_qry);
    }

    _z_session_mutex_unlock(zn);

    return ret;
}

z_result_t _z_liveliness_pending_query_reply(_z_session_t *zn, uint32_t interest_id, const _z_keyexpr_t *keyexpr,
                                             const _z_timestamp_t *timestamp) {
    z_result_t ret = _Z_RES_OK;

    _z_session_mutex_lock(zn);

    const _z_liveliness_pending_query_t *pq =
        _z_liveliness_pending_query_intmap_get(&zn->_liveliness_pending_queries, interest_id);
    if (pq == NULL) {
        ret = _Z_ERR_ENTITY_UNKNOWN;
    }

    _Z_DEBUG("Liveliness pending query reply %i resolve result %i", (int)interest_id, ret);

    if (ret == _Z_RES_OK) {
        _Z_DEBUG("Resolving %d - %.*s on mapping 0x%x", keyexpr->_id, (int)_z_string_len(&keyexpr->_suffix),
                 _z_string_data(&keyexpr->_suffix), _z_keyexpr_mapping_id(keyexpr));
        _z_keyexpr_t expanded_ke = __unsafe_z_get_expanded_key_from_key(zn, keyexpr, true);
        _Z_DEBUG("Reply liveliness query for %d - %.*s", expanded_ke._id, (int)_z_string_len(&expanded_ke._suffix),
                 _z_string_data(&expanded_ke._suffix));

        if (!_z_keyexpr_suffix_intersects(&pq->_key, &expanded_ke)) {
            ret = _Z_ERR_QUERY_NOT_MATCH;
        }

        if (ret == _Z_RES_OK) {
            _z_encoding_t encoding = _z_encoding_null();
            _z_bytes_t payload = _z_bytes_null();
            _z_bytes_t attachment = _z_bytes_null();
            _z_source_info_t source_info = _z_source_info_null();
            _z_reply_t reply = _z_reply_steal_data(&expanded_ke, zn->_local_zid, &payload, timestamp, &encoding,
                                                   Z_SAMPLE_KIND_PUT, &attachment, &source_info);

            pq->_callback(&reply, pq->_arg);
            _z_reply_clear(&reply);
        }
    }

    _z_session_mutex_unlock(zn);

    return ret;
}

z_result_t _z_liveliness_pending_query_drop(_z_session_t *zn, uint32_t interest_id) {
    z_result_t ret = _Z_RES_OK;

    _z_session_mutex_lock(zn);

    const _z_liveliness_pending_query_t *pq =
        _z_liveliness_pending_query_intmap_get(&zn->_liveliness_pending_queries, interest_id);
    if (pq == NULL) {
        ret = _Z_ERR_ENTITY_UNKNOWN;
    }

    if (ret == _Z_RES_OK) {
        _Z_DEBUG("Liveliness pending query drop %i", (int)interest_id);
        _z_liveliness_pending_query_intmap_remove(&zn->_liveliness_pending_queries, interest_id);
    }

    _z_session_mutex_unlock(zn);

    return ret;
}

void _z_liveliness_unregister_pending_query(_z_session_t *zn, uint32_t id) {
    _z_session_mutex_lock(zn);

    _z_liveliness_pending_query_intmap_remove(&zn->_liveliness_pending_queries, id);

    _z_session_mutex_unlock(zn);
}

#endif  // Z_FEATURE_QUERY == 1

/**************** Interest processing ****************/

z_result_t _z_liveliness_process_token_declare(_z_session_t *zn, const _z_n_msg_declare_t *decl) {
#if Z_FEATURE_QUERY == 1
    if (decl->has_interest_id) {
        _z_liveliness_pending_query_reply(zn, decl->_interest_id, &decl->_decl._body._decl_token._keyexpr,
                                          &decl->_ext_timestamp);
    }
#endif

#if Z_FEATURE_SUBSCRIPTION == 1
    return _z_liveliness_subscription_declare(zn, decl->_decl._body._decl_token._id,
                                              &decl->_decl._body._decl_token._keyexpr, &decl->_ext_timestamp);
#else
    _ZP_UNUSED(zn);
    _ZP_UNUSED(decl);
    return _Z_RES_OK;
#endif
}

z_result_t _z_liveliness_process_token_undeclare(_z_session_t *zn, const _z_n_msg_declare_t *decl) {
#if Z_FEATURE_SUBSCRIPTION == 1
    return _z_liveliness_subscription_undeclare(zn, decl->_decl._body._undecl_token._id, &decl->_ext_timestamp);
#else
    _ZP_UNUSED(zn);
    _ZP_UNUSED(decl);
    return _Z_RES_OK;
#endif
}

z_result_t _z_liveliness_process_declare_final(_z_session_t *zn, const _z_n_msg_declare_t *decl) {
#if Z_FEATURE_QUERY == 1
    if (decl->has_interest_id) {
        _z_liveliness_pending_query_drop(zn, decl->_interest_id);
    }
    return _Z_RES_OK;
#else
    _ZP_UNUSED(zn);
    _ZP_UNUSED(decl);
    return _Z_RES_OK;
#endif
}

/**************** Init/Clear ****************/

void _z_liveliness_init(_z_session_t *zn) {
    _z_session_mutex_lock(zn);

    zn->_remote_tokens = _z_keyexpr_intmap_make();
    zn->_local_tokens = _z_keyexpr_intmap_make();
#if Z_FEATURE_QUERY == 1
    zn->_liveliness_query_id = 1;
    zn->_liveliness_pending_queries = _z_liveliness_pending_query_intmap_make();
#endif

    _z_session_mutex_unlock(zn);
}

void _z_liveliness_clear(_z_session_t *zn) {
    _z_session_mutex_lock(zn);

#if Z_FEATURE_QUERY == 1
    _z_liveliness_pending_query_intmap_clear(&zn->_liveliness_pending_queries);
#endif
    _z_keyexpr_intmap_clear(&zn->_local_tokens);
    _z_keyexpr_intmap_clear(&zn->_remote_tokens);

    _z_session_mutex_unlock(zn);
}

#else  // Z_FEATURE_LIVELINESS == 0

z_result_t _z_liveliness_process_token_declare(_z_session_t *zn, const _z_n_msg_declare_t *decl) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(decl);
    return _Z_RES_OK;
}

z_result_t _z_liveliness_process_token_undeclare(_z_session_t *zn, const _z_n_msg_declare_t *decl) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(decl);
    return _Z_RES_OK;
}

z_result_t _z_liveliness_process_declare_final(_z_session_t *zn, const _z_n_msg_declare_t *decl) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(decl);
    return _Z_RES_OK;
}

#endif  // Z_FEATURE_LIVELINESS == 1
