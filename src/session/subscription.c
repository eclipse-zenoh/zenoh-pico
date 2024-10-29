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

// Subscription infos
typedef struct {
    _z_sample_handler_t callback;
    void *arg;
} _z_subscription_infos_t;

static inline void _z_subscription_infos_elem_move(void *dst, void *src) {
    *(_z_subscription_infos_t *)dst = *(_z_subscription_infos_t *)src;
}

typedef _z_svec_t _z_subscription_infos_svec_t;
static inline _z_subscription_infos_svec_t _z_subscription_infos_svec_make(size_t capacity) {
    return _z_svec_make(capacity, sizeof(_z_subscription_infos_t));
}
static inline size_t _z_subscription_infos_svec_len(const _z_subscription_infos_svec_t *v) { return _z_svec_len(v); }

static inline _Bool _z_subscription_infos_svec_append(_z_subscription_infos_svec_t *v,
                                                      const _z_subscription_infos_t *e) {
    return _z_svec_append(v, e, _z_subscription_infos_elem_move, sizeof(_z_subscription_infos_t));
}
static inline _z_subscription_infos_t *_z_subscription_infos_svec_get(const _z_subscription_infos_svec_t *v,
                                                                      size_t pos) {
    return (_z_subscription_infos_t *)_z_svec_get(v, pos, sizeof(_z_subscription_infos_t));
}

static inline void _z_subscription_infos_svec_release(_z_subscription_infos_svec_t *v) { _z_svec_release(v); }

// Keyexpr cache entry for memoization
#define Z_FEATURE_MEMOIZATION 1
#define Z_CONFIG_CACHE_ENTRY_SIZE 4
#define Z_CONFIG_CACHE_BUFFER_SIZE 32

#if Z_FEATURE_MEMOIZATION == 1
typedef struct {
    size_t info_nb;
    _z_keyexpr_t ke_in;
    _z_keyexpr_t ke_out;
    _z_subscription_infos_t infos[Z_CONFIG_CACHE_ENTRY_SIZE];
    uint8_t cache_buffer[Z_CONFIG_CACHE_BUFFER_SIZE];
} _z_subscription_cache_t;

static _z_subscription_cache_t sub_cache = {0};

static inline bool _z_subscription_check_cache(const _z_keyexpr_t *ke) {
    return _z_keyexpr_equals(ke, &sub_cache.ke_in);
}

static inline void _z_subscription_update_cache(const _z_keyexpr_t *ke_in, const _z_keyexpr_t *ke_out,
                                                const _z_subscription_infos_svec_t *infos) {
    size_t vec_len = _z_subscription_infos_svec_len(infos);
    if ((vec_len > Z_CONFIG_CACHE_ENTRY_SIZE) || (_z_string_len(&ke_out->_suffix) > Z_CONFIG_CACHE_BUFFER_SIZE)) {
        return;
    }
    if (_z_keyexpr_has_suffix(ke_in) && !_z_keyexpr_equals(ke_in, ke_out)) {
        return;
    }
    sub_cache.ke_out = _z_keyexpr_alias(*ke_out);
    memcpy(sub_cache.cache_buffer, _z_string_data(&ke_out->_suffix), _z_string_len(&ke_out->_suffix));
    sub_cache.ke_out._suffix._slice.start = sub_cache.cache_buffer;
    sub_cache.info_nb = vec_len;
    for (size_t i = 0; i < vec_len; i++) {
        sub_cache.infos[i] = *_z_subscription_infos_svec_get(infos, i);
    }
    if (!_z_keyexpr_has_suffix(ke_in)) {
        sub_cache.ke_in = _z_keyexpr_alias(*ke_in);
    } else {
        sub_cache.ke_in = _z_keyexpr_alias(sub_cache.ke_out);
    }
}
#else
static inline bool _z_subscription_check_cache(const _z_keyexpr_t *ke) {
    _ZP_UNUSED(ke);
    return false;
}

static inline void _z_subscription_update_cache(const _z_keyexpr_t *ke_in, const _z_keyexpr_t *ke_out,
                                                const _z_subscription_infos_svec_t *infos) {
    _ZP_UNUSED(ke_in);
    _ZP_UNUSED(ke_out);
    _ZP_UNUSED(infos);
    return;
}
#endif  // Z_FEATURE_MEMOIZATION == 1

