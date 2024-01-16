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

#include "zenoh-pico/transport/multicast/tx.h"

#include "zenoh-pico/transport/raweth/tx.h"
#include "zenoh-pico/transport/unicast/tx.h"
#include "zenoh-pico/utils/logging.h"

int8_t _z_send_n_msg(_z_session_t *zn, const _z_network_message_t *z_msg, z_reliability_t reliability,
                     z_congestion_control_t cong_ctrl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG(">> send network message");
    // Call transport function
    switch (zn->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            ret = _z_unicast_send_n_msg(zn, z_msg, reliability, cong_ctrl);
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            ret = _z_multicast_send_n_msg(zn, z_msg, reliability, cong_ctrl);
            break;
        case _Z_TRANSPORT_RAWETH_TYPE:
            ret = _z_raweth_send_n_msg(zn, z_msg, reliability, cong_ctrl);
            break;
        default:
            ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
            break;
    }
    return ret;
}
