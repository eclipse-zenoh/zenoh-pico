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

#ifndef ZENOH_PICO_SESSION_UTILS_H
#define ZENOH_PICO_SESSION_UTILS_H

#include "zenoh-pico/net/session.h"

/*------------------ Session ------------------*/
_z_hello_list_t *_z_scout_inner(const uint8_t what, const char *locator, const uint32_t timeout,
                              const int exit_on_first);

_z_session_t *_z_session_init(void);
int _z_session_close(_z_session_t *zn, uint8_t reason);
void _z_session_free(_z_session_t **zn);

int _z_handle_zenoh_message(_z_session_t *zn, _z_zenoh_message_t *z_msg);
int _z_send_z_msg(_z_session_t *zn, _z_zenoh_message_t *z_msg, z_reliability_t reliability,
                  z_congestion_control_t cong_ctrl);

#endif /* ZENOH_PICO_SESSION_UTILS_H */
