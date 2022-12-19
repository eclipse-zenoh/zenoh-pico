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

#include "zenoh-pico/transport/link/task/join.h"

#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/link/tx.h"

#if Z_MULTICAST_TRANSPORT == 1

int8_t _zp_multicast_send_join(_z_transport_multicast_t *ztm) {
    _z_conduit_sn_list_t next_sns;
    next_sns._is_qos = false;  // FIXME: make transport aware of qos configuration
    next_sns._val._plain._best_effort = ztm->_sn_tx_best_effort;
    next_sns._val._plain._reliable = ztm->_sn_tx_reliable;

    _z_bytes_t zid = _z_bytes_wrap(((_z_session_t *)ztm->_session)->_local_zid.start,
                                   ((_z_session_t *)ztm->_session)->_local_zid.len);
    _z_transport_message_t jsm =
        _z_t_msg_make_join(Z_PROTO_VERSION, Z_WHATAMI_PEER, Z_TRANSPORT_LEASE, Z_SN_RESOLUTION, zid, next_sns);

    return _z_multicast_send_t_msg(ztm, &jsm);
}

#endif  // Z_MULTICAST_TRANSPORT == 1
