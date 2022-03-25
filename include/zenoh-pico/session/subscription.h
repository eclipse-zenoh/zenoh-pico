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

#include "zenoh-pico/net/session.h"

/*------------------ Subscription ------------------*/
_z_subscriber_t *_z_get_subscription_by_id(_z_session_t *zn, int is_local, const _z_zint_t id);
_z_subscriber_list_t *_z_get_subscriptions_by_name(_z_session_t *zn, int is_local, const _z_str_t rname);
_z_subscriber_list_t *_z_get_subscription_by_key(_z_session_t *zn, int is_local, const _z_reskey_t *reskey);

int _z_register_subscription(_z_session_t *zn, int is_local, _z_subscriber_t *sub);
int _z_trigger_subscriptions(_z_session_t *zn, const _z_reskey_t reskey, const _z_bytes_t payload, const _z_encoding_t encoding);
void _z_unregister_subscription(_z_session_t *zn, int is_local, _z_subscriber_t *sub);
void _z_flush_subscriptions(_z_session_t *zn);

/*------------------ Pull ------------------*/
_z_zint_t _z_get_pull_id(_z_session_t *zn);

#endif /* ZENOH_PICO_SESSION_SUBSCRIPTION_H */
