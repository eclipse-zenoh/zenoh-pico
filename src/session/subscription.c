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

static inline _z_subscription_cache_data_t _z_subscription_cache_data_null(void) {
    _z_subscription_cache_data_t ret = {0};
    return ret;
}

void _z_subscription_cache_invalidate(_z_session_t *zn) {
#if Z_FEATURE_RX_CACHE == 1
    _z_subscription_lru_cache_clear(&zn->_subscription_cache);
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

void _z_subscription_cache_data_clear(_z_subscription_cache_data_t *val) {
    _z_subscription_infos_svec_clear(&val->infos);
    _z_keyexpr_clear(&val->ke_in);
    _z_keyexpr_clear(&val->ke_out);
}
#endif  // Z_FEATURE_RX_CACHE == 1

bool _z_subscription_eq(const _z_subscription_t *other, const _z_subscription_t *this_) {
    return this_->_id == other->_id;
}

void _z_subscription_clear(_z_subscription_t *sub) {
    if (sub->_dropper != NULL) {
        sub->_dropper(sub->_arg);
    }
    _z_keyexpr_clear(&sub->_key);
}

_z_subscription_rc_t *__z_get_subscription_by_id(_z_subscription_rc_list_t *subs, const _z_zint_t id) {
    _z_subscription_rc_t *ret = NULL;

    _z_subscription_rc_list_t *xs = subs;
    while (xs != NULL) {
        _z_subscription_rc_t *sub = _z_subscription_rc_list_head(xs);
        if (id == _Z_RC_IN_VAL(sub)->_id) {
            ret = sub;
            break;
        }

        xs = _z_subscription_rc_list_tail(xs);
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
    _z_subscription_rc_list_t *subs =
        (kind == _Z_SUBSCRIBER_KIND_SUBSCRIBER) ? zn->_subscriptions : zn->_liveliness_subscriptions;
    return __z_get_subscription_by_id(subs, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
static z_result_t __unsafe_z_get_subscriptions_by_key(_z_session_t *zn, _z_subscriber_kind_t kind,
                                                      const _z_keyexpr_t *key,
                                                      _z_subscription_infos_svec_t *sub_infos) {
    _z_subscription_rc_list_t *subs =
        (kind == _Z_SUBSCRIBER_KIND_SUBSCRIBER) ? zn->_subscriptions : zn->_liveliness_subscriptions;

    *sub_infos = _z_subscription_infos_svec_make(_Z_SUBINFOS_VEC_SIZE);
    _z_subscription_rc_list_t *xs = subs;
    while (xs != NULL) {
        // Parse subscription list
        _z_subscription_rc_t *sub = _z_subscription_rc_list_head(xs);
        if (_z_keyexpr_suffix_intersects(&_Z_RC_IN_VAL(sub)->_key, key)) {
            _z_subscription_infos_t new_sub_info = {.arg = _Z_RC_IN_VAL(sub)->_arg,
                                                    .callback = _Z_RC_IN_VAL(sub)->_callback};
            _Z_RETURN_IF_ERR(_z_subscription_infos_svec_append(sub_infos, &new_sub_info, false));
        }
        xs = _z_subscription_rc_list_tail(xs);
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

    ret = (_z_subscription_rc_t *)z_malloc(sizeof(_z_subscription_rc_t));
    if (ret != NULL) {
        *ret = _z_subscription_rc_new_from_val(s);
        if (kind == _Z_SUBSCRIBER_KIND_SUBSCRIBER) {
            zn->_subscriptions = _z_subscription_rc_list_push(zn->_subscriptions, ret);
        } else {
            zn->_liveliness_subscriptions = _z_subscription_rc_list_push(zn->_liveliness_subscriptions, ret);
        }
    }

    _z_session_mutex_unlock(zn);

    return ret;
}

z_result_t _z_trigger_subscriptions_put(_z_session_t *zn, _z_keyexpr_t *keyexpr, _z_bytes_t *payload,
                                        _z_encoding_t *encoding, const _z_timestamp_t *timestamp, const _z_n_qos_t qos,
                                        _z_bytes_t *attachment, z_reliability_t reliability,
                                        _z_source_info_t *source_info) {
    return _z_trigger_subscriptions_impl(zn, _Z_SUBSCRIBER_KIND_SUBSCRIBER, keyexpr, payload, encoding,
                                         Z_SAMPLE_KIND_PUT, timestamp, qos, attachment, reliability, source_info);
}

z_result_t _z_trigger_subscriptions_del(_z_session_t *zn, _z_keyexpr_t *keyexpr, const _z_timestamp_t *timestamp,
                                        const _z_n_qos_t qos, _z_bytes_t *attachment, z_reliability_t reliability,
                                        _z_source_info_t *source_info) {
    _z_encoding_t encoding = _z_encoding_null();
    _z_bytes_t payload = _z_bytes_null();
    return _z_trigger_subscriptions_impl(zn, _Z_SUBSCRIBER_KIND_SUBSCRIBER, keyexpr, &payload, &encoding,
                                         Z_SAMPLE_KIND_DELETE, timestamp, qos, attachment, reliability, source_info);
}

z_result_t _z_trigger_liveliness_subscriptions_declare(_z_session_t *zn, _z_keyexpr_t *keyexpr,
                                                       const _z_timestamp_t *timestamp) {
    _z_encoding_t encoding = _z_encoding_null();
    _z_bytes_t payload = _z_bytes_null();
    _z_bytes_t attachment = _z_bytes_null();
    _z_keyexpr_t key = _z_keyexpr_alias(keyexpr);
    _z_source_info_t source_info = _z_source_info_null();
    return _z_trigger_subscriptions_impl(zn, _Z_SUBSCRIBER_KIND_LIVELINESS_SUBSCRIBER, &key, &payload, &encoding,
                                         Z_SAMPLE_KIND_PUT, timestamp, _Z_N_QOS_DEFAULT, &attachment,
                                         Z_RELIABILITY_RELIABLE, &source_info);
}

z_result_t _z_trigger_liveliness_subscriptions_undeclare(_z_session_t *zn, _z_keyexpr_t *keyexpr,
                                                         const _z_timestamp_t *timestamp) {
    _z_encoding_t encoding = _z_encoding_null();
    _z_bytes_t payload = _z_bytes_null();
    _z_bytes_t attachment = _z_bytes_null();
    _z_keyexpr_t key = _z_keyexpr_alias(keyexpr);
    _z_source_info_t source_info = _z_source_info_null();
    return _z_trigger_subscriptions_impl(zn, _Z_SUBSCRIBER_KIND_LIVELINESS_SUBSCRIBER, &key, &payload, &encoding,
                                         Z_SAMPLE_KIND_DELETE, timestamp, _Z_N_QOS_DEFAULT, &attachment,
                                         Z_RELIABILITY_RELIABLE, &source_info);
}

static z_result_t _z_subscription_get_infos(_z_session_t *zn, _z_subscriber_kind_t kind,
                                            _z_subscription_cache_data_t *infos) {
    // Check cache
    _z_subscription_cache_data_t *cache_entry = NULL;
#if Z_FEATURE_RX_CACHE == 1
    cache_entry = _z_subscription_lru_cache_get(&zn->_subscription_cache, infos);
#endif
    // Note cache entry
    if (cache_entry != NULL) {
        infos->ke_out = _z_keyexpr_alias(&cache_entry->ke_out);
        infos->infos = _z_subscription_infos_svec_alias(&cache_entry->infos);
        infos->sub_nb = cache_entry->sub_nb;
    } else {  // Construct data and add to cache
        _Z_DEBUG("Resolving %d - %.*s on mapping 0x%x", infos->ke_in._id, (int)_z_string_len(&infos->ke_in._suffix),
                 _z_string_data(&infos->ke_in._suffix), _z_keyexpr_mapping_id(&infos->ke_in));
        _z_session_mutex_lock(zn);
        infos->ke_out = __unsafe_z_get_expanded_key_from_key(zn, &infos->ke_in, true);

        if (!_z_keyexpr_has_suffix(&infos->ke_out)) {
            _z_session_mutex_unlock(zn);
            return _Z_ERR_KEYEXPR_UNKNOWN;
        }
        // Get subscription list
        z_result_t ret = __unsafe_z_get_subscriptions_by_key(zn, kind, &infos->ke_out, &infos->infos);
        _z_session_mutex_unlock(zn);
        if (ret != _Z_RES_OK) {
            return ret;
        }
        infos->sub_nb = _z_subscription_infos_svec_len(&infos->infos);
#if Z_FEATURE_RX_CACHE == 1
        // Update cache, takes ownership of the data
        _z_subscription_cache_data_t cache_storage = {
            .infos = _z_subscription_infos_svec_transfer(&infos->infos),
            .ke_in = _z_keyexpr_duplicate(&infos->ke_in),
            .ke_out = _z_keyexpr_duplicate(&infos->ke_out),
            .sub_nb = infos->sub_nb,
        };
        return _z_subscription_lru_cache_insert(&zn->_subscription_cache, &cache_storage);
#endif
    }
    return _Z_RES_OK;
}

z_result_t _z_trigger_subscriptions_impl(_z_session_t *zn, _z_subscriber_kind_t sub_kind, _z_keyexpr_t *keyexpr,
                                         _z_bytes_t *payload, _z_encoding_t *encoding, const _z_zint_t sample_kind,
                                         const _z_timestamp_t *timestamp, const _z_n_qos_t qos, _z_bytes_t *attachment,
                                         z_reliability_t reliability, _z_source_info_t *source_info) {
    // Retrieve sub infos
    _z_subscription_cache_data_t sub_infos = _z_subscription_cache_data_null();
    sub_infos.ke_in = _z_keyexpr_steal(keyexpr);
    _Z_CLEAN_RETURN_IF_ERR(_z_subscription_get_infos(zn, sub_kind, &sub_infos), _z_keyexpr_clear(&sub_infos.ke_in);
                           _z_encoding_clear(encoding); _z_bytes_drop(payload); _z_bytes_drop(attachment);
                           _z_source_info_clear(source_info));
    // Check if there are subs
    _Z_DEBUG("Triggering %ju subs for key %d - %.*s", (uintmax_t)sub_infos.sub_nb, sub_infos.ke_out._id,
             (int)_z_string_len(&sub_infos.ke_out._suffix), _z_string_data(&sub_infos.ke_out._suffix));
    // Create sample
    z_result_t ret = _Z_RES_OK;
    _z_sample_t sample = _z_sample_steal_data(&sub_infos.ke_out, payload, timestamp, encoding, sample_kind, qos,
                                              attachment, reliability, source_info);
    // Parse subscription infos svec
    if (sub_infos.sub_nb == 1) {
        _z_subscription_infos_t *sub_info = _z_subscription_infos_svec_get(&sub_infos.infos, 0);
        sub_info->callback(&sample, sub_info->arg);
    } else {
        for (size_t i = 0; i < sub_infos.sub_nb; i++) {
            _z_subscription_infos_t *sub_info = _z_subscription_infos_svec_get(&sub_infos.infos, i);
            if (i + 1 == sub_infos.sub_nb) {
                sub_info->callback(&sample, sub_info->arg);
            } else {
                _z_sample_t sample_copy;
                ret = _z_sample_copy(&sample_copy, &sample);
                if (ret != _Z_RES_OK) {
                    break;
                }
                sub_info->callback(&sample_copy, sub_info->arg);
                _z_sample_clear(&sample_copy);
            }
        }
    }
    _z_sample_clear(&sample);
#if Z_FEATURE_RX_CACHE == 0
    _z_subscription_infos_svec_release(&sub_infos.infos);  // Otherwise it's released with cache
#endif
    _z_keyexpr_clear(&sub_infos.ke_in);
    return ret;
}

void _z_unregister_subscription(_z_session_t *zn, _z_subscriber_kind_t kind, _z_subscription_rc_t *sub) {
    _z_session_mutex_lock(zn);

    if (kind == _Z_SUBSCRIBER_KIND_SUBSCRIBER) {
        zn->_subscriptions = _z_subscription_rc_list_drop_filter(zn->_subscriptions, _z_subscription_rc_eq, sub);
    } else {
        zn->_liveliness_subscriptions =
            _z_subscription_rc_list_drop_filter(zn->_liveliness_subscriptions, _z_subscription_rc_eq, sub);
    }

    _z_session_mutex_unlock(zn);
}

void _z_flush_subscriptions(_z_session_t *zn) {
    _z_session_mutex_lock(zn);

    _z_subscription_rc_list_free(&zn->_subscriptions);
    _z_subscription_rc_list_free(&zn->_liveliness_subscriptions);

    _z_session_mutex_unlock(zn);
}
#else  // Z_FEATURE_SUBSCRIPTION == 0

z_result_t _z_trigger_subscriptions_put(_z_session_t *zn, _z_keyexpr_t *keyexpr, _z_bytes_t *payload,
                                        _z_encoding_t *encoding, const _z_timestamp_t *timestamp, const _z_n_qos_t qos,
                                        _z_bytes_t *attachment, z_reliability_t reliability,
                                        _z_source_info_t *source_info) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(keyexpr);
    _ZP_UNUSED(payload);
    _ZP_UNUSED(encoding);
    _ZP_UNUSED(qos);
    _ZP_UNUSED(timestamp);
    _ZP_UNUSED(attachment);
    _ZP_UNUSED(reliability);
    _ZP_UNUSED(source_info);

    return _Z_RES_OK;
}

z_result_t _z_trigger_subscriptions_del(_z_session_t *zn, _z_keyexpr_t *keyexpr, const _z_timestamp_t *timestamp,
                                        const _z_n_qos_t qos, _z_bytes_t *attachment, z_reliability_t reliability,
                                        _z_source_info_t *source_info) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(keyexpr);
    _ZP_UNUSED(qos);
    _ZP_UNUSED(timestamp);
    _ZP_UNUSED(attachment);
    _ZP_UNUSED(reliability);
    _ZP_UNUSED(source_info);

    return _Z_RES_OK;
}

z_result_t _z_trigger_liveliness_subscriptions_declare(_z_session_t *zn, _z_keyexpr_t *keyexpr,
                                                       const _z_timestamp_t *timestamp) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(keyexpr);
    _ZP_UNUSED(timestamp);

    return _Z_RES_OK;
}

z_result_t _z_trigger_liveliness_subscriptions_undeclare(_z_session_t *zn, _z_keyexpr_t *keyexpr,
                                                         const _z_timestamp_t *timestamp) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(keyexpr);
    _ZP_UNUSED(timestamp);

    return _Z_RES_OK;
}

void _z_subscription_cache_invalidate(_z_session_t *zn) { _ZP_UNUSED(zn); }

#endif  // Z_FEATURE_SUBSCRIPTION == 1
