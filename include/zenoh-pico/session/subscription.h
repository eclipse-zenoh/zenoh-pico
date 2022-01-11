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

#ifndef ZENOH_PICO_SESSION_SUBSCRIPTION_H
#define ZENOH_PICO_SESSION_SUBSCRIPTION_H

#include "zenoh-pico/api/session.h"

/*------------------ Subscription ------------------*/
_zn_subscriber_t *_zn_get_subscription_by_id(zn_session_t *zn, int is_local, const z_zint_t id);
_zn_subscriber_list_t *_zn_get_subscriptions_by_name(zn_session_t *zn, int is_local, const z_str_t rname);
_zn_subscriber_list_t *_zn_get_subscription_by_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey);

int _zn_register_subscription(zn_session_t *zn, int is_local, _zn_subscriber_t *sub);
int _zn_trigger_subscriptions(zn_session_t *zn, const zn_reskey_t reskey, const z_bytes_t payload);
void _zn_unregister_subscription(zn_session_t *zn, int is_local, _zn_subscriber_t *sub);
void _zn_flush_subscriptions(zn_session_t *zn);

/*------------------ Pull ------------------*/
z_zint_t _zn_get_pull_id(zn_session_t *zn);

#endif /* ZENOH_PICO_SESSION_SUBSCRIPTION_H */
