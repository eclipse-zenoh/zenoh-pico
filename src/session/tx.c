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

#include "zenoh-pico/transport/link/tx.h"
#include "zenoh-pico/utils/logging.h"

int _z_send_z_msg(_z_session_t *zn, _z_zenoh_message_t *z_msg, z_reliability_t reliability, _z_congestion_control_t cong_ctrl)
{
    _Z_DEBUG(">> send zenoh message\n");

    if (zn->_tp->_type == _Z_TRANSPORT_UNICAST_TYPE)
        return _z_unicast_send_z_msg(zn, z_msg, reliability, cong_ctrl);
    else if (zn->_tp->_type == _Z_TRANSPORT_MULTICAST_TYPE)
        return _z_multicast_send_z_msg(zn, z_msg, reliability, cong_ctrl);
    else
        return -1;
}

