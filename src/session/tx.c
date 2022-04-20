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

#include "zenoh-pico/transport/link/tx.h"
#include "zenoh-pico/utils/logging.h"

int _zn_send_z_msg(zn_session_t *zn, _zn_zenoh_message_t *z_msg, zn_reliability_t reliability, zn_congestion_control_t cong_ctrl)
{
    _Z_DEBUG(">> send zenoh message\n");

    if (zn->tp->type == _ZN_TRANSPORT_UNICAST_TYPE)
        return _zn_unicast_send_z_msg(zn, z_msg, reliability, cong_ctrl);
    else if (zn->tp->type == _ZN_TRANSPORT_MULTICAST_TYPE)
        return _zn_multicast_send_z_msg(zn, z_msg, reliability, cong_ctrl);
    else
        return -1;
}
