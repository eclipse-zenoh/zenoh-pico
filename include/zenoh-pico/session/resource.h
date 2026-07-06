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

#include "zenoh-pico/session/keyexpr.h"
#include "zenoh-pico/session/weak_session.h"
#include "zenoh-pico/transport/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------ Entity ------------------*/
uint32_t _z_get_entity_id(_z_session_t *zn);

/*------------------ Resource ------------------*/
uint16_t _z_get_resource_id(_z_session_t *zn);
// Return a keyexpr view from a wireexpr. With the lifetime bound to the wireexpr itself, if it has no prefix, or to
// that of out_buf, if it has a prefix. The out_buf must be large enough to hold the keyexpr string representation.
z_result_t _z_get_keyexpr_view_from_wireexpr(_z_session_t *zn, _z_keyexpr_view_t *out, const _z_wireexpr_t *expr,
                                             size_t peer_id, char *out_buf, size_t out_buf_len);
z_result_t _z_get_keyexpr_from_wireexpr(_z_session_t *zn, _z_keyexpr_t *out, const _z_wireexpr_t *expr, size_t peer_id);
z_result_t _z_register_remote_resource(_z_session_t *zn, const _z_wireexpr_t *key, uint16_t id, size_t peer_id);
z_result_t _z_register_local_resource(_z_session_t *zn, const _z_string_t *expr, uint16_t *out_id);
z_result_t _z_unregister_remote_resource(_z_session_t *zn, uint16_t id, size_t peer_id);
z_result_t _z_unregister_local_resource(_z_session_t *zn, uint16_t id);
void _z_flush_local_resources(_z_session_t *zn);
void _z_flush_remote_resources(_z_session_t *zn);
void _z_flush_remote_resources_for_peer(_z_session_t *zn, size_t peer_id);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZENOH_PICO_SESSION_RESOURCE_H */
