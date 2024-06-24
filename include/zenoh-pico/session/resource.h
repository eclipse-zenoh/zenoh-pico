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

#ifndef INCLUDE_ZENOH_PICO_SESSION_RESOURCE_H
#define INCLUDE_ZENOH_PICO_SESSION_RESOURCE_H

#include <stdint.h>

#include "zenoh-pico/net/session.h"

/*------------------ Entity ------------------*/
uint32_t _z_get_entity_id(_z_session_t *zn);

/*------------------ Resource ------------------*/
uint16_t _z_get_resource_id(_z_session_t *zn);
_z_resource_t *_z_get_resource_by_id(_z_session_t *zn, uint16_t mapping, _z_zint_t rid);
_z_resource_t *_z_get_resource_by_key(_z_session_t *zn, const _z_keyexpr_t *keyexpr);
_z_keyexpr_t _z_get_expanded_key_from_key(_z_session_t *zn, const _z_keyexpr_t *keyexpr);
uint16_t _z_register_resource(_z_session_t *zn, const _z_keyexpr_t key, uint16_t id, uint16_t register_to_mapping);
void _z_unregister_resource(_z_session_t *zn, uint16_t id, uint16_t mapping);
void _z_unregister_resources_for_peer(_z_session_t *zn, uint16_t mapping);
void _z_flush_resources(_z_session_t *zn);

_z_keyexpr_t __unsafe_z_get_expanded_key_from_key(_z_session_t *zn, const _z_keyexpr_t *keyexpr);
_z_resource_t *__unsafe_z_get_resource_by_id(_z_session_t *zn, uint16_t mapping, _z_zint_t id);
_z_resource_t *__unsafe_z_get_resource_matching_key(_z_session_t *zn, const _z_keyexpr_t *keyexpr);

#endif /* INCLUDE_ZENOH_PICO_SESSION_RESOURCE_H */
