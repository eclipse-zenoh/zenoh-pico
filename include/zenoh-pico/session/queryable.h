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

#include "zenoh-pico/utils/collections.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/msg.h"

/*------------------ Queryable ------------------*/
_zn_queryable_t *_zn_get_queryable_by_id(zn_session_t *zn, z_zint_t id);
int _zn_register_queryable(zn_session_t *zn, _zn_queryable_t *q);
void _zn_unregister_queryable(zn_session_t *zn, _zn_queryable_t *q);
void _zn_flush_queryables(zn_session_t *zn);
void _zn_trigger_queryables(zn_session_t *zn, const _zn_query_t *query);

void __unsafe_zn_add_rem_res_to_loc_qle_map(zn_session_t *zn, z_zint_t id, zn_reskey_t *reskey);

#endif /* ZENOH_PICO_SESSION_QUERYABLE_H */
