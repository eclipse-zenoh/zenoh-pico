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

#ifndef ZENOH_PICO_SESSION_UTILS_H
#define ZENOH_PICO_SESSION_UTILS_H

#include "zenoh-pico/api/session.h"

/*------------------ Session ------------------*/
zn_hello_array_t _zn_scout(const unsigned int what, const zn_properties_t *config, const unsigned long scout_period, const int exit_on_first);

zn_session_t *_zn_session_init();
int _zn_session_close(zn_session_t *zn, uint8_t reason);
void _zn_session_free(zn_session_t **zn);

int _zn_handle_zenoh_message(zn_session_t *zn, _zn_zenoh_message_t *z_msg);
int _zn_send_z_msg(zn_session_t *zn, _zn_zenoh_message_t *z_msg, zn_reliability_t reliability, zn_congestion_control_t cong_ctrl);

#endif /* ZENOH_PICO_SESSION_UTILS_H */
