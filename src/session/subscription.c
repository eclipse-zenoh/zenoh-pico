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

#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/utils/logging.h"

int _zn_subscriber_eq(const _zn_subscriber_t *other, const _zn_subscriber_t *this)
{
    return this->id == other->id;
}

void _zn_subscriber_clear(_zn_subscriber_t *sub)
{
    _z_str_clear(sub->rname);
    _zn_reskey_clear(&sub->key);
    if (sub->info.period)
        z_free(sub->info.period);
}

/*------------------ Pull ------------------*/
z_zint_t _zn_get_pull_id(zn_session_t *zn)
{
    return zn->pull_id++;
}

_zn_subscriber_t *__zn_get_subscription_by_id(_zn_subscriber_list_t *subs, const z_zint_t id)
{
    _zn_subscriber_t *sub = NULL;
    while (subs != NULL)
    {
        sub = _zn_subscriber_list_head(subs);
        if (id == sub->id)
            return sub;

        subs = _zn_subscriber_list_tail(subs);
    }

    return sub;
}

_zn_subscriber_list_t *__zn_get_subscriptions_by_name(_zn_subscriber_list_t *subs, const z_str_t rname)
{
    _zn_subscriber_list_t *xs = NULL;
    while (subs != NULL)
    {
        _zn_subscriber_t *sub = _zn_subscriber_list_head(subs);
        if (zn_rname_intersect(sub->rname, rname))
            xs = _zn_subscriber_list_push(xs, sub);

        subs = _zn_subscriber_list_tail(subs);
    }

    return xs;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_zn_subscriber_t *__unsafe_zn_get_subscription_by_id(zn_session_t *zn, int is_local, const z_zint_t id)
{
    _zn_subscriber_list_t *subs = is_local ? zn->local_subscriptions : zn->remote_subscriptions;
    return __zn_get_subscription_by_id(subs, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_zn_subscriber_list_t *__unsafe_zn_get_subscriptions_by_name(zn_session_t *zn, int is_local, const z_str_t rname)
{
    _zn_subscriber_list_t *subs = is_local ? zn->local_subscriptions : zn->remote_subscriptions;
    return __zn_get_subscriptions_by_name(subs, rname);
}

_zn_subscriber_t *_zn_get_subscription_by_id(zn_session_t *zn, int is_local, const z_zint_t id)
{
    z_mutex_lock(&zn->mutex_inner);
    _zn_subscriber_t *sub = __unsafe_zn_get_subscription_by_id(zn, is_local, id);
    z_mutex_unlock(&zn->mutex_inner);
    return sub;
}

_zn_subscriber_list_t *_zn_get_subscriptions_by_name(zn_session_t *zn, int is_local, const z_str_t rname)
{
    z_mutex_lock(&zn->mutex_inner);
    _zn_subscriber_list_t *subs = __unsafe_zn_get_subscriptions_by_name(zn, is_local, rname);
    z_mutex_unlock(&zn->mutex_inner);
    return subs;
}

_zn_subscriber_list_t *_zn_get_subscription_by_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
{
    z_mutex_lock(&zn->mutex_inner);
    z_str_t rname = __unsafe_zn_get_resource_name_from_key(zn, is_local, reskey);
    _zn_subscriber_list_t *subs = __unsafe_zn_get_subscriptions_by_name(zn, is_local, rname);
    z_mutex_unlock(&zn->mutex_inner);

    _z_str_clear(rname);
    return subs;
}

int _zn_register_subscription(zn_session_t *zn, int is_local, _zn_subscriber_t *sub)
{
    _Z_DEBUG(">>> Allocating sub decl for (%s)\n", sub->rname);
    z_mutex_lock(&zn->mutex_inner);

    _zn_subscriber_list_t *subs = __unsafe_zn_get_subscriptions_by_name(zn, is_local, sub->rname);
    if (subs != NULL) // A subscription for this name already exists
        goto ERR;

    // Register the subscription
    if (is_local)
        zn->local_subscriptions = _zn_subscriber_list_push(zn->local_subscriptions, sub);
    else
        zn->remote_subscriptions = _zn_subscriber_list_push(zn->remote_subscriptions, sub);

    z_mutex_unlock(&zn->mutex_inner);
    return 0;

ERR:
    z_mutex_unlock(&zn->mutex_inner);
    return -1;
}

int _zn_trigger_subscriptions(zn_session_t *zn, const zn_reskey_t reskey, const z_bytes_t payload)
{
    z_mutex_lock(&zn->mutex_inner);

    z_str_t rname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_RESOURCE_REMOTE, &reskey);
    if (rname == NULL)
        goto ERR;

    // Build the sample
    zn_sample_t s;
    s.key.val = rname;
    s.key.len = strlen(s.key.val);
    s.value = payload;

    _zn_subscriber_list_t *subs = __unsafe_zn_get_subscriptions_by_name(zn, _ZN_RESOURCE_IS_LOCAL, rname);
    _zn_subscriber_list_t *xs = subs;
    while (xs != NULL)
    {
        _zn_subscriber_t *sub = _zn_subscriber_list_head(xs);
        sub->callback(&s, sub->arg);
        xs = _zn_subscriber_list_tail(xs);
    }

    _z_str_clear(rname);
    _z_list_free(&subs, _zn_noop_free);
    z_mutex_unlock(&zn->mutex_inner);
    return 0;

ERR:
    _z_str_clear(rname);
    z_mutex_unlock(&zn->mutex_inner);
    return -1;
}

void _zn_unregister_subscription(zn_session_t *zn, int is_local, _zn_subscriber_t *sub)
{
    z_mutex_lock(&zn->mutex_inner);

    if (is_local)
        zn->local_subscriptions = _zn_subscriber_list_drop_filter(zn->local_subscriptions, _zn_subscriber_eq, sub);
    else
        zn->remote_subscriptions = _zn_subscriber_list_drop_filter(zn->remote_subscriptions, _zn_subscriber_eq, sub);

    z_mutex_unlock(&zn->mutex_inner);
}

void _zn_flush_subscriptions(zn_session_t *zn)
{
    z_mutex_lock(&zn->mutex_inner);

    _zn_subscriber_list_free(&zn->local_subscriptions);
    _zn_subscriber_list_free(&zn->remote_subscriptions);

    z_mutex_unlock(&zn->mutex_inner);
}
