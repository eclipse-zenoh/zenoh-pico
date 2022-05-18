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

#ifndef ZENOH_PICO_SESSION_QUERYABLE_H
#define ZENOH_PICO_SESSION_QUERYABLE_H

#include "zenoh-pico/net/session.h"

#define _Z_QUERYABLE_COMPLETE_DEFAULT 1
#define _Z_QUERYABLE_DISTANCE_DEFAULT 0

/*------------------ Queryable ------------------*/
_z_questionable_t *_z_get_queryable_by_id(_z_session_t *zn, const _z_zint_t id);
_z_questionable_list_t *_z_get_queryables_by_key(_z_session_t *zn, const _z_keyexpr_t key);

int _z_register_queryable(_z_session_t *zn, _z_questionable_t *q);
int _z_trigger_queryables(_z_session_t *zn, const _z_msg_query_t *query);
void _z_unregister_queryable(_z_session_t *zn, _z_questionable_t *q);
void _z_flush_queryables(_z_session_t *zn);

#endif /* ZENOH_PICO_SESSION_QUERYABLE_H */
