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
                                             _z_transport_peer_common_t *peer, char *out_buf, size_t out_buf_len);
z_result_t _z_get_keyexpr_from_wireexpr(_z_session_t *zn, _z_keyexpr_t *out, const _z_wireexpr_t *expr,
                                        _z_transport_peer_common_t *peer);
z_result_t _z_register_resource(_z_session_t *zn, const _z_wireexpr_t *key, uint16_t id,
                                _z_transport_peer_common_t *peer, uint16_t *out_id);
z_result_t _z_unregister_resource(_z_session_t *zn, uint16_t id, _z_transport_peer_common_t *peer);
void _z_flush_local_resources(_z_session_t *zn);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZENOH_PICO_SESSION_RESOURCE_H */
