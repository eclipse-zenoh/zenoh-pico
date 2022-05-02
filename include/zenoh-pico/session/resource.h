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

#ifndef ZENOH_PICO_SESSION_RESOURCE_H
#define ZENOH_PICO_SESSION_RESOURCE_H

#include "zenoh-pico/net/session.h"

/*------------------ Entity ------------------*/
_z_zint_t _z_get_entity_id(_z_session_t *zn);

/*------------------ Resource ------------------*/
_z_zint_t _z_get_resource_id(_z_session_t *zn);
_z_resource_t *_z_get_resource_by_id(_z_session_t *zn, int is_local, _z_zint_t rid);
_z_resource_t *_z_get_resource_by_key(_z_session_t *zn, int is_local, const _z_keyexpr_t *keyexpr);
_z_str_t _z_get_resource_name_from_key(_z_session_t *zn, int is_local, const _z_keyexpr_t *keyexpr);
int _z_register_resource(_z_session_t *zn, int is_local, _z_resource_t *res);
void _z_unregister_resource(_z_session_t *zn, int is_local, _z_resource_t *res);
void _z_flush_resources(_z_session_t *zn);

_z_str_t __unsafe_z_get_resource_name_from_key(_z_session_t *zn, int is_local, const _z_keyexpr_t *keyexpr);
_z_resource_t *__unsafe_z_get_resource_by_id(_z_session_t *zn, int is_local, _z_zint_t id);
_z_resource_t *__unsafe_z_get_resource_matching_key(_z_session_t *zn, int is_local, const _z_keyexpr_t *keyexpr);

#endif /* ZENOH_PICO_SESSION_RESOURCE_H */
