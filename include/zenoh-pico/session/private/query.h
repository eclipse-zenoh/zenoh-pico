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

#ifndef _ZENOH_PICO_SESSION_PRIVATE_QUERY_H
#define _ZENOH_PICO_SESSION_PRIVATE_QUERY_H

#include "zenoh-pico/utils/types.h"
#include "zenoh-pico/session/types.h"
#include "zenoh-pico/session/private/types.h"
#include "zenoh-pico/protocol/types.h"
#include "zenoh-pico/protocol/private/msg.h"

/*------------------ Query ------------------*/
z_zint_t _zn_get_query_id(zn_session_t *zn);
int _zn_register_pending_query(zn_session_t *zn, _zn_pending_query_t *pq);
void _zn_unregister_pending_query(zn_session_t *zn, _zn_pending_query_t *pq);
void _zn_flush_pending_queries(zn_session_t *zn);
void _zn_trigger_query_reply_partial(zn_session_t *zn, const _zn_reply_context_t *reply_context, const zn_reskey_t reskey, const z_bytes_t payload, const _zn_data_info_t data_info);
void _zn_trigger_query_reply_final(zn_session_t *zn, const _zn_reply_context_t *reply_context);

#endif /* _ZENOH_PICO_SESSION_PRIVATE_QUERY_H */
