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

#include "zenoh-pico/session/subscription.h"

#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/net/sample.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_SUBSCRIPTION == 1

#define _Z_SUBINFOS_VEC_SIZE 4  // Arbitrary initial size

void _z_subscription_cache_invalidate(_z_session_t *zn) {
#if Z_FEATURE_RX_CACHE == 1
    _z_session_mutex_lock(zn);
    _z_subscription_lru_cache_clear(&zn->_subscription_cache);
    _z_session_mutex_unlock(zn);
#else
    _ZP_UNUSED(zn);
#endif
}

#if Z_FEATURE_RX_CACHE == 1
int _z_subscription_cache_data_compare(const void *first, const void *second) {
    _z_subscription_cache_data_t *first_data = (_z_subscription_cache_data_t *)first;
    _z_subscription_cache_data_t *second_data = (_z_subscription_cache_data_t *)second;
    return _z_keyexpr_compare(&first_data->ke_in, &second_data->ke_in);
}
#endif  // Z_FEATURE_RX_CACHE == 1

void _z_subscription_cache_data_clear(_z_subscription_cache_data_t *val) {
    _z_subscription_rc_svec_rc_drop(&val->infos);
    _z_keyexpr_clear(&val->ke_in);
    _z_keyexpr_clear(&val->ke_out);
}

bool _z_subscription_eq(const _z_subscription_t *other, const _z_subscription_t *this_) {
    return this_->_id == other->_id;
}

void _z_subscription_clear(_z_subscription_t *sub) {
    if (sub->_dropper != NULL) {
        sub->_dropper(sub->_arg);
    }
    _z_keyexpr_clear(&sub->_key);
    _z_keyexpr_clear(&sub->_declared_key);
}

