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

#ifndef ZENOH_PICO_SESSION_RESOURCE_H
#define ZENOH_PICO_SESSION_RESOURCE_H

#include "zenoh-pico/api/session.h"

/*------------------ Entity ------------------*/
z_zint_t _zn_get_entity_id(zn_session_t *zn);

/*------------------ Resource ------------------*/
z_zint_t _zn_get_resource_id(zn_session_t *zn);
_zn_resource_t *_zn_get_resource_by_id(zn_session_t *zn, int is_local, z_zint_t rid);
_zn_resource_t *_zn_get_resource_by_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey);
z_str_t _zn_get_resource_name_from_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey);
int _zn_register_resource(zn_session_t *zn, int is_local, _zn_resource_t *res);
void _zn_unregister_resource(zn_session_t *zn, int is_local, _zn_resource_t *res);
void _zn_flush_resources(zn_session_t *zn);

z_str_t __unsafe_zn_get_resource_name_from_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey);
_zn_resource_t *__unsafe_zn_get_resource_by_id(zn_session_t *zn, int is_local, z_zint_t id);
_zn_resource_t *__unsafe_zn_get_resource_matching_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey);

#endif /* ZENOH_PICO_SESSION_RESOURCE_H */
