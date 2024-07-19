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
_Bool _z_subscription_eq(const _z_subscription_t *other, const _z_subscription_t *this_) {
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

_z_subscription_rc_list_t *__z_get_subscriptions_by_key(_z_subscription_rc_list_t *subs, const _z_keyexpr_t key) {
    _z_subscription_rc_list_t *ret = NULL;

    _z_subscription_rc_list_t *xs = subs;
    while (xs != NULL) {
        _z_subscription_rc_t *sub = _z_subscription_rc_list_head(xs);
        if (_z_keyexpr_intersects(_Z_RC_IN_VAL(sub)->_key._suffix, strlen(_Z_RC_IN_VAL(sub)->_key._suffix), key._suffix,
                                  strlen(key._suffix)) == true) {
            ret = _z_subscription_rc_list_push(ret, _z_subscription_rc_clone_as_ptr(sub));
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
_z_subscription_rc_list_t *__unsafe_z_get_subscriptions_by_key(_z_session_t *zn, uint8_t is_local,
                                                               const _z_keyexpr_t key) {
    _z_subscription_rc_list_t *subs =
        (is_local == _Z_RESOURCE_IS_LOCAL) ? zn->_local_subscriptions : zn->_remote_subscriptions;
    return __z_get_subscriptions_by_key(subs, key);
}

_z_subscription_rc_t *_z_get_subscription_by_id(_z_session_t *zn, uint8_t is_local, const _z_zint_t id) {
    _zp_session_lock_mutex(zn);

    _z_subscription_rc_t *sub = __unsafe_z_get_subscription_by_id(zn, is_local, id);

    _zp_session_unlock_mutex(zn);

    return sub;
}

_z_subscription_rc_list_t *_z_get_subscriptions_by_key(_z_session_t *zn, uint8_t is_local, const _z_keyexpr_t *key) {
    _zp_session_lock_mutex(zn);

    _z_subscription_rc_list_t *subs = __unsafe_z_get_subscriptions_by_key(zn, is_local, *key);

    _zp_session_unlock_mutex(zn);

    return subs;
}

_z_subscription_rc_t *_z_register_subscription(_z_session_t *zn, uint8_t is_local, _z_subscription_t *s) {
    _Z_DEBUG(">>> Allocating sub decl for (%ju:%s)", (uintmax_t)s->_key._id, s->_key._suffix);
    _z_subscription_rc_t *ret = NULL;

    _zp_session_lock_mutex(zn);

    ret = (_z_subscription_rc_t *)z_malloc(sizeof(_z_subscription_rc_t));
    if (ret != NULL) {
        *ret = _z_subscription_rc_new_from_val(s);
        if (is_local == _Z_RESOURCE_IS_LOCAL) {
            zn->_local_subscriptions = _z_subscription_rc_list_push(zn->_local_subscriptions, ret);
        } else {
            zn->_remote_subscriptions = _z_subscription_rc_list_push(zn->_remote_subscriptions, ret);
        }
    }

    _zp_session_unlock_mutex(zn);

    return ret;
}

void _z_trigger_local_subscriptions(_z_session_t *zn, const _z_keyexpr_t keyexpr, const _z_bytes_t payload,
                                    const _z_n_qos_t qos, const _z_bytes_t attachment) {
    _z_encoding_t encoding = _z_encoding_null();
    _z_timestamp_t timestamp = _z_timestamp_null();
    int8_t ret =
        _z_trigger_subscriptions(zn, keyexpr, payload, &encoding, Z_SAMPLE_KIND_PUT, &timestamp, qos, attachment);
    (void)ret;
}

int8_t _z_trigger_subscriptions(_z_session_t *zn, const _z_keyexpr_t keyexpr, const _z_bytes_t payload,
                                _z_encoding_t *encoding, const _z_zint_t kind, const _z_timestamp_t *timestamp,
                                const _z_n_qos_t qos, const _z_bytes_t attachment) {
    int8_t ret = _Z_RES_OK;

    _zp_session_lock_mutex(zn);

    _Z_DEBUG("Resolving %d - %s on mapping 0x%x", keyexpr._id, keyexpr._suffix, _z_keyexpr_mapping_id(&keyexpr));
    _z_keyexpr_t key = __unsafe_z_get_expanded_key_from_key(zn, &keyexpr);
    _Z_DEBUG("Triggering subs for %d - %s", key._id, key._suffix);
    if (key._suffix != NULL) {
        _z_subscription_rc_list_t *subs = __unsafe_z_get_subscriptions_by_key(zn, _Z_RESOURCE_IS_LOCAL, key);

        _zp_session_unlock_mutex(zn);

        // Build the sample
        _z_sample_t sample = _z_sample_create(&key, payload, timestamp, encoding, kind, qos, attachment);
        // Parse subscription list
        _z_subscription_rc_list_t *xs = subs;
        _Z_DEBUG("Triggering %ju subs", (uintmax_t)_z_subscription_rc_list_len(xs));
        while (xs != NULL) {
            _z_subscription_rc_t *sub = _z_subscription_rc_list_head(xs);
            _Z_RC_IN_VAL(sub)->_callback(&sample, _Z_RC_IN_VAL(sub)->_arg);
            xs = _z_subscription_rc_list_tail(xs);
        }
        // Clean up
        _z_sample_clear(&sample);
        _z_subscription_rc_list_free(&subs);
    } else {
        _zp_session_unlock_mutex(zn);
        ret = _Z_ERR_KEYEXPR_UNKNOWN;
    }

    return ret;
}

void _z_unregister_subscription(_z_session_t *zn, uint8_t is_local, _z_subscription_rc_t *sub) {
    _zp_session_lock_mutex(zn);

    if (is_local == _Z_RESOURCE_IS_LOCAL) {
        zn->_local_subscriptions =
            _z_subscription_rc_list_drop_filter(zn->_local_subscriptions, _z_subscription_rc_eq, sub);
    } else {
        zn->_remote_subscriptions =
            _z_subscription_rc_list_drop_filter(zn->_remote_subscriptions, _z_subscription_rc_eq, sub);
    }

    _zp_session_unlock_mutex(zn);
}

void _z_flush_subscriptions(_z_session_t *zn) {
    _zp_session_lock_mutex(zn);

    _z_subscription_rc_list_free(&zn->_local_subscriptions);
    _z_subscription_rc_list_free(&zn->_remote_subscriptions);

    _zp_session_unlock_mutex(zn);
}
#else  // Z_FEATURE_SUBSCRIPTION == 0

void _z_trigger_local_subscriptions(_z_session_t *zn, const _z_keyexpr_t keyexpr, const _z_bytes_t payload,
                                    _z_n_qos_t qos, const _z_bytes_t attachment) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(keyexpr);
    _ZP_UNUSED(payload);
    _ZP_UNUSED(qos);
    _ZP_UNUSED(attachment);
}

#endif  // Z_FEATURE_SUBSCRIPTION == 1