_z_subscription_rc_t *__z_get_subscription_by_id(_z_subscription_rc_slist_t *subs, const _z_zint_t id) {
    _z_subscription_rc_t *ret = NULL;

    _z_subscription_rc_slist_t *xs = subs;
    while (xs != NULL) {
        _z_subscription_rc_t *sub = _z_subscription_rc_slist_value(xs);
        if (id == _Z_RC_IN_VAL(sub)->_id) {
            ret = sub;
            break;
        }
        xs = _z_subscription_rc_slist_next(xs);
    }

    return ret;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_subscription_rc_t *__unsafe_z_get_subscription_by_id(_z_session_t *zn, _z_subscriber_kind_t kind,
                                                        const _z_zint_t id) {
    _z_subscription_rc_slist_t *subs =
        (kind == _Z_SUBSCRIBER_KIND_SUBSCRIBER) ? zn->_subscriptions : zn->_liveliness_subscriptions;
    return __z_get_subscription_by_id(subs, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
static z_result_t __unsafe_z_get_subscriptions_by_key(_z_session_t *zn, _z_subscriber_kind_t kind,
                                                      const _z_keyexpr_t *key, _z_subscription_rc_svec_t *sub_infos) {
    _z_subscription_rc_slist_t *subs =
        (kind == _Z_SUBSCRIBER_KIND_SUBSCRIBER) ? zn->_subscriptions : zn->_liveliness_subscriptions;

    *sub_infos = _z_subscription_rc_svec_make(_Z_SUBINFOS_VEC_SIZE);
    _Z_RETURN_ERR_OOM_IF_TRUE(sub_infos->_val == NULL);
    _z_subscription_rc_slist_t *xs = subs;
    while (xs != NULL) {
        // Parse subscription list
        _z_subscription_rc_t *sub = _z_subscription_rc_slist_value(xs);
        if (_z_keyexpr_suffix_intersects(&_Z_RC_IN_VAL(sub)->_key, key)) {
            _z_subscription_rc_t sub_clone = _z_subscription_rc_clone(sub);
            _Z_CLEAN_RETURN_IF_ERR(_z_subscription_rc_svec_append(sub_infos, &sub_clone, true),
                                   _z_subscription_rc_svec_clear(sub_infos));
        }
        xs = _z_subscription_rc_slist_next(xs);
    }
    return _Z_RES_OK;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
static z_result_t __unsafe_z_get_subscriptions_rc_by_key(_z_session_t *zn, _z_subscriber_kind_t kind,
                                                         const _z_keyexpr_t *key,
                                                         _z_subscription_rc_svec_rc_t *sub_infos) {
    *sub_infos = _z_subscription_rc_svec_rc_new_undefined();
    z_result_t ret = !_Z_RC_IS_NULL(sub_infos) ? _Z_RES_OK : _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    _Z_SET_IF_OK(ret, __unsafe_z_get_subscriptions_by_key(zn, kind, key, _Z_RC_IN_VAL(sub_infos)));
    if (ret != _Z_RES_OK) {
        _z_subscription_rc_svec_rc_drop(sub_infos);
    }
    return _Z_RES_OK;
}

_z_subscription_rc_t *_z_get_subscription_by_id(_z_session_t *zn, _z_subscriber_kind_t kind, const _z_zint_t id) {
    _z_session_mutex_lock(zn);

    _z_subscription_rc_t *sub = __unsafe_z_get_subscription_by_id(zn, kind, id);

    _z_session_mutex_unlock(zn);

    return sub;
}

_z_subscription_rc_t *_z_register_subscription(_z_session_t *zn, _z_subscriber_kind_t kind, _z_subscription_t *s) {
    _Z_DEBUG(">>> Allocating sub decl for (%ju:%.*s)", (uintmax_t)s->_key._id, (int)_z_string_len(&s->_key._suffix),
             _z_string_data(&s->_key._suffix));

    _z_subscription_rc_t *ret = NULL;
    _z_session_mutex_lock(zn);
    if (kind == _Z_SUBSCRIBER_KIND_SUBSCRIBER) {
        zn->_subscriptions = _z_subscription_rc_slist_push_empty(zn->_subscriptions);
        ret = _z_subscription_rc_slist_value(zn->_subscriptions);
    } else {
        zn->_liveliness_subscriptions = _z_subscription_rc_slist_push_empty(zn->_liveliness_subscriptions);
        ret = _z_subscription_rc_slist_value(zn->_liveliness_subscriptions);
    }
    *ret = _z_subscription_rc_new_from_val(s);
    _z_session_mutex_unlock(zn);

    return ret;
}

z_result_t _z_trigger_liveliness_subscriptions_declare(_z_session_t *zn, const _z_keyexpr_t *keyexpr,
                                                       const _z_timestamp_t *timestamp,
                                                       _z_transport_peer_common_t *peer) {
    _z_encoding_t encoding = _z_encoding_null();
    _z_bytes_t payload = _z_bytes_null();
    _z_bytes_t attachment = _z_bytes_null();
    _z_keyexpr_t key = _z_keyexpr_alias(keyexpr);
    _z_source_info_t source_info = _z_source_info_null();
    return _z_trigger_subscriptions_impl(zn, _Z_SUBSCRIBER_KIND_LIVELINESS_SUBSCRIBER, &key, &payload, &encoding,
                                         Z_SAMPLE_KIND_PUT, timestamp, _Z_N_QOS_DEFAULT, &attachment,
                                         Z_RELIABILITY_RELIABLE, &source_info, peer);
}

z_result_t _z_trigger_liveliness_subscriptions_undeclare(_z_session_t *zn, const _z_keyexpr_t *keyexpr,
                                                         const _z_timestamp_t *timestamp,
                                                         _z_transport_peer_common_t *peer) {
    _z_encoding_t encoding = _z_encoding_null();
    _z_bytes_t payload = _z_bytes_null();
    _z_bytes_t attachment = _z_bytes_null();
    _z_keyexpr_t key = _z_keyexpr_alias(keyexpr);
    _z_source_info_t source_info = _z_source_info_null();
    return _z_trigger_subscriptions_impl(zn, _Z_SUBSCRIBER_KIND_LIVELINESS_SUBSCRIBER, &key, &payload, &encoding,
                                         Z_SAMPLE_KIND_DELETE, timestamp, _Z_N_QOS_DEFAULT, &attachment,
                                         Z_RELIABILITY_RELIABLE, &source_info, peer);
}

static z_result_t _z_subscription_get_infos(_z_session_t *zn, _z_subscriber_kind_t kind,
                                            _z_subscription_cache_data_t *infos, _z_transport_peer_common_t *peer) {
    _z_session_mutex_lock(zn);
    _z_subscription_cache_data_t *cache_entry = NULL;
    z_result_t ret = _Z_RES_OK;
#if Z_FEATURE_RX_CACHE == 1
    cache_entry = _z_subscription_lru_cache_get(&zn->_subscription_cache, infos);
#endif
    if (cache_entry != NULL) {  // Copy cache entry
        infos->infos = _z_subscription_rc_svec_rc_clone(&cache_entry->infos);
        ret = _z_keyexpr_copy(&infos->ke_out, &cache_entry->ke_out);
    } else {  // Construct data and add to cache
        _Z_DEBUG("Resolving %d - %.*s on mapping 0x%x", infos->ke_in._id, (int)_z_string_len(&infos->ke_in._suffix),
                 _z_string_data(&infos->ke_in._suffix), (unsigned int)infos->ke_in._mapping);
        infos->ke_out = __unsafe_z_get_expanded_key_from_key(zn, &infos->ke_in, false, peer);
        ret = _z_keyexpr_has_suffix(&infos->ke_out) ? _Z_RES_OK : _Z_ERR_KEYEXPR_UNKNOWN;
        // Get subscription list
        _Z_SET_IF_OK(ret, __unsafe_z_get_subscriptions_rc_by_key(zn, kind, &infos->ke_out, &infos->infos));
#if Z_FEATURE_RX_CACHE == 1
        _z_subscription_cache_data_t cache_storage = _z_subscription_cache_data_null();
        cache_storage.infos = _z_subscription_rc_svec_rc_clone(&infos->infos);
        _Z_SET_IF_OK(ret, _z_keyexpr_copy(&cache_storage.ke_out, &infos->ke_out));
        _Z_SET_IF_OK(ret, _z_keyexpr_copy(&cache_storage.ke_in, &infos->ke_in));
        _Z_SET_IF_OK(ret, _z_subscription_lru_cache_insert(&zn->_subscription_cache, &cache_storage));
        if (ret != _Z_RES_OK) {
            _z_subscription_cache_data_clear(&cache_storage);
        }
#endif
    }
    if (ret != _Z_RES_OK) {
        _z_subscription_cache_data_clear(infos);
    }
    _z_session_mutex_unlock(zn);
    return ret;
}

z_result_t _z_trigger_subscriptions_impl(_z_session_t *zn, _z_subscriber_kind_t sub_kind, _z_keyexpr_t *keyexpr,
                                         _z_bytes_t *payload, _z_encoding_t *encoding, const _z_zint_t sample_kind,
                                         const _z_timestamp_t *timestamp, const _z_n_qos_t qos, _z_bytes_t *attachment,
                                         z_reliability_t reliability, _z_source_info_t *source_info,
                                         _z_transport_peer_common_t *peer) {
    // Retrieve sub infos
    _z_subscription_cache_data_t sub_infos = _z_subscription_cache_data_null();
    sub_infos.ke_in = _z_keyexpr_steal(keyexpr);
    _Z_CLEAN_RETURN_IF_ERR(_z_subscription_get_infos(zn, sub_kind, &sub_infos, peer), _z_encoding_clear(encoding);
                           _z_bytes_drop(payload); _z_bytes_drop(attachment); _z_source_info_clear(source_info););
    const _z_subscription_rc_svec_t *subs = _Z_RC_IN_VAL(&sub_infos.infos);
    size_t sub_nb = _z_subscription_rc_svec_len(subs);
    _Z_DEBUG("Triggering %ju subs for key %d - %.*s", (uintmax_t)sub_nb, sub_infos.ke_out._id,
             (int)_z_string_len(&sub_infos.ke_out._suffix), _z_string_data(&sub_infos.ke_out._suffix));
    // Create sample
    z_result_t ret = _Z_RES_OK;
    _z_sample_t sample;
    _z_sample_steal_data(&sample, &sub_infos.ke_out, payload, timestamp, encoding, sample_kind, qos, attachment,
                         reliability, source_info);
    // Parse subscription infos svec
    if (sub_nb == 1) {
        _z_subscription_t *sub_info = _Z_RC_IN_VAL(_z_subscription_rc_svec_get(subs, 0));
        sub_info->_callback(&sample, sub_info->_arg);
    } else {
        for (size_t i = 0; i < sub_nb; i++) {
            _z_subscription_t *sub_info = _Z_RC_IN_VAL(_z_subscription_rc_svec_get(subs, i));
            if (i + 1 == sub_nb) {
                sub_info->_callback(&sample, sub_info->_arg);
            } else {
                _z_sample_t sample_copy;
                ret = _z_sample_copy(&sample_copy, &sample);
                if (ret != _Z_RES_OK) {
                    break;
                }
                sub_info->_callback(&sample_copy, sub_info->_arg);
                _z_sample_clear(&sample_copy);
            }
        }
    }
    _z_sample_clear(&sample);
    _z_subscription_cache_data_clear(&sub_infos);
    return ret;
}

void _z_unregister_subscription(_z_session_t *zn, _z_subscriber_kind_t kind, _z_subscription_rc_t *sub) {
    _z_session_mutex_lock(zn);

    if (kind == _Z_SUBSCRIBER_KIND_SUBSCRIBER) {
        zn->_subscriptions = _z_subscription_rc_slist_drop_first_filter(zn->_subscriptions, _z_subscription_rc_eq, sub);
    } else {
        zn->_liveliness_subscriptions =
            _z_subscription_rc_slist_drop_first_filter(zn->_liveliness_subscriptions, _z_subscription_rc_eq, sub);
    }

    _z_session_mutex_unlock(zn);
}

void _z_flush_subscriptions(_z_session_t *zn) {
    _z_session_mutex_lock(zn);

    _z_subscription_rc_slist_free(&zn->_subscriptions);
    _z_subscription_rc_slist_free(&zn->_liveliness_subscriptions);

    _z_session_mutex_unlock(zn);
}
#else  // Z_FEATURE_SUBSCRIPTION == 0
z_result_t _z_trigger_liveliness_subscriptions_declare(_z_session_t *zn, const _z_keyexpr_t *keyexpr,
                                                       const _z_timestamp_t *timestamp,
                                                       _z_transport_peer_common_t *peer) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(keyexpr);
    _ZP_UNUSED(timestamp);
    _ZP_UNUSED(peer);
    return _Z_RES_OK;
}

z_result_t _z_trigger_liveliness_subscriptions_undeclare(_z_session_t *zn, const _z_keyexpr_t *keyexpr,
                                                         const _z_timestamp_t *timestamp,
                                                         _z_transport_peer_common_t *peer) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(keyexpr);
    _ZP_UNUSED(timestamp);
    _ZP_UNUSED(peer);
    return _Z_RES_OK;
}

void _z_subscription_cache_invalidate(_z_session_t *zn) { _ZP_UNUSED(zn); }

#endif  // Z_FEATURE_SUBSCRIPTION == 1
