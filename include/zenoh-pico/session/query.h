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

#ifndef ZENOH_PICO_SESSION_QUERY_H
#define ZENOH_PICO_SESSION_QUERY_H

#include "zenoh-pico/net/session.h"

/*------------------ Query ------------------*/
_z_zint_t _z_get_query_id(_z_session_t *zn);

_z_pending_query_t *_z_get_pending_query_by_id(_z_session_t *zn, const _z_zint_t id);

int _z_register_pending_query(_z_session_t *zn, _z_pending_query_t *pq);
int _z_trigger_query_reply_partial(_z_session_t *zn, const _z_reply_context_t *reply_context, const _z_reskey_t reskey, const _z_bytes_t payload, const _z_data_info_t data_info);
int _z_trigger_query_reply_final(_z_session_t *zn, const _z_reply_context_t *reply_context);
void _z_unregister_pending_query(_z_session_t *zn, _z_pending_query_t *pq);
void _z_flush_pending_queries(_z_session_t *zn);

#endif /* ZENOH_PICO_SESSION_QUERY_H */
