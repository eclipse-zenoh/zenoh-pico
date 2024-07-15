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

#ifndef INCLUDE_ZENOH_PICO_SESSION_UTILS_H
#define INCLUDE_ZENOH_PICO_SESSION_UTILS_H

#include <stdbool.h>
#include <stdint.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/core.h"

/*------------------ Session ------------------*/
_z_hello_list_t *_z_scout_inner(const z_what_t what, _z_id_t id, const char *locator, const uint32_t timeout,
                                const _Bool exit_on_first);

int8_t _z_session_init(_z_session_rc_t *zsrc, _z_id_t *zid);
void _z_session_clear(_z_session_t *zn);
int8_t _z_session_close(_z_session_t *zn, uint8_t reason);

int8_t _z_handle_network_message(_z_session_rc_t *zsrc, _z_zenoh_message_t *z_msg, uint16_t local_peer_id);
int8_t _z_send_n_msg(_z_session_t *zn, _z_network_message_t *n_msg, z_reliability_t reliability,
                     z_congestion_control_t cong_ctrl);

void _zp_session_lock_mutex(_z_session_t *zn);
void _zp_session_unlock_mutex(_z_session_t *zn);

#endif /* INCLUDE_ZENOH_PICO_SESSION_UTILS_H */
