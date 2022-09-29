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

#include "zenoh-pico/config.h"
#include "zenoh-pico/net/resource.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/utils/logging.h"

int _z_subscription_eq(const _z_subscription_t *other, const _z_subscription_t *this) {
    return this->_id == other->_id;
}

void _z_subscription_clear(_z_subscription_t *sub) {
    if (sub->_dropper != NULL) {
        sub->_dropper(sub->_arg);
    }
    _z_keyexpr_clear(&sub->_key);
}

/*------------------ Pull ------------------*/
_z_zint_t _z_get_pull_id(_z_session_t *zn) { return zn->_pull_id++; }

_z_subscription_sptr_t *__z_get_subscription_by_id(_z_subscription_sptr_list_t *subs, const _z_zint_t id) {
    _z_subscription_sptr_t *sub = NULL;
    while (subs != NULL) {
        sub = _z_subscription_sptr_list_head(subs);
        if (id == sub->ptr->_id) {
            return sub;
        }

        subs = _z_subscription_sptr_list_tail(subs);
    }

    return sub;
}

_z_subscription_sptr_list_t *__z_get_subscriptions_by_key(_z_subscription_sptr_list_t *subs, const _z_keyexpr_t key) {
    _z_subscription_sptr_list_t *xs = NULL;
    while (subs != NULL) {
        _z_subscription_sptr_t *sub = _z_subscription_sptr_list_head(subs);
        if (_z_keyexpr_intersects(sub->ptr->_key._suffix, strlen(sub->ptr->_key._suffix), key._suffix,
                                  strlen(key._suffix)) == true) {
            xs = _z_subscription_sptr_list_push(xs, _z_subscription_sptr_clone_as_ptr(sub));
        }

        subs = _z_subscription_sptr_list_tail(subs);
    }

    return xs;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_subscription_sptr_t *__unsafe_z_get_subscription_by_id(_z_session_t *zn, int is_local, const _z_zint_t id) {
    _z_subscription_sptr_list_t *subs = is_local ? zn->_local_subscriptions : zn->_remote_subscriptions;
    return __z_get_subscription_by_id(subs, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_subscription_sptr_list_t *__unsafe_z_get_subscriptions_by_key(_z_session_t *zn, int is_local,
                                                                 const _z_keyexpr_t key) {
    _z_subscription_sptr_list_t *subs = is_local ? zn->_local_subscriptions : zn->_remote_subscriptions;
    return __z_get_subscriptions_by_key(subs, key);
}

_z_subscription_sptr_t *_z_get_subscription_by_id(_z_session_t *zn, int is_local, const _z_zint_t id) {
#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    _z_subscription_sptr_t *sub = __unsafe_z_get_subscription_by_id(zn, is_local, id);

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    return sub;
}

_z_subscription_sptr_list_t *_z_get_subscriptions_by_key(_z_session_t *zn, int is_local, const _z_keyexpr_t *key) {
#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    _z_subscription_sptr_list_t *subs = __unsafe_z_get_subscriptions_by_key(zn, is_local, *key);

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    return subs;
}

int _z_register_subscription(_z_session_t *zn, int is_local, _z_subscription_t *s) {
    _Z_DEBUG(">>> Allocating sub decl for (%lu:%s)\n", s->_key._id, s->_key._suffix);

#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    _z_subscription_sptr_list_t *subs = __unsafe_z_get_subscriptions_by_key(zn, is_local, s->_key);
    if (subs != NULL) {  // A subscription for this name already exists
        goto ERR;
    }

    // Register the subscription
    _z_subscription_sptr_t *sub = (_z_subscription_sptr_t *)z_malloc(sizeof(_z_subscription_sptr_t));
    *sub = _z_subscription_sptr_new(*s);
    if (is_local == _Z_RESOURCE_IS_LOCAL) {
        zn->_local_subscriptions = _z_subscription_sptr_list_push(zn->_local_subscriptions, sub);
    } else {
        zn->_remote_subscriptions = _z_subscription_sptr_list_push(zn->_remote_subscriptions, sub);
    }

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    return 0;

ERR:
#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    return -1;
}

int _z_trigger_subscriptions(_z_session_t *zn, const _z_keyexpr_t keyexpr, const _z_bytes_t payload,
                             const _z_encoding_t encoding, const _z_zint_t kind, const _z_timestamp_t timestamp) {
#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    _z_keyexpr_t key = __unsafe_z_get_expanded_key_from_key(zn, _Z_RESOURCE_IS_REMOTE, &keyexpr);
    if (key._suffix == NULL) {
        goto ERR;
    }

    // This two step approach allows the lock to be released while in the user callback
    // Scoped because of the previous goto ERR
    {
        _z_subscription_sptr_list_t *subs = __unsafe_z_get_subscriptions_by_key(zn, _Z_RESOURCE_IS_LOCAL, key);

#if Z_MULTI_THREAD == 1
        _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

        // Build the sample
        _z_sample_t s;
        s.keyexpr = key;
        s.payload = payload;
        s.encoding = encoding;
        s.kind = kind;
        s.timestamp = timestamp;
        _z_subscription_sptr_list_t *xs = subs;
        while (xs != NULL) {
            _z_subscription_sptr_t *sub = _z_subscription_sptr_list_head(xs);
            sub->ptr->_callback(&s, sub->ptr->_arg);
            xs = _z_subscription_sptr_list_tail(xs);
        }

        _z_keyexpr_clear(&key);
        _z_subscription_sptr_list_free(&subs);
    }

    return 0;

ERR:
#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    return -1;
}

void _z_unregister_subscription(_z_session_t *zn, int is_local, _z_subscription_sptr_t *sub) {
#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    if (is_local == _Z_RESOURCE_IS_LOCAL) {
        zn->_local_subscriptions =
            _z_subscription_sptr_list_drop_filter(zn->_local_subscriptions, _z_subscription_sptr_eq, sub);
    } else {
        zn->_remote_subscriptions =
            _z_subscription_sptr_list_drop_filter(zn->_remote_subscriptions, _z_subscription_sptr_eq, sub);
    }

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1
}

void _z_flush_subscriptions(_z_session_t *zn) {
#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    _z_subscription_sptr_list_free(&zn->_local_subscriptions);
    _z_subscription_sptr_list_free(&zn->_remote_subscriptions);

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1
}
