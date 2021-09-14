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

#ifndef _ZENOH_PICO_SESSION_PRIVATE_SUBSCRIPTION_H
#define _ZENOH_PICO_SESSION_PRIVATE_SUBSCRIPTION_H

#include "zenoh-pico/utils/types.h"
#include "zenoh-pico/protocol/private/msg.h"
#include "zenoh-pico/protocol/private/msgcodec.h"
#include "zenoh-pico/session/types.h"
#include "zenoh-pico/session/private/types.h"

/*------------------ Subscription ------------------*/
z_list_t *_zn_get_subscriptions_from_remote_key(zn_session_t *zn, const zn_reskey_t *reskey);
_zn_subscriber_t *_zn_get_subscription_by_id(zn_session_t *zn, int is_local, z_zint_t id);
_zn_subscriber_t *_zn_get_subscription_by_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey);
int _zn_register_subscription(zn_session_t *zn, int is_local, _zn_subscriber_t *sub);
void _zn_unregister_subscription(zn_session_t *zn, int is_local, _zn_subscriber_t *sub);
void _zn_flush_subscriptions(zn_session_t *zn);
void _zn_trigger_subscriptions(zn_session_t *zn, const zn_reskey_t reskey, const z_bytes_t payload);

void __unsafe_zn_add_rem_res_to_loc_sub_map(zn_session_t *zn, z_zint_t id, zn_reskey_t *reskey);

/*------------------ Pull ------------------*/
z_zint_t _zn_get_pull_id(zn_session_t *zn);

#endif /* _ZENOH_PICO_SESSION_PRIVATE_SUBSCRIPTION_H */
