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

#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/net/resource.h"
#include "zenoh-pico/utils/logging.h"

int _z_subscriber_eq(const _z_subscriber_t *other, const _z_subscriber_t *this)
{
    return this->id == other->id;
}

void _z_subscriber_clear(_z_subscriber_t *sub)
{
    _z_str_clear(sub->rname);
    _z_reskey_clear(&sub->key);
}

/*------------------ Pull ------------------*/
_z_zint_t _z_get_pull_id(_z_session_t *zn)
{
    return zn->pull_id++;
}

_z_subscriber_t *__z_get_subscription_by_id(_z_subscriber_list_t *subs, const _z_zint_t id)
{
    _z_subscriber_t *sub = NULL;
    while (subs != NULL)
    {
        sub = _z_subscriber_list_head(subs);
        if (id == sub->id)
            return sub;

        subs = _z_subscriber_list_tail(subs);
    }

    return sub;
}

_z_subscriber_list_t *__z_get_subscriptions_by_name(_z_subscriber_list_t *subs, const _z_str_t rname)
{
    _z_subscriber_list_t *xs = NULL;
    while (subs != NULL)
    {
        _z_subscriber_t *sub = _z_subscriber_list_head(subs);
        if (_z_rname_intersect(sub->rname, rname))
            xs = _z_subscriber_list_push(xs, sub);

        subs = _z_subscriber_list_tail(subs);
    }

    return xs;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_z_subscriber_t *__unsafe_z_get_subscription_by_id(_z_session_t *zn, int is_local, const _z_zint_t id)
{
    _z_subscriber_list_t *subs = is_local ? zn->local_subscriptions : zn->remote_subscriptions;
    return __z_get_subscription_by_id(subs, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_z_subscriber_list_t *__unsafe_z_get_subscriptions_by_name(_z_session_t *zn, int is_local, const _z_str_t rname)
{
    _z_subscriber_list_t *subs = is_local ? zn->local_subscriptions : zn->remote_subscriptions;
    return __z_get_subscriptions_by_name(subs, rname);
}

_z_subscriber_t *_z_get_subscription_by_id(_z_session_t *zn, int is_local, const _z_zint_t id)
{
    _z_mutex_lock(&zn->mutex_inner);
    _z_subscriber_t *sub = __unsafe_z_get_subscription_by_id(zn, is_local, id);
    _z_mutex_unlock(&zn->mutex_inner);
    return sub;
}

_z_subscriber_list_t *_z_get_subscriptions_by_name(_z_session_t *zn, int is_local, const _z_str_t rname)
{
    _z_mutex_lock(&zn->mutex_inner);
    _z_subscriber_list_t *subs = __unsafe_z_get_subscriptions_by_name(zn, is_local, rname);
    _z_mutex_unlock(&zn->mutex_inner);
    return subs;
}

_z_subscriber_list_t *_z_get_subscription_by_key(_z_session_t *zn, int is_local, const _z_reskey_t *reskey)
{
    _z_mutex_lock(&zn->mutex_inner);
    _z_str_t rname = __unsafe_z_get_resource_name_from_key(zn, is_local, reskey);
    _z_subscriber_list_t *subs = __unsafe_z_get_subscriptions_by_name(zn, is_local, rname);
    _z_mutex_unlock(&zn->mutex_inner);

    _z_str_clear(rname);
    return subs;
}

int _z_register_subscription(_z_session_t *zn, int is_local, _z_subscriber_t *sub)
{
    _Z_DEBUG(">>> Allocating sub decl for (%s)\n", sub->rname);
    _z_mutex_lock(&zn->mutex_inner);

    _z_subscriber_list_t *subs = __unsafe_z_get_subscriptions_by_name(zn, is_local, sub->rname);
    if (subs != NULL) // A subscription for this name already exists
        goto ERR;

    // Register the subscription
    if (is_local)
        zn->local_subscriptions = _z_subscriber_list_push(zn->local_subscriptions, sub);
    else
        zn->remote_subscriptions = _z_subscriber_list_push(zn->remote_subscriptions, sub);

    _z_mutex_unlock(&zn->mutex_inner);
    return 0;

ERR:
    _z_mutex_unlock(&zn->mutex_inner);
    return -1;
}

int _z_trigger_subscriptions(_z_session_t *zn, const _z_reskey_t reskey, const _z_bytes_t payload, const _z_encoding_t encoding)
{
    _z_mutex_lock(&zn->mutex_inner);

    _z_str_t rname = __unsafe_z_get_resource_name_from_key(zn, _Z_RESOURCE_REMOTE, &reskey);
    if (rname == NULL)
        goto ERR;
    _z_reskey_t key = _z_rname(rname);

    // Build the sample
    _z_sample_t s;
    s.key = key;
    s.value = payload;
    s.encoding.prefix = encoding.prefix;
    s.encoding.suffix = encoding.suffix;

    _z_subscriber_list_t *subs = __unsafe_z_get_subscriptions_by_name(zn, _Z_RESOURCE_IS_LOCAL, rname);
    _z_subscriber_list_t *xs = subs;
    while (xs != NULL)
    {
        _z_subscriber_t *sub = _z_subscriber_list_head(xs);
        sub->callback(&s, sub->arg);
        xs = _z_subscriber_list_tail(xs);
    }

    _z_reskey_clear(&key);
    _z_str_clear(rname);
    _z_list_free(&subs, _z_noop_free);
    _z_mutex_unlock(&zn->mutex_inner);
    return 0;

ERR:
    _z_mutex_unlock(&zn->mutex_inner);
    return -1;
}

void _z_unregister_subscription(_z_session_t *zn, int is_local, _z_subscriber_t *sub)
{
    _z_mutex_lock(&zn->mutex_inner);

    if (is_local)
        zn->local_subscriptions = _z_subscriber_list_drop_filter(zn->local_subscriptions, _z_subscriber_eq, sub);
    else
        zn->remote_subscriptions = _z_subscriber_list_drop_filter(zn->remote_subscriptions, _z_subscriber_eq, sub);

    _z_mutex_unlock(&zn->mutex_inner);
}

void _z_flush_subscriptions(_z_session_t *zn)
{
    _z_mutex_lock(&zn->mutex_inner);

    _z_subscriber_list_free(&zn->local_subscriptions);
    _z_subscriber_list_free(&zn->remote_subscriptions);

    _z_mutex_unlock(&zn->mutex_inner);
}
