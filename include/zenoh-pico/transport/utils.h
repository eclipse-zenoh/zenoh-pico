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

#ifndef ZENOH_PICO_TRANSPORT_UTILS_H
#define ZENOH_PICO_TRANSPORT_UTILS_H

#include "zenoh-pico/utils/collections.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/msg.h"
#include "zenoh-pico/protocol/msgcodec.h"

/*------------------ SN helpers ------------------*/
int _zn_sn_precedes(z_zint_t sn_resolution_half, z_zint_t sn_left, z_zint_t sn_right);

/*------------------ Transmission and Reception helpers ------------------*/
int _zn_send_t_msg(zn_session_t *zn, _zn_transport_message_t *m);
int _zn_send_z_msg(zn_session_t *zn, _zn_zenoh_message_t *m, zn_reliability_t reliability, zn_congestion_control_t cong_ctrl);

_zn_transport_message_p_result_t _zn_recv_t_msg(zn_session_t *zn);
void _zn_recv_t_msg_na(zn_session_t *zn, _zn_transport_message_p_result_t *r);

int _zn_handle_transport_message(zn_session_t *zn, _zn_transport_message_t *msg);

#endif /* ZENOH_PICO_TRANSPORT_UTILS_H */
