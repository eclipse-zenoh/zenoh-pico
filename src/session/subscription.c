/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/net/resource.h"
#include "zenoh-pico/utils/logging.h"

int _z_subscription_eq(const _z_subscription_t *other, const _z_subscription_t *this)
{
    return this->_id == other->_id;
}

void _z_subscription_clear(_z_subscription_t *sub)
{
    if (sub->_dropper != NULL)
        sub->_dropper(sub->_arg);
    _z_keyexpr_clear(&sub->_key);
}

/*------------------ Pull ------------------*/
_z_zint_t _z_get_pull_id(_z_session_t *zn)
{
    return zn->_pull_id++;
}

_z_subscription_t *__z_get_subscription_by_id(_z_subscriber_list_t *subs, const _z_zint_t id)
{
    _z_subscription_t *sub = NULL;
    while (subs != NULL)
    {
        sub = _z_subscriber_list_head(subs);
        if (id == sub->_id)
            return sub;

        subs = _z_subscriber_list_tail(subs);
    }

    return sub;
}

_z_subscriber_list_t *__z_get_subscriptions_by_key(_z_subscriber_list_t *subs, const _z_keyexpr_t key)
{
    _z_subscriber_list_t *xs = NULL;
    while (subs != NULL)
    {
        _z_subscription_t *sub = _z_subscriber_list_head(subs);
        if (_z_keyexpr_intersect(sub->_key._suffix, strlen(sub->_key._suffix), key._suffix, strlen(key._suffix)))
            xs = _z_subscriber_list_push(xs, sub);

        subs = _z_subscriber_list_tail(subs);
    }

    return xs;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_subscription_t *__unsafe_z_get_subscription_by_id(_z_session_t *zn, int is_local, const _z_zint_t id)
{
    _z_subscriber_list_t *subs = is_local ? zn->_local_subscriptions : zn->_remote_subscriptions;
    return __z_get_subscription_by_id(subs, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_subscriber_list_t *__unsafe_z_get_subscriptions_by_key(_z_session_t *zn, int is_local, const _z_keyexpr_t key)
{
    _z_subscriber_list_t *subs = is_local ? zn->_local_subscriptions : zn->_remote_subscriptions;
    return __z_get_subscriptions_by_key(subs, key);
}

_z_subscription_t *_z_get_subscription_by_id(_z_session_t *zn, int is_local, const _z_zint_t id)
{
    _z_mutex_lock(&zn->_mutex_inner);
    _z_subscription_t *sub = __unsafe_z_get_subscription_by_id(zn, is_local, id);
    _z_mutex_unlock(&zn->_mutex_inner);
    return sub;
}

_z_subscriber_list_t *_z_get_subscriptions_by_key(_z_session_t *zn, int is_local, const _z_keyexpr_t *key)
{
    _z_mutex_lock(&zn->_mutex_inner);
    _z_subscriber_list_t *subs = __unsafe_z_get_subscriptions_by_key(zn, is_local, *key);
    _z_mutex_unlock(&zn->_mutex_inner);
    return subs;
}

int _z_register_subscription(_z_session_t *zn, int is_local, _z_subscription_t *sub)
{
    _Z_DEBUG(">>> Allocating sub decl for (%s)\n", sub->rname);
    _z_mutex_lock(&zn->_mutex_inner);

    _z_subscriber_list_t *subs = __unsafe_z_get_subscriptions_by_key(zn, is_local, sub->_key);
    if (subs != NULL) // A subscription for this name already exists
        goto ERR;

    // Register the subscription
    if (is_local)
        zn->_local_subscriptions = _z_subscriber_list_push(zn->_local_subscriptions, sub);
    else
        zn->_remote_subscriptions = _z_subscriber_list_push(zn->_remote_subscriptions, sub);

    _z_mutex_unlock(&zn->_mutex_inner);
    return 0;

ERR:
    _z_mutex_unlock(&zn->_mutex_inner);
    return -1;
}

int _z_trigger_subscriptions(_z_session_t *zn, const _z_keyexpr_t keyexpr, const _z_bytes_t payload, const _z_encoding_t encoding, const _z_zint_t kind, const _z_timestamp_t timestamp)
{
    _z_mutex_lock(&zn->_mutex_inner);

    _z_keyexpr_t key = __unsafe_z_get_expanded_key_from_key(zn, _Z_RESOURCE_REMOTE, &keyexpr);
    if (key._suffix == NULL)
        goto ERR;

    // Build the sample
    _z_sample_t s;
    s.keyexpr = key;
    s.payload = payload;
    s.encoding = encoding;
    s.kind = kind;
    s.timestamp = timestamp;

    _z_subscriber_list_t *subs = __unsafe_z_get_subscriptions_by_key(zn, _Z_RESOURCE_IS_LOCAL, key);
    _z_subscriber_list_t *xs = subs;
    while (xs != NULL)
    {
        _z_subscription_t *sub = _z_subscriber_list_head(xs);
        sub->_callback(&s, sub->_arg);
        xs = _z_subscriber_list_tail(xs);
    }

    _z_keyexpr_clear(&key);
    _z_list_free(&subs, _z_noop_free);
    _z_mutex_unlock(&zn->_mutex_inner);
    return 0;

ERR:
    _z_mutex_unlock(&zn->_mutex_inner);
    return -1;
}

void _z_unregister_subscription(_z_session_t *zn, int is_local, _z_subscription_t *sub)
{
    _z_mutex_lock(&zn->_mutex_inner);

    if (is_local)
        zn->_local_subscriptions = _z_subscriber_list_drop_filter(zn->_local_subscriptions, _z_subscription_eq, sub);
    else
        zn->_remote_subscriptions = _z_subscriber_list_drop_filter(zn->_remote_subscriptions, _z_subscription_eq, sub);

    _z_mutex_unlock(&zn->_mutex_inner);
}

void _z_flush_subscriptions(_z_session_t *zn)
{
    _z_mutex_lock(&zn->_mutex_inner);

    _z_subscriber_list_free(&zn->_local_subscriptions);
    _z_subscriber_list_free(&zn->_remote_subscriptions);

    _z_mutex_unlock(&zn->_mutex_inner);
}