// Subscription
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
_z_subscription_rc_t *__unsafe_z_get_subscription_by_id(_z_session_t *zn, uint8_t is_local, const _z_zint_t id) {
    _z_subscription_rc_list_t *subs =
        (is_local == _Z_RESOURCE_IS_LOCAL) ? zn->_local_subscriptions : zn->_remote_subscriptions;
    return __z_get_subscription_by_id(subs, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
static z_result_t __unsafe_z_get_subscriptions_by_key(_z_session_t *zn, uint8_t is_local, const _z_keyexpr_t *key,
                                                      _z_subscription_infos_svec_t *sub_infos) {
    _z_subscription_rc_list_t *subs =
        (is_local == _Z_RESOURCE_IS_LOCAL) ? zn->_local_subscriptions : zn->_remote_subscriptions;

    *sub_infos = _z_subscription_infos_svec_make(_Z_SUBINFOS_VEC_SIZE);
    _z_subscription_rc_list_t *xs = subs;
    while (xs != NULL) {
        // Parse subscription list
        _z_subscription_rc_t *sub = _z_subscription_rc_list_head(xs);
        if (_z_keyexpr_suffix_intersects(&_Z_RC_IN_VAL(sub)->_key, key)) {
            _z_subscription_infos_t new_sub_info = {.arg = _Z_RC_IN_VAL(sub)->_arg,
                                                    .callback = _Z_RC_IN_VAL(sub)->_callback};
            if (!_z_subscription_infos_svec_append(sub_infos, &new_sub_info)) {
                _Z_ERROR("Failed to allocate subscription info vector");
                return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
            }
        }
        xs = _z_subscription_rc_list_tail(xs);
    }
    return _Z_RES_OK;
}

_z_subscription_rc_t *_z_get_subscription_by_id(_z_session_t *zn, uint8_t is_local, const _z_zint_t id) {
    _z_session_mutex_lock(zn);

    _z_subscription_rc_t *sub = __unsafe_z_get_subscription_by_id(zn, is_local, id);

    _z_session_mutex_unlock(zn);

    return sub;
}

_z_subscription_rc_t *_z_register_subscription(_z_session_t *zn, uint8_t is_local, _z_subscription_t *s) {
    _Z_DEBUG(">>> Allocating sub decl for (%ju:%.*s)", (uintmax_t)s->_key._id, (int)_z_string_len(&s->_key._suffix),
             _z_string_data(&s->_key._suffix));
    _z_subscription_rc_t *ret = NULL;

    _z_session_mutex_lock(zn);

    ret = (_z_subscription_rc_t *)z_malloc(sizeof(_z_subscription_rc_t));
    if (ret != NULL) {
        *ret = _z_subscription_rc_new_from_val(s);
        if (is_local == _Z_RESOURCE_IS_LOCAL) {
            zn->_local_subscriptions = _z_subscription_rc_list_push(zn->_local_subscriptions, ret);
        } else {
            zn->_remote_subscriptions = _z_subscription_rc_list_push(zn->_remote_subscriptions, ret);
        }
    }

    _z_session_mutex_unlock(zn);

    return ret;
}

void _z_trigger_local_subscriptions(_z_session_t *zn, const _z_keyexpr_t *keyexpr, const _z_bytes_t *payload,
                                    _z_encoding_t *encoding, const _z_n_qos_t qos, const _z_timestamp_t *timestamp,
                                    const _z_bytes_t *attachment, z_reliability_t reliability) {
    z_result_t ret = _z_trigger_subscriptions(zn, keyexpr, payload, encoding, Z_SAMPLE_KIND_PUT, timestamp, qos,
                                              attachment, reliability);
    (void)ret;
}

z_result_t _z_trigger_subscriptions(_z_session_t *zn, const _z_keyexpr_t *keyexpr, const _z_bytes_t *payload,
                                    _z_encoding_t *encoding, const _z_zint_t kind, const _z_timestamp_t *timestamp,
                                    const _z_n_qos_t qos, const _z_bytes_t *attachment, z_reliability_t reliability) {
    _z_sample_t sample;
    _z_keyexpr_t key;
    z_result_t ret = _Z_RES_OK;
    _Z_DEBUG("Resolving %d - %.*s on mapping 0x%x", keyexpr->_id, (int)_z_string_len(&keyexpr->_suffix),
             _z_string_data(&keyexpr->_suffix), _z_keyexpr_mapping_id(keyexpr));

    // Check cache
    if (_z_subscription_check_cache(keyexpr)) {
        // Key is cached but no subs
        if (sub_cache.info_nb == 0) {
            return _Z_RES_OK;
        }
        _Z_DEBUG("Triggering %ju subs", (uintmax_t)sub_cache.info_nb);
        // Build the sample
        key = _z_keyexpr_alias(sub_cache.ke_out);
        ret = _z_sample_create(&sample, &key, payload, timestamp, encoding, kind, qos, attachment, reliability);
        if (ret == _Z_RES_OK) {
            // Parse subscription infos
            for (size_t i = 0; i < sub_cache.info_nb; i++) {
                _z_subscription_infos_t *sub_info = &sub_cache.infos[i];
                sub_info->callback(&sample, sub_info->arg);
            }
        }
        // Clean up
        _z_sample_clear(&sample);
        return ret;
    }

    _z_session_mutex_lock(zn);
    key = __unsafe_z_get_expanded_key_from_key(zn, keyexpr, true);
    _Z_DEBUG("Triggering subs for %d - %.*s", key._id, (int)_z_string_len(&key._suffix), _z_string_data(&key._suffix));
    if (_z_keyexpr_has_suffix(&key)) {
        // Get subscription list
        _z_subscription_infos_svec_t subs;
        ret = __unsafe_z_get_subscriptions_by_key(zn, _Z_RESOURCE_IS_LOCAL, &key, &subs);
        _z_session_mutex_unlock(zn);
        if (ret != _Z_RES_OK) {
            return ret;
        }
        // Check if there is subs
        size_t sub_nb = _z_subscription_infos_svec_len(&subs);
        // Update cache
        _z_subscription_update_cache(keyexpr, &key, &subs);
        if (sub_nb == 0) {
            return _Z_RES_OK;
        }
        _Z_DEBUG("Triggering %ju subs", (uintmax_t)sub_nb);
        // Build the sample
        ret = _z_sample_create(&sample, &key, payload, timestamp, encoding, kind, qos, attachment, reliability);
        if (ret == _Z_RES_OK) {
            // Parse subscription infos svec
            for (size_t i = 0; i < sub_nb; i++) {
                _z_subscription_infos_t *sub_info = _z_subscription_infos_svec_get(&subs, i);
                sub_info->callback(&sample, sub_info->arg);
            }
        }
        // Clean up
        _z_sample_clear(&sample);
        _z_subscription_infos_svec_release(&subs);
    } else {
        _z_session_mutex_unlock(zn);
        ret = _Z_ERR_KEYEXPR_UNKNOWN;
    }

    return ret;
}

void _z_unregister_subscription(_z_session_t *zn, uint8_t is_local, _z_subscription_rc_t *sub) {
    _z_session_mutex_lock(zn);

    if (is_local == _Z_RESOURCE_IS_LOCAL) {
        zn->_local_subscriptions =
            _z_subscription_rc_list_drop_filter(zn->_local_subscriptions, _z_subscription_rc_eq, sub);
    } else {
        zn->_remote_subscriptions =
            _z_subscription_rc_list_drop_filter(zn->_remote_subscriptions, _z_subscription_rc_eq, sub);
    }

    _z_session_mutex_unlock(zn);
}

void _z_flush_subscriptions(_z_session_t *zn) {
    _z_session_mutex_lock(zn);

    _z_subscription_rc_list_free(&zn->_local_subscriptions);
    _z_subscription_rc_list_free(&zn->_remote_subscriptions);

    _z_session_mutex_unlock(zn);
}
#else  // Z_FEATURE_SUBSCRIPTION == 0

void _z_trigger_local_subscriptions(_z_session_t *zn, const _z_keyexpr_t *keyexpr, const _z_bytes_t *payload,
                                    _z_encoding_t *encoding, const _z_n_qos_t qos, const _z_timestamp_t *timestamp,
                                    const _z_bytes_t *attachment, z_reliability_t reliability) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(keyexpr);
    _ZP_UNUSED(payload);
    _ZP_UNUSED(encoding);
    _ZP_UNUSED(qos);
    _ZP_UNUSED(timestamp);
    _ZP_UNUSED(attachment);
    _ZP_UNUSED(reliability);
}

#endif  // Z_FEATURE_SUBSCRIPTION == 1
